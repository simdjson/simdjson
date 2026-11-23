#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
// simdjson::padded_memory_map needs <windows.h> before simdjson.h.
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#else
#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "input.h"

#include "simdjson.h"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace sjq {

std::atomic<bool> g_input_changed{false};

// The mapping being read, for the fault handler.
static std::atomic<const char *> g_map_base{nullptr};
static std::atomic<size_t> g_map_len{0};

struct input_source::impl {
  simdjson::padded_string owned{};
#if SIMDJSON_HAS_PADDED_MEMORY_MAP
  std::unique_ptr<simdjson::padded_memory_map> map{};
#endif
  const char *p{nullptr};
  size_t n{0};
  bool mapped{false};
#if !defined(_WIN32)
  int fd{-1};  // kept open to notice when the file shrinks
#endif

  ~impl() {
    if (mapped) {
      g_map_base.store(nullptr);
      g_map_len.store(0);
    }
#if !defined(_WIN32)
    if (fd >= 0) { close(fd); }
#endif
  }

#if SIMDJSON_HAS_PADDED_MEMORY_MAP
  bool map_path(const char *path) {
    std::unique_ptr<simdjson::padded_memory_map> m(new (std::nothrow) simdjson::padded_memory_map(path));
    if (!m || !m->is_valid()) { return false; }
    simdjson::padded_string_view v = m->view();
    if (v.size() == 0) { return false; }
    p = v.data();
    n = v.size();
    map = std::move(m);
    mapped = true;
#if !defined(_WIN32)
    madvise(const_cast<char *>(p), n, MADV_SEQUENTIAL);
#endif
    g_input_changed.store(false);
    g_map_len.store(n);
    g_map_base.store(p);
    return true;
  }
#endif
};

input_source::input_source() : p_(new impl()) {}
input_source::~input_source() = default;

const char *input_source::data() const { return p_->p; }
size_t input_source::size() const { return p_->n; }

#if defined(_WIN32)
static bool is_regular_file(const char *path) {
  DWORD attr = GetFileAttributesA(path);
  return attr != INVALID_FILE_ATTRIBUTES && !(attr & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_DEVICE));
}
#else
static bool is_regular_file(const char *path) {
  struct stat st{};
  return stat(path, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}
#endif

bool input_source::load(const std::string &fname, bool use_mmap) {
#if SIMDJSON_HAS_PADDED_MEMORY_MAP
  if (use_mmap && is_regular_file(fname.c_str()) && p_->map_path(fname.c_str())) {
#if !defined(_WIN32)
    p_->fd = open(fname.c_str(), O_RDONLY);
    // Test hook: truncate the file right after mapping it.
    if (const char *t = getenv("SJQ_TEST_TRUNCATE")) {
      if (truncate(fname.c_str(), off_t(atoll(t))) != 0) { perror("truncate"); }
    }
#endif
    return true;
  }
#else
  (void)use_mmap;
#endif
  if (simdjson::padded_string::load(fname).get(p_->owned)) { return false; }
  p_->p = p_->owned.data();
  p_->n = p_->owned.size();
  return true;
}

bool input_source::load_stdin_file(bool use_mmap) {
  if (!use_mmap) { return false; }
#if SIMDJSON_HAS_PADDED_MEMORY_MAP
#if defined(_WIN32)
  char path[MAX_PATH * 4];
  DWORD len = GetFinalPathNameByHandleA(GetStdHandle(STD_INPUT_HANDLE), path, DWORD(sizeof(path)), FILE_NAME_NORMALIZED);
  if (len == 0 || len >= sizeof(path)) { return false; }
  return p_->map_path(path);
#else
  if (!p_->map_path("/dev/stdin")) { return false; }
  p_->fd = dup(0);
  return true;
#endif
#else
  return false;
#endif
}

bool input_source::changed() const {
  if (!p_->mapped) { return false; }
  if (g_input_changed.load()) { return true; }
#if defined(_WIN32)
  // A file with a mapped view cannot be truncated on Windows.
  return false;
#else
  // A shrink within the last page does not fault (the kernel zero-fills it),
  // and macOS may keep serving the old pages of a mapping.
  struct stat st{};
  return p_->fd < 0 || fstat(p_->fd, &st) != 0 || size_t(st.st_size) < p_->n;
#endif
}

#if defined(_WIN32)

// On Windows a mapped file cannot shrink, but reading its pages can still
// fail (a network share that goes away, a removed drive): the read raises
// EXCEPTION_IN_PAGE_ERROR. Inside our mapping, report it and exit instead of
// crashing; other exceptions continue their normal handling.
static LONG CALLBACK on_mapped_fault(EXCEPTION_POINTERS *info) {
  const EXCEPTION_RECORD *rec = info->ExceptionRecord;
  if (rec->ExceptionCode == EXCEPTION_IN_PAGE_ERROR && rec->NumberParameters >= 2) {
    const char *addr = reinterpret_cast<const char *>(rec->ExceptionInformation[1]);
    const char *base = g_map_base.load();
    size_t len = g_map_len.load();
    if (base && addr >= base && addr < base + len) {
      static const char msg[] = "sjq: error: cannot read the input file (I/O error); the output is incomplete\n";
      DWORD written = 0;
      WriteFile(GetStdHandle(STD_ERROR_HANDLE), msg, DWORD(sizeof(msg) - 1), &written, NULL);
      TerminateProcess(GetCurrentProcess(), 2);
    }
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

void install_fault_handler() { AddVectoredExceptionHandler(1, on_mapped_fault); }

// sjq.exe is linked with an 8 MiB stack (see CMakeLists.txt); leave a margin.
size_t main_stack_budget() { return size_t(6) << 20; }

bool stdout_is_terminal() { return _isatty(_fileno(stdout)) != 0; }

bool stdin_is_regular_file() {
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  LARGE_INTEGER size{};
  return GetFileType(h) == FILE_TYPE_DISK && GetFileSizeEx(h, &size) && size.QuadPart > 0;
}

long long read_stdin(char *dst, size_t n) {
  unsigned chunk = unsigned(n > (size_t(1) << 30) ? (size_t(1) << 30) : n);
  int r = _read(0, dst, chunk);
  return r < 0 ? -1 : (long long)r;
}

bool stdin_has_more() {
  HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
  if (GetFileType(h) != FILE_TYPE_PIPE) { return false; }
  DWORD avail = 0;
  return PeekNamedPipe(h, NULL, 0, NULL, &avail, NULL) && avail > 0;
}

bool prepare_console(bool binary_output) {
  _setmode(_fileno(stdin), _O_BINARY);
  if (binary_output) { _setmode(_fileno(stdout), _O_BINARY); }
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD mode = 0;
  if (!GetConsoleMode(out, &mode)) { return true; }  // not a console
  SetConsoleOutputCP(CP_UTF8);
  return SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
}

#else

// When a mapped file shrinks, reading its vanished pages raises SIGBUS. The
// handler, for faults inside the mapping only, maps zero pages over the rest
// of it and returns, so the read is retried and sees zeros; the drivers check
// g_input_changed and stop at the next document. Faults elsewhere crash as
// before: the previous action (default, or a sanitizer's handler) is put back
// and the faulting instruction runs again.
static struct sigaction g_old_bus, g_old_segv;
static void on_mapped_fault(int sig, siginfo_t *si, void *) {
  char *addr = static_cast<char *>(si->si_addr);
  const char *base = g_map_base.load();
  size_t len = g_map_len.load();
  if (base && addr >= base && addr < base + len) {
    size_t page = size_t(getpagesize());
    char *start = reinterpret_cast<char *>(reinterpret_cast<uintptr_t>(addr) & ~uintptr_t(page - 1));
    if (mmap(start, size_t(base + len - start), PROT_READ, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0) != MAP_FAILED) {
      g_input_changed.store(true);
      return;
    }
  }
  sigaction(sig, sig == SIGBUS ? &g_old_bus : &g_old_segv, nullptr);
  if (si->si_code <= 0) { raise(sig); }  // sent by a process, not a fault
}

void install_fault_handler() {
  struct sigaction sa{};
  memset(&sa, 0, sizeof(sa));
  sa.sa_sigaction = on_mapped_fault;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  // Some systems raise SIGSEGV rather than SIGBUS for a vanished page.
  sigaction(SIGBUS, &sa, &g_old_bus);
  sigaction(SIGSEGV, &sa, &g_old_segv);
}

// Three quarters of the stack limit, which the main thread gets.
size_t main_stack_budget() {
  struct rlimit rl;
  if (getrlimit(RLIMIT_STACK, &rl) != 0 || rl.rlim_cur == RLIM_INFINITY || rl.rlim_cur > (rlim_t(64) << 20)) {
    return size_t(48) << 20;
  }
  return size_t(rl.rlim_cur) / 4 * 3;
}

bool stdout_is_terminal() { return isatty(STDOUT_FILENO) != 0; }

bool stdin_is_regular_file() {
  struct stat st{};
  return fstat(0, &st) == 0 && S_ISREG(st.st_mode) && st.st_size > 0;
}

long long read_stdin(char *dst, size_t n) {
  while (true) {
    ssize_t r = read(0, dst, n);
    if (r < 0 && errno == EINTR) { continue; }
    return (long long)r;
  }
}

bool stdin_has_more() {
  struct pollfd pfd{};
  pfd.fd = 0;
  pfd.events = POLLIN;
  pfd.revents = 0;
  return poll(&pfd, 1, 0) > 0;
}

bool prepare_console(bool) { return true; }

#endif

} // namespace sjq
