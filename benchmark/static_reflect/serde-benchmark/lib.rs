extern crate serde;
extern crate serde_json;
extern crate libc;
use std::slice;

use libc::{c_char, size_t};
use serde::{Serialize, Deserialize};

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
