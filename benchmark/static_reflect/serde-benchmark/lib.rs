extern crate serde;
extern crate serde_json;
extern crate libc;

use libc::{c_char, size_t};
use serde::{Serialize, Deserialize};
use std::{collections::HashMap, ffi::CString, ptr, slice};
use serde::de::{self, Deserializer};
/******************************************************/
/******************************************************/
/**
 * Warning: the C++ code may not generate the same JSON.
 */
 /******************************************************/
 /******************************************************/

// This has no equivalent in C++:
#[derive(Serialize, Deserialize)]
pub struct Metadata {
    result_type: String,
    iso_language_code: String,
}

#[derive(Serialize, Deserialize)]
pub struct User {
    id: i64,
    id_str: String,
    name: String,
    screen_name: String,
    location: String,
    description: String,
    // C++ does not have those:
    // url: Option<String>,
    //protected: bool,
    //listed_count: i64,
    //created_at: String,
    //favourites_count: i64,
    //utc_offset: Option<i64>,
    //time_zone: Option<String>,
    //geo_enabled: bool,
    verified: bool,
    followers_count: i64,
    friends_count: i64,
    statuses_count: i64,
    // C++ does not have those:
    //lang: String,
    //profile_background_color: String,
    //profile_background_image_url: String,
    //profile_background_image_url_https: String,
    //profile_background_tile: bool,
    //profile_image_url: String,
    //profile_image_url_https: String,
    //profile_banner_url: Option<String>,
    //profile_link_color: String,
    //profile_sidebar_border_color: String,
    //profile_sidebar_fill_color: String,
    //profile_text_color: String,
    //profile_use_background_image: bool,
    //default_profile: bool,
    //default_profile_image: bool,
    //following: bool,
    //follow_request_sent: bool,
    //notifications: bool,
}

#[derive(Serialize, Deserialize)]
pub struct Hashtag {
    text: String,

    // C++ has those but D. Lemire does not know what they are, they don't appear in the JSON:
    // int64_t indices_start;
    // int64_t indices_end;
}

#[derive(Serialize, Deserialize)]
pub struct Url {
    url: String,
    expanded_url: String,
    display_url: String,
    // C++ has those but D. Lemire does not know what they are, they don't appear in the JSON:
    // int64_t indices_start;
    // int64_t indices_end;
}

#[derive(Serialize, Deserialize)]
pub struct UserMention {
    id: i64,
    name: String,
    screen_name: String,
    // Not in the C++ equivalent:
    //id_str: String,
    //indices: Vec<i64>,
    // C++ has those but D. Lemire does not know what they are, they don't appear in the JSON:
    // int64_t indices_start;
    // int64_t indices_end;
}

#[derive(Serialize, Deserialize)]
pub struct Entities {
    hashtags: Vec<Hashtag>,
    urls: Vec<Url>,
    user_mentions: Vec<UserMention>,
}

#[derive(Serialize, Deserialize)]
pub struct Status {
    created_at: String,
    id: i64,
    text: String,
    user: User,
    entities: Entities,
    retweet_count: i64,
    favorite_count: i64,
    favorited: bool,
    retweeted: bool,
    // None of these are in the C++ equivalent:
    /*
    metadata: Metadata,
    id_str: String,
    source: String,
    truncated: bool,
    in_reply_to_status_id: Option<i64>,
    in_reply_to_status_id_str: Option<String>,
    in_reply_to_user_id: Option<i64>,
    in_reply_to_user_id_str: Option<String>,
    in_reply_to_screen_name: Option<String>,
    geo: Option<String>,
    coordinates: Option<String>,
    place: Option<String>,
    contributors: Option<String>,
    lang: String,
    */
}

#[derive(Serialize, Deserialize)]
pub struct TwitterData {
    statuses: Vec<Status>,
}

#[no_mangle]
pub unsafe extern "C" fn  twitter_from_str(raw_input: *const c_char, raw_input_length: size_t) -> *mut TwitterData {
  let input = std::str::from_utf8_unchecked(slice::from_raw_parts(raw_input as *const u8, raw_input_length));
  match serde_json::from_str(&input) {
    Ok(result) => Box::into_raw(Box::new(result)),
    Err(_) => std::ptr::null_mut(),
  }
}

#[no_mangle]
pub unsafe extern "C" fn str_from_twitter(raw: *mut TwitterData) -> *const c_char {
  let twitter_thing = { &*raw };
  let serialized = serde_json::to_string(&twitter_thing).unwrap();
  return std::ffi::CString::new(serialized.as_str()).unwrap().into_raw()
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

// Functions associated with the CitmCatalog benchmark

#[derive(Serialize, Deserialize)]
pub struct Area {
    pub id: i64,
    pub name: Option<String>,  // Changed to Option
    pub parent: i64,
    #[serde(rename = "childAreas")]
    pub child_areas: Vec<i64>,
}

#[derive(Serialize, Deserialize)]
pub struct AudienceSubCategory {
    pub id: i64,
    pub name: Option<String>,  // Changed to Option
    pub parent: i64,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Event {
    #[serde(default)]
    pub description: Option<String>,
    pub id: i64,
    #[serde(default)]
    pub logo: Option<String>,
    #[serde(default)]
    pub name: Option<String>,
    #[serde(default)]
    pub subTopicIds: Vec<i64>,
    #[serde(default)]
    pub subjectCode: Option<String>,
    #[serde(default)]
    pub subtitle: Option<String>,
    #[serde(default)]
    pub topicIds: Vec<i64>,
    // Add a catch-all for any other fields
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

#[derive(Serialize, Deserialize, Debug)]
pub struct Performance {
    #[serde(default)]
    pub id: i64,

    #[serde(default)]
    pub name: Option<String>,

    #[serde(default)]
    pub event: i64,

    // This is the key fix - accept any JSON value type for timestamps
    // This allows both string dates and integer timestamps (line 3511)
    #[serde(default)]
    pub start: serde_json::Value,

    #[serde(rename = "venueCode")]
    pub venue_code: String,

    // Add a catch-all for any other fields
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

#[derive(Serialize, Deserialize)]
pub struct SeatCategory {
    pub id: i64,
    pub name: Option<String>,  // Changed to Option
    pub areas: Vec<i64>,
}

#[derive(Serialize, Deserialize)]
pub struct SubTopic {
    pub id: i64,
    pub name: Option<String>,  // Changed to Option
    pub parent: i64,
}

#[derive(Serialize, Deserialize)]
pub struct Topic {
    pub id: i64,
    pub name: Option<String>,  // Changed to Option
}

#[derive(Serialize, Deserialize)]
pub struct Venue {
    pub id: i64,
    pub name: Option<String>,  // Changed to Option
    pub address: i64,
}

// Custom deserializers
fn deserialize_string_to_area<'de, D>(deserializer: D) -> Result<HashMap<String, Area>, D::Error>
where D: Deserializer<'de> {
    let string_map: HashMap<String, String> = HashMap::deserialize(deserializer)?;
    let mut result = HashMap::new();

    for (id, name) in string_map {
        let id_num = id.parse::<i64>().unwrap_or(0);
        result.insert(id.clone(), Area {
            id: id_num,
            name: Some(name),
            parent: 0,
            child_areas: Vec::new(),
        });
    }

    Ok(result)
}

fn deserialize_string_to_audience_subcategory<'de, D>(deserializer: D) -> Result<HashMap<String, AudienceSubCategory>, D::Error>
where D: Deserializer<'de> {
    let string_map: HashMap<String, String> = HashMap::deserialize(deserializer)?;
    let mut result = HashMap::new();

    for (id, name) in string_map {
        let id_num = id.parse::<i64>().unwrap_or(0);
        result.insert(id.clone(), AudienceSubCategory {
            id: id_num,
            name: Some(name),
            parent: 0,
        });
    }

    Ok(result)
}

fn deserialize_string_to_seat_category<'de, D>(deserializer: D) -> Result<HashMap<String, SeatCategory>, D::Error>
where D: Deserializer<'de> {
    let string_map: HashMap<String, String> = HashMap::deserialize(deserializer)?;
    let mut result = HashMap::new();

    for (id, name) in string_map {
        let id_num = id.parse::<i64>().unwrap_or(0);
        result.insert(id.clone(), SeatCategory {
            id: id_num,
            name: Some(name),
            areas: Vec::new(),
        });
    }

    Ok(result)
}

fn deserialize_string_to_subtopic<'de, D>(deserializer: D) -> Result<HashMap<String, SubTopic>, D::Error>
where D: Deserializer<'de> {
    let string_map: HashMap<String, String> = HashMap::deserialize(deserializer)?;
    let mut result = HashMap::new();

    for (id, name) in string_map {
        let id_num = id.parse::<i64>().unwrap_or(0);
        result.insert(id.clone(), SubTopic {
            id: id_num,
            name: Some(name),
            parent: 0,
        });
    }

    Ok(result)
}

fn deserialize_string_to_topic<'de, D>(deserializer: D) -> Result<HashMap<String, Topic>, D::Error>
where D: Deserializer<'de> {
    let string_map: HashMap<String, String> = HashMap::deserialize(deserializer)?;
    let mut result = HashMap::new();

    for (id, name) in string_map {
        let id_num = id.parse::<i64>().unwrap_or(0);
        result.insert(id.clone(), Topic {
            id: id_num,
            name: Some(name),
        });
    }

    Ok(result)
}

fn deserialize_string_to_venue<'de, D>(deserializer: D) -> Result<HashMap<String, Venue>, D::Error>
where D: Deserializer<'de> {
    let string_map: HashMap<String, String> = HashMap::deserialize(deserializer)?;
    let mut result = HashMap::new();

    for (id, name) in string_map {
        result.insert(id.clone(), Venue {
            id: 0,
            name: Some(name),
            address: 0,
        });
    }

    Ok(result)
}

#[derive(Serialize, Deserialize, Debug)]
pub struct CitmCatalog {
    #[serde(rename = "areaNames")]
    pub area_names: HashMap<String, String>,

    #[serde(rename = "audienceSubCategoryNames")]
    pub audience_subcategory_names: HashMap<String, String>,

    #[serde(default)]
    #[serde(rename = "blockNames")]
    pub block_names: HashMap<String, String>,

    pub events: HashMap<String, Event>,

    #[serde(default)]
    pub performances: Vec<Performance>,

    #[serde(rename = "seatCategoryNames")]
    pub seat_category_names: HashMap<String, String>,

    #[serde(rename = "subTopicNames")]
    pub subtopic_names: HashMap<String, String>,

    #[serde(default)]
    #[serde(rename = "subjectNames")]
    pub subject_names: HashMap<String, String>,

    #[serde(rename = "topicNames")]
    pub topic_names: HashMap<String, String>,

    #[serde(rename = "topicSubTopics")]
    pub topic_subtopics: HashMap<String, Vec<i64>>,

    #[serde(rename = "venueNames")]
    pub venue_names: HashMap<String, String>,

    // Catch-all for other fields
    #[serde(flatten)]
    pub extra: HashMap<String, serde_json::Value>,
}

/// Creates a CitmCatalog from a JSON string (UTF-8 encoded).
#[no_mangle]
pub unsafe extern "C" fn citm_from_str(
    raw_input: *const c_char,
    raw_input_length: usize
) -> *mut CitmCatalog {
    if raw_input.is_null() {
        eprintln!("Error: Input pointer is null");
        return ptr::null_mut();
    }

    // Convert the raw pointer + length into a Rust slice
    let bytes = slice::from_raw_parts(raw_input as *const u8, raw_input_length);
    let input_str = match std::str::from_utf8(bytes) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Error: Invalid UTF-8 string: {}", e);
            return ptr::null_mut();
        }
    };

    // Try deserializing the input string into CitmCatalog
    match serde_json::from_str::<CitmCatalog>(input_str) {
        Ok(catalog) => Box::into_raw(Box::new(catalog)),
        Err(e) => {
            eprintln!("Error deserializing JSON: {}", e);
            eprintln!("JSON snippet (first 200 chars): {:.200}...", input_str);
            ptr::null_mut()
        }
    }
}

/// Serializes a CitmCatalog into a JSON string (UTF-8).
#[no_mangle]
pub unsafe extern "C" fn str_from_citm(raw_catalog: *mut CitmCatalog) -> *mut c_char {
    if raw_catalog.is_null() {
        eprintln!("Error: Catalog pointer is null");
        return ptr::null_mut();
    }

    // Fix: Actually serialize the catalog
    let catalog = &*raw_catalog;

    match serde_json::to_string(catalog) {
        Ok(serialized) => {
            match CString::new(serialized) {
                Ok(cstr) => cstr.into_raw(),
                Err(e) => {
                    eprintln!("Error creating CString: {}", e);
                    ptr::null_mut()
                }
            }
        },
        Err(e) => {
            eprintln!("Error serializing catalog to JSON: {}", e);
            ptr::null_mut()
        }
    }
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
            // Convert back into a CString, which automatically frees the memory
            let _ = CString::from_raw(ptr);
        }
    }
}