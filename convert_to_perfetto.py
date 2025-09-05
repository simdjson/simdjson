#!/usr/bin/env python3
import sys
import json

def parse_perf_script(input_file, output_file):
    """Convert perf script output to Perfetto JSON format"""
    
    samples = []
    current_sample = None
    
    with open(input_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
                
            # New sample line
            if 'cpu-clock:pppH:' in line:
                if current_sample and current_sample['stack']:
                    samples.append(current_sample)
                    
                parts = line.split()
                timestamp = float(parts[2].rstrip(':')) * 1000000  # Convert to microseconds
                current_sample = {
                    'ts': timestamp,
                    'stack': [],
                    'name': 'cpu-clock'
                }
            # Stack frame
            elif line.startswith('\t') and current_sample:
                # Extract function name from the line
                parts = line.strip().split()
                if len(parts) >= 2:
                    func_info = parts[1]
                    # Clean up function name
                    if '+' in func_info:
                        func_name = func_info.split('+')[0]
                    else:
                        func_name = func_info
                    
                    # Skip unknown symbols
                    if func_name != '[unknown]':
                        current_sample['stack'].append(func_name)
    
    # Add last sample
    if current_sample and current_sample['stack']:
        samples.append(current_sample)
    
    # Create Perfetto trace format
    trace = {
        'traceEvents': [],
        'samples': [],
        'stacks': {}
    }
    
    # Convert to Perfetto sampling profiler format
    for i, sample in enumerate(samples):
        if sample['stack']:
            # Reverse stack for bottom-up view
            stack = list(reversed(sample['stack']))
            
            # Create a stack ID
            stack_id = str(i)
            trace['stacks'][stack_id] = stack
            
            # Add sample event
            trace['samples'].append({
                'ts': sample['ts'],
                'sf': stack_id,  # Stack frame ID
                'pid': 1,
                'tid': 1,
                'weight': 1
            })
    
    # Write JSON output
    with open(output_file, 'w') as f:
        json.dump(trace, f, indent=2)
    
    print(f"Converted {len(samples)} samples to Perfetto format")
    print(f"Output written to {output_file}")

if __name__ == "__main__":
    parse_perf_script("perf_simdjson_serialization.txt", "perf_simdjson_perfetto.json")
