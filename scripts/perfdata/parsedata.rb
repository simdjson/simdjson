#
# Usage: ruby perfdata.rb <perf output from master> <new text file>
#
# Typically, you pass in the output from `benchmark/parse jsonexamples/*.json` from master, and
# from another PR.
#
# Change this to :stage1 or :stage2 to print tables *just* for stage 1 or stage 2.
#

STAGE = :all

#
# Parses one or more performance outputs of this form:
#
# jsonexamples/twitter.json
# =========================
#      9867 blocks -     631515 bytes - 55263 structurals (  8.8 %)
# special blocks with: utf8      2284 ( 23.1 %) - escape       598 (  6.1 %) - 0 structurals      1287 ( 13.0 %) - 1+ structurals      8581 ( 87.0 %) - 8+ structurals      3272 ( 33.2 %) - 16+ structurals         0 (  0.0 %)
# special block flips: utf8      1104 ( 11.2 %) - escape       642 (  6.5 %) - 0 structurals       940 (  9.5 %) - 1+ structurals       940 (  9.5 %) - 8+ structurals      2593 ( 26.3 %) - 16+ structurals         0 (  0.0 %)
#
# All Stages
# |    Speed        :  25.7622 ns per block ( 99.34%) -   0.4026 ns per byte -   4.6002 ns per structural -   2.4841 GB/s
# |    Cycles       :  95.1722 per block    ( 99.41%) -   1.4872 per byte    -  16.9944 per structural    -    3.694 GHz est. frequency
# |    Instructions : 306.5807 per block    (100.00%) -   4.7906 per byte    -  54.7444 per structural    -    3.221 per cycle
# |    Misses       :    2570 branch misses ( 93.99%) - 0 cache misses (  0.00%) - 84115.00 cache references
# |- Stage 1
# |    Speed        :  12.1804 ns per block ( 46.97%) -   0.1903 ns per byte -   2.1750 ns per structural -   5.2540 GB/s
# |    Cycles       :  44.9869 per block    ( 46.99%) -   0.7030 per byte    -   8.0331 per structural    -    3.693 GHz est. frequency
# |    Instructions : 156.8147 per block    ( 51.15%) -   2.4504 per byte    -  28.0015 per structural    -    3.486 per cycle
# |    Misses       :    1158 branch misses ( 42.35%) - 0 cache misses (  0.00%) - 29346.00 cache references
# |- Stage 2
# |    Speed        :  13.4511 ns per block ( 51.87%) -   0.2102 ns per byte -   2.4019 ns per structural -   4.7577 GB/s
# |    Cycles       :  49.6711 per block    ( 51.88%) -   0.7762 per byte    -   8.8695 per structural    -    3.693 GHz est. frequency
# |    Instructions : 149.5415 per block    ( 48.78%) -   2.3367 per byte    -  26.7028 per structural    -    3.011 per cycle
# |    Misses       :    1313 branch misses ( 48.02%) - 0 cache misses (  0.00%) - 54446.00 cache references
#

def read_output(filename)
  last_line = nil
  files = []
  file = nil
  stage = nil
  File.open(filename).each_line do |line|
    case line
    when /^=+\s*$/
      files.push(file) unless file.nil?
      last_line = last_line.chomp
      last_line = last_line.sub(/^jsonexamples\//, "")
      last_line = last_line.sub(/\.json$/, "")
      file = {
        name: last_line.chomp
      }
    when /^\s*(\d+)\s+blocks\s+-\s*(\d+)\s+bytes\s+-\s*(\d+)\s+structurals/
      file[:blocks] = $1
      file[:bytes] = $2
      file[:structurals] = $3
    when /^\s*All Stages\s*$/
      stage = file[:all] = {}
    when /^\s*\|-\s*Allocation\s*$/
      stage = file[:allocation] = {}
    when /^\s*\|-\s*Stage 1\s*$/
      stage = file[:stage1] = {}
    when /^\s*\|-\s*Stage 2\s*$/
      stage = file[:stage2] = {}
    when /^\|\s*(\w+)\s*:\s*([0-9.]+) branch misses [^-]*-\s*([0-9.]+) cache misses [^-]*-\s*([0-9.]+) cache references\s*$/
      stage["Branch Misses"] = $2.to_f
      stage["Cache Misses"] = $3.to_f
      stage["Cache References"] = $4.to_f
    when /^\|\s*(\w+)\s*:\s*([0-9.]+).+per block.+$/
      stage[$1] = $2.to_f
    end
    last_line = line
  end
  files.push(file) unless file.nil?
  return files
end

def percent(old_val, new_val)
  if old_val == 0 || new_val == 0
    return 0
  end
  (new_val - old_val) / old_val * 100
end
old_files = read_output(ARGV[0])
new_files = read_output(ARGV[1])

puts "| %-35s | %5s | %5s | %5s | %5s | %5s | %5s | %6s | %6s | %5s | %6s | %6s | %5s | %6s | %6s | %5s |" % [
  "File",
  "master Cycles", "PR Cycles", "+Throughput",
  "master Instr.", "PR Instr.", "-Instr.",
  "master Branch Misses", "PR Branch Misses", "-Branch Misses",
  "master Cache Misses", "PR Cache Misses", "-Cache Misses",
  "master Cache Refs", "PR Cache Refs", "-Cache Refs",
]
puts "| %-35s | %5s | %5s | %5s | %5s | %5s | %5s | %6s | %6s | %5s | %6s | %6s | %5s | %6s | %6s | %5s |" % [
  "---",
  "--:", "--:", "--:",
  "--:", "--:", "--:",
  "--:", "--:", "--:",
  "--:", "--:", "--:",
  "--:", "--:", "--:",
]

results = old_files.zip(new_files).map do |old_file, new_file|
  if old_file[:name] != new_file[:name]
    raise "Files different! #{old_file[:name]}, #{new_file[:name]}"
  end
  old_stage = old_file[STAGE]
  new_stage = new_file[STAGE]
  {
    name: old_file[:name],
    old: old_stage,
    new: new_stage
  }
end
results.sort_by { |x| -percent(x[:old]["Cycles"], x[:new]["Cycles"]) }.reverse.each do |result|
  puts "| %-35s | %5.1f | %5.1f | %4d%% | %5.1f | %5.1f | %4d%% | %6d | %6d | %4d%% | %6d | %6d | %4d%% | %6d | %6d | %4d%% |" % [
    result[:name],
    result[:old]["Cycles"], result[:new]["Cycles"], -percent(result[:old]["Cycles"], result[:new]["Cycles"]),
    result[:old]["Instructions"], result[:new]["Instructions"], -percent(result[:old]["Instructions"], result[:new]["Instructions"]),
    result[:old]["Branch Misses"], result[:new]["Branch Misses"], -percent(result[:old]["Branch Misses"], result[:new]["Branch Misses"]),
    result[:old]["Cache Misses"], result[:new]["Cache Misses"], -percent(result[:old]["Cache Misses"], result[:new]["Cache Misses"]),
    result[:old]["Cache References"], result[:new]["Cache References"], -percent(result[:old]["Cache References"], result[:new]["Cache References"])
  ]
end
