use std::fs;

// Import from the parent lib.rs
include!("../lib.rs");

fn main() {
    // Read the Twitter JSON file
    let json_str = fs::read_to_string("/Users/random_person/Desktop/simdjson/build/jsonexamples/twitter.json")
        .expect("Failed to read file");

    // Parse it
    let data: TwitterData = serde_json::from_str(&json_str)
        .expect("Failed to parse JSON");

    // Serialize it back (compact)
    let output = serde_json::to_vec(&data)
        .expect("Failed to serialize");
    
    let output_str = String::from_utf8(output.clone()).unwrap();

    // Write to file for comparison
    fs::write("rust_output_test.json", &output)
        .expect("Failed to write output");

    println!("Output size: {} bytes", output.len());
    
    // Count statuses
    println!("Number of statuses: {}", data.statuses.len());
    
    // Check what fields are in the first status
    if let Some(first) = data.statuses.first() {
        // Let's serialize just the first status to see what fields are included
        let first_json = serde_json::to_string_pretty(first).unwrap();
        println!("First status (pretty):\n{}", first_json);
    }
}
