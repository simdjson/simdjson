use std::fs;

// Include the lib.rs content directly
include!("../lib.rs");

fn main() {
    // Read the Twitter JSON file
    let json_str = fs::read_to_string("/Users/random_person/Desktop/simdjson/build/jsonexamples/twitter.json")
        .expect("Failed to read file");

    // Parse it
    let data: TwitterData = serde_json::from_str(&json_str)
        .expect("Failed to parse JSON");

    // Serialize it back
    let output = serde_json::to_string(&data)
        .expect("Failed to serialize");

    // Write to file for comparison
    fs::write("rust_output.json", &output)
        .expect("Failed to write output");

    println!("Output size: {} bytes", output.len());
    println!("Written to rust_output.json");

    // Also write pretty version for easier inspection
    let pretty = serde_json::to_string_pretty(&data)
        .expect("Failed to serialize pretty");
    fs::write("rust_output_pretty.json", &pretty)
        .expect("Failed to write pretty output");
}