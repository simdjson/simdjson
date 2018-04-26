// https://github.com/WojciechMula/toys/blob/master/000helpers/linux-perf-events.h
#pragma once
#ifdef __linux__

#include <unistd.h>             // for syscall
#include <sys/ioctl.h>          // for ioctl
#include <asm/unistd.h>         // for __NR_perf_event_open
#include <linux/perf_event.h>   // for perf event constants

#include <cerrno>               // for errno
#include <cstring>              // for memset
#include <stdexcept>


template <int TYPE = PERF_TYPE_HARDWARE>
class LinuxEvents {

    int fd;
    perf_event_attr attribs;

public:
    LinuxEvents(int config) : fd(0) {
        memset(&attribs, 0, sizeof(attribs));
        attribs.type        = TYPE;
        attribs.size        = sizeof(attribs);
        attribs.config      = config;
        attribs.disabled        = 1;
        attribs.exclude_kernel  = 1;
        attribs.exclude_hv      = 1;

        const int pid = 0;    // the current process
        const int cpu = -1;   // all CPUs
        const int group = -1; // no group
        const unsigned long flags = 0;
        fd = syscall(__NR_perf_event_open, &attribs, pid, cpu, group, flags);
        if (fd == -1) {
            report_error("perf_event_open");
        }
    }

    ~LinuxEvents() {
        close(fd);
    }

    void start() {
        if (ioctl(fd, PERF_EVENT_IOC_RESET, 0) == -1) {
            report_error("ioctl(PERF_EVENT_IOC_RESET)");
        }

        if (ioctl(fd, PERF_EVENT_IOC_ENABLE, 0) == -1) {
            report_error("ioctl(PERF_EVENT_IOC_ENABLE)");
        }
    }

    unsigned long end() {
        if (ioctl(fd, PERF_EVENT_IOC_DISABLE, 0) == -1) {
            report_error("ioctl(PERF_EVENT_IOC_DISABLE)");
        }

        unsigned long result;
        if (read(fd, &result, sizeof(result)) == -1) {
            report_error("read");
        }

        return result;
    }

private:
    void report_error(const std::string& context) {
        throw std::runtime_error(context + ": " + std::string(strerror(errno)));
    }

};
#endif 
