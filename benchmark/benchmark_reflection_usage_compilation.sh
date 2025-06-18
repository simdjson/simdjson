#!/bin/bash

# JSON Parsing Compilation Benchmark: Reflection Usage vs Manual Parsing
# Compares compilation times when ACTUALLY USING reflection for parsing vs manual parsing
# This measures the compile-time cost of reflection-based automatic deserialization

set -e

echo "=== simdjson Reflection Usage Compilation Benchmark ==="
echo "Measuring compilation impact of ACTUALLY USING reflection for parsing"
echo "Starting at: $(date)"
echo
echo "╔════════════════════════════════════════════════════════════════════════════╗"
echo "║                              METHODOLOGY                                   ║"
echo "╠════════════════════════════════════════════════════════════════════════════╣"
echo "║                                                                            ║"
echo "║ FAIR COMPARISON STRATEGY:                                                  ║"
echo "║ This benchmark compares two DIFFERENT approaches to parsing the same JSON: ║"
echo "║                                                                            ║"
echo "║ • MANUAL PARSING: Traditional simdjson with explicit .get() calls          ║"
echo "║   - Uses doc[\"field\"].get(variable) for each field                         ║"
echo "║   - No reflection involved                                                 ║"
echo "║                                                                            ║"
echo "║ • REFLECTION PARSING: Automatic deserialization with reflection            ║"
echo "║   - Uses doc.get<MyStruct>() for automatic field mapping                   ║"
echo "║   - Relies on compile-time reflection to generate parsing code             ║"
echo "║                                                                            ║"
echo "║ WHAT WE'RE MEASURING:                                                      ║"
echo "║ • Compile-time cost of reflection-based automatic deserialization          ║"
echo "║ • Template instantiation overhead for reflection parsing                   ║"
echo "║ • Code generation complexity from using reflection features                ║"
echo "║                                                                            ║"
echo "║ TEST SCENARIOS:                                                            ║"
echo "║                                                                            ║"
echo "║ 1. SIMPLE STRUCT: Basic fields (string, int, bool)                         ║"
echo "║    - Measures baseline reflection overhead                                 ║"
echo "║                                                                            ║"
echo "║ 2. NESTED STRUCT: Multiple levels of nested objects                        ║"
echo "║    - Measures reflection complexity scaling                                ║"
echo "║                                                                            ║"
echo "║ 3. COMPLEX STRUCT: Arrays, optional fields, mixed types                    ║"
echo "║    - Measures real-world reflection usage impact                           ║"
echo "║                                                                            ║"
echo "║ WHY THIS IS MEANINGFUL:                                                    ║"
echo "║ • Shows actual cost of using reflection features                           ║"
echo "║ • Measures compile-time code generation overhead                           ║"
echo "║ • Helps developers understand reflection's compilation impact              ║"
echo "║ • Compares equivalent functionality implemented two different ways         ║"
echo "║                                                                            ║"
echo "╚════════════════════════════════════════════════════════════════════════════╝"
echo

# Configuration
ITERATIONS=50
JOBS=4

# ───────────────────────────── BOX-PRINT HELPER ────────────────────────────
BOX_WIDTH=74                               # characters between the pipes
print_box_line() {                         # usage: print_box_line "text"
  printf "║ %-*s ║\n" "${BOX_WIDTH}" "$1"
}

# Make sure we use the reflection-enabled clang
export CXX=/usr/local/bin/clang++
export CC=/usr/local/bin/clang

echo "Using compiler: $(/usr/local/bin/clang++ --version | head -n1)"
echo

# Function to create manual parsing test
create_manual_parsing_test() {
    local test_name="$1"
    local struct_complexity="$2"

    cat > "${test_name}_manual.cpp" << 'EOF'
#include <simdjson.h>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

// Test structures
struct Person {
    std::string name;
    int age;
    bool active;
};

struct Address {
    std::string street;
    std::string city;
    int zipcode;
};

struct Employee {
    Person person;
    Address address;
    std::vector<std::string> skills;
    std::optional<std::string> department;
    double salary;
};

// Manual parsing functions
bool parse_person_manual(simdjson::ondemand::value& val, Person& person) {
    auto obj = val.get_object();
    if (obj.error()) return false;

    for (auto field : obj) {
        std::string_view key = field.unescaped_key();
        if (key == "name") {
            std::string_view name_val;
            if (field.value().get(name_val)) return false;
            person.name = name_val;
        } else if (key == "age") {
            if (field.value().get(person.age)) return false;
        } else if (key == "active") {
            if (field.value().get(person.active)) return false;
        }
    }
    return true;
}

bool parse_address_manual(simdjson::ondemand::value& val, Address& address) {
    auto obj = val.get_object();
    if (obj.error()) return false;

    for (auto field : obj) {
        std::string_view key = field.unescaped_key();
        if (key == "street") {
            std::string_view street_val;
            if (field.value().get(street_val)) return false;
            address.street = street_val;
        } else if (key == "city") {
            std::string_view city_val;
            if (field.value().get(city_val)) return false;
            address.city = city_val;
        } else if (key == "zipcode") {
            if (field.value().get(address.zipcode)) return false;
        }
    }
    return true;
}

bool parse_employee_manual(simdjson::ondemand::document& doc, Employee& employee) {
    auto obj = doc.get_object();
    if (obj.error()) return false;

    for (auto field : obj) {
        std::string_view key = field.unescaped_key();
        if (key == "person") {
            auto person_val = field.value();
            if (!parse_person_manual(person_val, employee.person)) return false;
        } else if (key == "address") {
            auto addr_val = field.value();
            if (!parse_address_manual(addr_val, employee.address)) return false;
        } else if (key == "skills") {
            auto skills_array = field.value().get_array();
            if (skills_array.error()) return false;
            for (auto skill : skills_array) {
                std::string_view skill_val;
                if (skill.get(skill_val)) return false;
                employee.skills.emplace_back(skill_val);
            }
        } else if (key == "department") {
            std::string_view dept_val;
            if (!field.value().get(dept_val)) {
                employee.department = dept_val;
            }
        } else if (key == "salary") {
            if (field.value().get(employee.salary)) return false;
        }
    }
    return true;
}

int main() {
    simdjson::ondemand::parser parser;
    std::string json_str = R"({
        "person": {
            "name": "John Doe",
            "age": 30,
            "active": true
        },
        "address": {
            "street": "123 Main St",
            "city": "Anytown",
            "zipcode": 12345
        },
        "skills": ["C++", "JSON", "Programming"],
        "department": "Engineering",
        "salary": 85000.50
    })";

    simdjson::ondemand::document doc;
    auto error = parser.iterate(simdjson::pad(json_str)).get(doc);
    if (error) {
        std::cerr << "Parse error" << std::endl;
        return 1;
    }

    Employee employee;
    if (!parse_employee_manual(doc, employee)) {
        std::cerr << "Manual parsing failed" << std::endl;
        return 1;
    }

    std::cout << "Manual parsing successful: " << employee.person.name
              << ", age " << employee.person.age << std::endl;
    return 0;
}
EOF
}

# Function to create reflection parsing test
create_reflection_parsing_test() {
    local test_name="$1"
    local struct_complexity="$2"

    cat > "${test_name}_reflection.cpp" << 'EOF'
#include <simdjson.h>
#include <iostream>
#include <string>
#include <vector>
#include <optional>

// Test structures (same as manual version)
struct Person {
    std::string name;
    int age;
    bool active;
};

struct Address {
    std::string street;
    std::string city;
    int zipcode;
};

struct Employee {
    Person person;
    Address address;
    std::vector<std::string> skills;
    std::optional<std::string> department;
    double salary;
};

int main() {
    simdjson::ondemand::parser parser;
    std::string json_str = R"({
        "person": {
            "name": "John Doe",
            "age": 30,
            "active": true
        },
        "address": {
            "street": "123 Main St",
            "city": "Anytown",
            "zipcode": 12345
        },
        "skills": ["C++", "JSON", "Programming"],
        "department": "Engineering",
        "salary": 85000.50
    })";

    simdjson::ondemand::document doc;
    auto error = parser.iterate(simdjson::pad(json_str)).get(doc);
    if (error) {
        std::cerr << "Parse error" << std::endl;
        return 1;
    }

    // Use reflection-based automatic deserialization
    Employee employee;
    auto result = doc.get<Employee>();
    if (result.error()) {
        std::cerr << "Reflection parsing failed" << std::endl;
        return 1;
    }
    employee = result.value();

    std::cout << "Reflection parsing successful: " << employee.person.name
              << ", age " << employee.person.age << std::endl;
    return 0;
}
EOF
}

# Function to time compilation of parsing approach
time_parsing_compilation() {
    local description="$1"
    local test_file="$2"
    local use_reflection="$3"
    local iteration="$4"

    echo "[$iteration] $description"

    # Clean build
    rm -rf build_parsing_test
    mkdir build_parsing_test
    cd build_parsing_test

    # Copy test file
    cp "../$test_file" .

    echo "  Configuring..."
    if [ "$use_reflection" = "true" ]; then
        cmake -DCMAKE_CXX_COMPILER=clang++ \
              -DSIMDJSON_DEVELOPER_MODE=ON \
              -DSIMDJSON_STATIC_REFLECTION=ON \
              -DBUILD_SHARED_LIBS=OFF \
              ../.. >/dev/null 2>&1
    else
        cmake -DCMAKE_CXX_COMPILER=clang++ \
              -DSIMDJSON_DEVELOPER_MODE=ON \
              -DSIMDJSON_STATIC_REFLECTION=OFF \
              -DBUILD_SHARED_LIBS=OFF \
              ../.. >/dev/null 2>&1
    fi

    echo "  Building simdjson..."
    cmake --build . --target simdjson >/dev/null 2>&1

    echo "  Compiling parsing test..."
    # Time just the test compilation
    start_time=$(date +%s.%N)

    /usr/local/bin/clang++ -std=c++17 -I../../include "$test_file" -L. -lsimdjson -o parsing_test >/dev/null 2>&1

    end_time=$(date +%s.%N)

    # Calculate time duration
    time_taken=$(echo "$end_time $start_time" | awk '{printf "%.3f", $1 - $2}')
    echo "  Completed in: ${time_taken}s"

    cd ..
    rm -rf build_parsing_test

    echo "$time_taken"
}

# Create test files
echo "Creating test files..."
create_manual_parsing_test "complex" "complex"
create_reflection_parsing_test "complex" "complex"

# Arrays to store times
times_manual=""
times_reflection=""

echo
echo "=== MANUAL PARSING COMPILATION ==="
echo "Testing traditional simdjson parsing with explicit .get() calls"
echo

for i in $(seq 1 $ITERATIONS); do
    time_result=$(time_parsing_compilation "Compiling manual parsing test" "complex_manual.cpp" "false" "$i" | tail -n1)
    times_manual="$times_manual $time_result"
done

echo
echo "=== REFLECTION PARSING COMPILATION ==="
echo "Testing automatic deserialization with doc.get<Struct>()"
echo

for i in $(seq 1 $ITERATIONS); do
    time_result=$(time_parsing_compilation "Compiling reflection parsing test" "complex_reflection.cpp" "true" "$i" | tail -n1)
    times_reflection="$times_reflection $time_result"
done

echo
echo "╔════════════════════════════════════════════════════════════════════════════╗"
echo "║           REFLECTION USAGE COMPILATION RESULTS                             ║"
echo "╠════════════════════════════════════════════════════════════════════════════╣"
echo "║                                                                            ║"
echo "║ MANUAL PARSING (explicit .get() calls):                                    ║"
count=1
for t in $times_manual; do
    if [[ "$t" =~ ^[0-9]+\.?[0-9]*$ ]]; then
        line=$(printf "Run %2d: %7.3f seconds" "$count" "$t")
        print_box_line "$line"
        count=$((count + 1))
    fi
done
echo "║                                                                            ║"
echo "║ REFLECTION PARSING (automatic doc.get<Struct>()):                          ║"
count=1
for t in $times_reflection; do
    if [[ "$t" =~ ^[0-9]+\.?[0-9]*$ ]]; then
        line=$(printf "Run %2d: %7.3f seconds" "$count" "$t")
        print_box_line "$line"
        count=$((count + 1))
    fi
done
echo "╚════════════════════════════════════════════════════════════════════════════╝"

echo
echo "╔════════════════════════════════════════════════════════════════════════════╗"
echo "║                              ANALYSIS SUMMARY                              ║"
echo "╠════════════════════════════════════════════════════════════════════════════╣"
echo "║                                                                            ║"

# Calculate averages and percentages - filter to only numeric values first
manual_numbers=""
reflection_numbers=""

for t in $times_manual; do
    if [[ "$t" =~ ^[0-9]+\.?[0-9]*$ ]]; then
        manual_numbers="$manual_numbers $t"
    fi
done

for t in $times_reflection; do
    if [[ "$t" =~ ^[0-9]+\.?[0-9]*$ ]]; then
        reflection_numbers="$reflection_numbers $t"
    fi
done

if [ -n "$manual_numbers" ] && [ -n "$reflection_numbers" ]; then
    manual_avg=$(echo "$manual_numbers" | awk '{sum=0; for(i=1;i<=NF;i++) sum+=$i; print sum/NF}')
    reflection_avg=$(echo "$reflection_numbers" | awk '{sum=0; for(i=1;i<=NF;i++) sum+=$i; print sum/NF}')
    overhead=$(echo "$reflection_avg $manual_avg" | awk '{printf "%.3f", $1 - $2}')

    if [ $(echo "$manual_avg > 0" | awk '{print ($1 > 0)}') -eq 1 ]; then
        percent=$(echo "$reflection_avg $manual_avg" | awk '{printf "%.1f", ($1 - $2) / $2 * 100}')
    else
        percent="0"
    fi

    print_box_line "MANUAL PARSING RESULTS:"
    print_box_line "$(printf "Average compilation time: %.3fs" "$manual_avg")"
    print_box_line ""
    print_box_line "REFLECTION PARSING RESULTS:"
    print_box_line "$(printf "Average compilation time: %.3fs" "$reflection_avg")"
    print_box_line ""
    print_box_line "REFLECTION OVERHEAD:"
    print_box_line "$(printf "Additional time: %.3fs (%+.1f%%)" "$overhead" "$percent")"
    print_box_line ""
else
    echo "║ ERROR: Could not extract valid timing data                                 ║"
    echo "║ Manual times: $times_manual"
    echo "║ Reflection times: $times_reflection"
    echo "║                                                                            ║"
fi

echo "╠════════════════════════════════════════════════════════════════════════════╣"
echo "║                              INTERPRETATION                                ║"
echo "╠════════════════════════════════════════════════════════════════════════════╣"
echo "║                                                                            ║"
echo "║ WHAT THESE RESULTS SHOW:                                                   ║"
echo "║                                                                            ║"
echo "║ • COMPILE-TIME COST: How much longer reflection parsing takes to compile   ║"
echo "║   - Higher % = more expensive template instantiation and codegen           ║"
echo "║                                                                            ║"
echo "║ • CODE GENERATION OVERHEAD: Reflection creates parsing code at compile     ║"
echo "║   time, which requires more template processing than manual parsing        ║"
echo "║                                                                            ║"
echo "║ • DEVELOPER TRADE-OFF: Reflection provides automatic deserialization       ║"
echo "║   but at the cost of increased compilation time                            ║"
echo "║                                                                            ║"
echo "║ EVALUATION:                                                                ║"
echo "║ • Low overhead (0-20%): Reflection is compile-time efficient               ║"
echo "║ • Medium overhead (20-50%): Noticeable but potentially acceptable          ║"
echo "║ • High overhead (50%+): Significant compilation cost for reflection        ║"
echo "║                                                                            ║"
echo "║ REAL-WORLD IMPACT:                                                         ║"
echo "║ • Small projects: Absolute time matters more than percentage               ║"
echo "║ • Large projects: Percentage overhead compounds across many files          ║"
echo "║ • CI/CD pipelines: Longer builds affect development velocity               ║"
echo "║                                                                            ║"
echo "╚════════════════════════════════════════════════════════════════════════════╝"

# Clean up test files
rm -f complex_manual.cpp complex_reflection.cpp

echo
echo "Completed at: $(date)"