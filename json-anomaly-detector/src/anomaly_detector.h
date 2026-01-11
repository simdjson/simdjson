#pragma once

#include <string>
#include <vector>
#include <map>
#include <deque>
#include <cmath>
#include <simdjson.h>
#include <chrono>

enum class Severity {
    INFO,
    WARNING,
    ERROR
};

enum class RuleType {
    THRESHOLD,
    ZSCORE,
    FREQUENCY
};

struct Rule {
    std::string name;
    RuleType type;
    std::string field;
    Severity severity;
    
    // Threshold specific
    std::string condition; // e.g. "> 5000"
    
    // Z-score specific
    double threshold_z;
    int window_size_seconds;
    
    // Frequency specific
    int max_count;
    
    // Internal state for Z-score
    std::deque<double> values;
    std::deque<std::chrono::system_clock::time_point> timestamps;
    
    // Internal state for Frequency
    std::deque<std::chrono::system_clock::time_point> event_timestamps;
};

struct Alert {
    std::chrono::system_clock::time_point timestamp;
    std::string rule_name;
    Severity severity;
    std::string message;
    std::string raw_json;
};

class AnomalyDetector {
public:
    AnomalyDetector();
    
    void add_rule(const Rule& rule);
    void process_document(simdjson::ondemand::document& doc, std::string_view raw_json);
    
    const std::vector<Alert>& get_alerts() const { return alerts_; }
    void clear_alerts() { alerts_.clear(); }

private:
    std::vector<Rule> rules_;
    std::vector<Alert> alerts_;
    
    void check_threshold(Rule& rule, double value, std::string_view raw_json);
    void check_zscore(Rule& rule, double value, std::string_view raw_json);
    void check_frequency(Rule& rule, std::string_view value_str, std::string_view raw_json);
    
    void trigger_alert(const Rule& rule, const std::string& message, std::string_view raw_json);
};
