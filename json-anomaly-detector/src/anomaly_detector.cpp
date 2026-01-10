#include "anomaly_detector.h"
#include <iostream>
#include <sstream>
#include <numeric>

AnomalyDetector::AnomalyDetector() {}

void AnomalyDetector::add_rule(const Rule& rule) {
    rules_.push_back(rule);
}

void AnomalyDetector::process_document(simdjson::ondemand::document& doc, std::string_view raw_json) {
    for (auto& rule : rules_) {
        // Extract field value
        // Note: This is a simplification. Nested fields need more logic.
        // Also, we need to rewind the document or use a fresh iterator if we access multiple fields?
        // simdjson::ondemand is forward-only. If rules access different fields, 
        // we might need to parse the document into a DOM or structure if we can't control the order.
        // However, for high performance, we should try to iterate once.
        // But here we have a list of rules, each wanting a field.
        // If we use ondemand, we can't easily jump around.
        // 
        // Strategy: 
        // 1. If we have multiple rules on different fields, we might need to iterate the object fields
        //    and check against rules.
        // 2. Or we parse to a temporary struct if the schema is known.
        // 3. Or we use simdjson::dom (slower but random access).
        //
        // Given the requirement for high performance, we should try to iterate the object.
        
        // For this implementation, let's assume we iterate the object and check rules.
        // But since we are passing `doc` which is the document root, we can try to find the field.
        // But `find_field` advances the iterator.
        
        // To support arbitrary rules on arbitrary fields efficiently with ondemand, 
        // we should iterate over the object's fields and match them against our rules.
        
        // However, `doc` is a document, which might be an object or array.
        // Let's assume root is an object.
    }
    
    // Correct approach for ondemand with multiple fields:
    // Iterate over the object fields once.
    
    simdjson::ondemand::object obj;
    auto error = doc.get_object().get(obj);
    if (error) return;

    // Map field names to rules for faster lookup?
    // Or just iterate fields and check all rules.
    
    for (auto field : obj) {
        std::string_view key = field.key_raw();
        
        for (auto& rule : rules_) {
            if (rule.field == key) {
                // Found a matching field for a rule
                if (rule.type == RuleType::THRESHOLD || rule.type == RuleType::ZSCORE) {
                    double value;
                    if (field.value().get(value) == simdjson::SUCCESS) {
                        if (rule.type == RuleType::THRESHOLD) {
                            check_threshold(rule, value, raw_json);
                        } else {
                            check_zscore(rule, value, raw_json);
                        }
                    }
                } else if (rule.type == RuleType::FREQUENCY) {
                    // Frequency might be on string values or just existence
                    // Let's assume string value for now
                    std::string_view value_str;
                    // Try string, if not, maybe convert number to string?
                    // For now, only support string fields for frequency
                    if (field.value().get(value_str) == simdjson::SUCCESS) {
                        check_frequency(rule, value_str, raw_json);
                    }
                }
            }
        }
    }
}

void AnomalyDetector::check_threshold(Rule& rule, double value, std::string_view raw_json) {
    // Simple parsing of condition like "> 5000"
    // This is very basic.
    std::stringstream ss(rule.condition);
    std::string op;
    double threshold;
    ss >> op >> threshold;
    
    bool triggered = false;
    if (op == ">") triggered = value > threshold;
    else if (op == "<") triggered = value < threshold;
    else if (op == ">=") triggered = value >= threshold;
    else if (op == "<=") triggered = value <= threshold;
    else if (op == "==") triggered = value == threshold;
    
    if (triggered) {
        std::string msg = "Value " + std::to_string(value) + " " + rule.condition;
        trigger_alert(rule, msg, raw_json);
    }
}

void AnomalyDetector::check_zscore(Rule& rule, double value, std::string_view raw_json) {
    auto now = std::chrono::system_clock::now();
    
    // Remove old values
    while (!rule.timestamps.empty()) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - rule.timestamps.front()).count();
        if (diff > rule.window_size_seconds) {
            rule.timestamps.pop_front();
            rule.values.pop_front();
        } else {
            break;
        }
    }
    
    // Add new value
    rule.timestamps.push_back(now);
    rule.values.push_back(value);
    
    // Need enough data points
    if (rule.values.size() < 10) return;
    
    // Calculate Mean
    double sum = std::accumulate(rule.values.begin(), rule.values.end(), 0.0);
    double mean = sum / rule.values.size();
    
    // Calculate StdDev
    double sq_sum = std::inner_product(rule.values.begin(), rule.values.end(), rule.values.begin(), 0.0);
    double stdev = std::sqrt(sq_sum / rule.values.size() - mean * mean);
    
    if (stdev == 0) return;
    
    double zscore = (value - mean) / stdev;
    
    if (std::abs(zscore) > rule.threshold_z) {
        std::string msg = "Z-Score " + std::to_string(zscore) + " > " + std::to_string(rule.threshold_z);
        trigger_alert(rule, msg, raw_json);
    }
}

void AnomalyDetector::check_frequency(Rule& rule, std::string_view value_str, std::string_view raw_json) {
    // This implementation is simplified. 
    // Real frequency analysis might need to track counts per value.
    // Here we just track event frequency for the rule (e.g. "error" logs)
    // If the rule matches (field exists and maybe value matches), we count it.
    
    // For now, let's assume if we are here, the rule matched the field.
    // If we want to match specific value, we need that in Rule.
    
    auto now = std::chrono::system_clock::now();
    rule.event_timestamps.push_back(now);
    
    // Remove old events
    while (!rule.event_timestamps.empty()) {
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - rule.event_timestamps.front()).count();
        if (diff > rule.window_size_seconds) {
            rule.event_timestamps.pop_front();
        } else {
            break;
        }
    }
    
    if (rule.event_timestamps.size() > rule.max_count) {
        std::string msg = "Frequency " + std::to_string(rule.event_timestamps.size()) + " > " + std::to_string(rule.max_count);
        trigger_alert(rule, msg, raw_json);
    }
}

void AnomalyDetector::trigger_alert(const Rule& rule, const std::string& message, std::string_view raw_json) {
    Alert alert;
    alert.timestamp = std::chrono::system_clock::now();
    alert.rule_name = rule.name;
    alert.severity = rule.severity;
    alert.message = message;
    alert.raw_json = std::string(raw_json);
    
    alerts_.push_back(alert);
    
    // Print to stderr for now
    std::cerr << "ALERT [" << rule.name << "]: " << message << std::endl;
}
