extern crate serde;
extern crate serde_json;
extern crate libc;

use libc::{c_char, size_t};
use serde::{Serialize, Deserialize};
use std::{collections::HashMap, ffi::CString, ptr, slice};

//==============================================================================
// Twitter Benchmark Structures
// These match the C++ TwitterData structures exactly
//==============================================================================

#[derive(Serialize, Deserialize)]
pub struct User {
    id: u64,
    name: String,
    screen_name: String,
    location: String,
    description: String,
    verified: bool,
    followers_count: u64,
    friends_count: u64,
    statuses_count: u64,
}

#[derive(Serialize, Deserialize)]
pub struct Status {
    created_at: String,
    id: u64,
    text: String,
    user: User,
    retweet_count: u64,
    favorite_count: u64,
}

#[derive(Serialize, Deserialize)]
pub struct TwitterData {
    statuses: Vec<Status>,
}

static mut TWITTER_DATA: *mut TwitterData = std::ptr::null_mut();

#[no_mangle]
pub unsafe extern "C" fn twitter_from_str(raw_input: *const c_char, raw_input_length: size_t) -> *mut TwitterData {
    let input = std::str::from_utf8_unchecked(slice::from_raw_parts(raw_input as *const u8, raw_input_length));
    match serde_json::from_str(&input) {
        Ok(result) => Box::into_raw(Box::new(result)),
        Err(_) => std::ptr::null_mut(),
    }
}

#[no_mangle]
pub unsafe extern "C" fn set_twitter_data(raw: *mut TwitterData) {
    TWITTER_DATA = raw;
}

#[no_mangle]
pub unsafe extern "C" fn serialize_twitter_to_string() -> *const c_char {
    if TWITTER_DATA.is_null() {
        return std::ptr::null();
    }
    let data = &*TWITTER_DATA;
    let serialized = serde_json::to_string(data).unwrap();
    std::ffi::CString::new(serialized).unwrap().into_raw() as *const c_char
}

#[no_mangle]
pub unsafe extern "C" fn free_twitter(raw: *mut TwitterData) {
    if raw.is_null() {
        return;
    }
    drop(Box::from_raw(raw))
}

#[no_mangle]
pub unsafe extern fn free_string(ptr: *const c_char) {
    let _ = std::ffi::CString::from_raw(ptr as *mut _);
}

//==============================================================================
// CITM Catalog Benchmark Structures
// These match the C++ CitmCatalog structures EXACTLY for fair comparison
//==============================================================================

/// Matches C++ CITMPrice struct exactly
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CITMPrice {
    pub amount: u64,
    #[serde(rename = "audienceSubCategoryId")]
    pub audience_sub_category_id: u64,
    #[serde(rename = "seatCategoryId")]
    pub seat_category_id: u64,
}

/// Matches C++ CITMArea struct exactly
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CITMArea {
    #[serde(rename = "areaId")]
    pub area_id: u64,
    #[serde(rename = "blockIds")]
    pub block_ids: Vec<u64>,
}

/// Matches C++ CITMSeatCategory struct exactly
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CITMSeatCategory {
    pub areas: Vec<CITMArea>,
    #[serde(rename = "seatCategoryId")]
    pub seat_category_id: u64,
}

/// Matches C++ CITMPerformance struct exactly
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CITMPerformance {
    pub id: u64,
    #[serde(rename = "eventId")]
    pub event_id: u64,
    #[serde(default)]
    pub logo: Option<String>,
    #[serde(default)]
    pub name: Option<String>,
    pub prices: Vec<CITMPrice>,
    #[serde(rename = "seatCategories")]
    pub seat_categories: Vec<CITMSeatCategory>,
    #[serde(default)]
    #[serde(rename = "seatMapImage")]
    pub seat_map_image: Option<String>,
    pub start: u64,
    #[serde(rename = "venueCode")]
    pub venue_code: String,
}

/// Matches C++ CITMEvent struct exactly
#[derive(Serialize, Deserialize, Debug, Clone)]
pub struct CITMEvent {
    pub id: u64,
    #[serde(default)]
    pub name: Option<String>,
    #[serde(default)]
    pub description: Option<String>,
    #[serde(default)]
    pub logo: Option<String>,
    #[serde(default)]
    #[serde(rename = "subTopicIds")]
    pub sub_topic_ids: Vec<u64>,
    #[serde(default)]
    #[serde(rename = "subjectCode")]
    pub subject_code: Option<String>,
    #[serde(default)]
    pub subtitle: Option<String>,
    #[serde(default)]
    #[serde(rename = "topicIds")]
    pub topic_ids: Vec<u64>,
}

/// Matches C++ CitmCatalog struct exactly - ONLY events and performances
/// This is the key fix: we serialize only what C++ serializes
#[derive(Serialize, Deserialize, Debug)]
pub struct CitmCatalog {
    pub events: HashMap<String, CITMEvent>,
    pub performances: Vec<CITMPerformance>,
}

static mut CITM_DATA: *mut CitmCatalog = std::ptr::null_mut();

/// Creates a CitmCatalog from a JSON string (UTF-8 encoded).
/// Only extracts events and performances to match C++ behavior.
#[no_mangle]
pub unsafe extern "C" fn citm_from_str(
    raw_input: *const c_char,
    raw_input_length: usize
) -> *mut CitmCatalog {
    if raw_input.is_null() {
        eprintln!("Error: Input pointer is null");
        return ptr::null_mut();
    }

    let bytes = slice::from_raw_parts(raw_input as *const u8, raw_input_length);
    let input_str = match std::str::from_utf8(bytes) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Error: Invalid UTF-8 string: {}", e);
            return ptr::null_mut();
        }
    };

    // Parse the full JSON to extract only events and performances
    match serde_json::from_str::<serde_json::Value>(input_str) {
        Ok(full_json) => {
            // Extract only the fields we need (matching C++ behavior)
            let events: HashMap<String, CITMEvent> = full_json.get("events")
                .and_then(|v| serde_json::from_value(v.clone()).ok())
                .unwrap_or_default();

            let performances: Vec<CITMPerformance> = full_json.get("performances")
                .and_then(|v| serde_json::from_value(v.clone()).ok())
                .unwrap_or_default();

            let catalog = CitmCatalog { events, performances };
            Box::into_raw(Box::new(catalog))
        },
        Err(e) => {
            eprintln!("Error deserializing JSON: {}", e);
            ptr::null_mut()
        }
    }
}

/// Serializes a CitmCatalog into a JSON string (UTF-8).
#[no_mangle]
pub unsafe extern "C" fn set_citm_data(raw: *mut CitmCatalog) {
    CITM_DATA = raw;
}

#[no_mangle]
pub unsafe extern "C" fn serialize_citm_to_string() -> *mut c_char {
    if CITM_DATA.is_null() {
        return std::ptr::null_mut();
    }
    let data = &*CITM_DATA;
    let serialized = serde_json::to_string(data).unwrap();
    std::ffi::CString::new(serialized).unwrap().into_raw()
}

/// Frees the CitmCatalog pointer.
#[no_mangle]
pub unsafe extern "C" fn free_citm(raw_catalog: *mut CitmCatalog) {
    if !raw_catalog.is_null() {
        drop(Box::from_raw(raw_catalog));
    }
}

#[no_mangle]
pub extern "C" fn free_str(ptr: *mut c_char) {
    if !ptr.is_null() {
        unsafe {
            let _ = CString::from_raw(ptr);
        }
    }
}

//==============================================================================
// FFI Overhead Measurement Functions
// These allow measuring the actual FFI overhead vs pure Rust serialization
//==============================================================================

/// Result structure for FFI overhead measurement
#[repr(C)]
pub struct FfiOverheadResult {
    /// Time in nanoseconds for pure serde_json::to_string() (no FFI overhead)
    pub pure_serde_ns: u64,
    /// Time in nanoseconds for serde + CString conversion
    pub serde_plus_cstring_ns: u64,
    /// Number of iterations performed
    pub iterations: u64,
    /// Output size in bytes (for verification)
    pub output_size: u64,
}

/// Prevents compiler from optimizing away the value
/// Works on stable Rust (unlike std::hint::black_box which is unstable)
#[inline(never)]
fn black_box<T>(dummy: T) -> T {
    unsafe {
        let ret = std::ptr::read_volatile(&dummy);
        std::mem::forget(dummy);
        ret
    }
}

/// Measures FFI overhead for Twitter serialization.
/// Performs `iterations` serializations entirely in Rust and returns timing data.
/// This allows comparing against per-call FFI overhead.
#[no_mangle]
pub unsafe extern "C" fn measure_twitter_ffi_overhead(
    raw: *mut TwitterData,
    iterations: u64
) -> FfiOverheadResult {
    use std::time::Instant;

    let twitter_data = &*raw;
    let output_size: u64;

    // Warm-up run
    let warmup = serde_json::to_string(&twitter_data).unwrap();
    output_size = warmup.len() as u64;

    // Measure pure serde_json::to_string() - no CString conversion
    let start_pure = Instant::now();
    for _ in 0..iterations {
        let serialized = serde_json::to_string(&twitter_data).unwrap();
        // Prevent optimization from eliminating the work
        black_box(&serialized);
    }
    let pure_serde_ns = start_pure.elapsed().as_nanos() as u64;

    // Measure serde + CString conversion (but not FFI return)
    let start_cstring = Instant::now();
    for _ in 0..iterations {
        let serialized = serde_json::to_string(&twitter_data).unwrap();
        let cstring = CString::new(serialized).unwrap();
        // Prevent optimization from eliminating the work
        black_box(&cstring);
    }
    let serde_plus_cstring_ns = start_cstring.elapsed().as_nanos() as u64;

    FfiOverheadResult {
        pure_serde_ns,
        serde_plus_cstring_ns,
        iterations,
        output_size,
    }
}

/// Measures FFI overhead for CITM serialization.
#[no_mangle]
pub unsafe extern "C" fn measure_citm_ffi_overhead(
    raw: *mut CitmCatalog,
    iterations: u64
) -> FfiOverheadResult {
    use std::time::Instant;

    let catalog = &*raw;
    let output_size: u64;

    // Warm-up run
    let warmup = serde_json::to_string(&catalog).unwrap();
    output_size = warmup.len() as u64;

    // Measure pure serde_json::to_string() - no CString conversion
    let start_pure = Instant::now();
    for _ in 0..iterations {
        let serialized = serde_json::to_string(&catalog).unwrap();
        black_box(&serialized);
    }
    let pure_serde_ns = start_pure.elapsed().as_nanos() as u64;

    // Measure serde + CString conversion
    let start_cstring = Instant::now();
    for _ in 0..iterations {
        let serialized = serde_json::to_string(&catalog).unwrap();
        let cstring = CString::new(serialized).unwrap();
        black_box(&cstring);
    }
    let serde_plus_cstring_ns = start_cstring.elapsed().as_nanos() as u64;

    FfiOverheadResult {
        pure_serde_ns,
        serde_plus_cstring_ns,
        iterations,
        output_size,
    }
}
