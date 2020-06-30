def expand_binary(*specs)
  specs.flat_map do |spec|
    values = [0]
    spec.each_char do |ch|
      case ch
      when '0'
        values = values.map { |v| v<<1 }
      when '1'
        values = values.map { |v| (v<<1)+1 }
      when '_'
        values = values.flat_map { |v| [ v<<1, (v<<1)+1 ] }
      else
        raise "Unknown char"
      end
    end
    values
  end
end

MAX = 1<<12
possible_values = Array.new(MAX)
0.upto(MAX) do |i|
  possible_values[i] = {}
end

categories = {
  "Too Long 1" => { binary: %w{0_______10__} },
  "Too Short"  => { binary: %w{11______0___ 11______11__} },
  "5+ Byte"    => { binary: %w{11111___10__} },
  "Overlong 2" => { binary: %w{1100000_10__} },
  "Overlong 3" => { binary: %w{11100000100_} },
  "Overlong 4" => { binary: %w{111100001000} },
  "Surrogate"  => { binary: %w{11101101101_} },
  "Too Large"  => { binary: %w{111101001001 11110100101_ 1111010110__ 1111011_10__} },
  "Two Conts"  => { binary: %w{10______10__} },
}
categories.each do |(name,category)|
  category[:values] = expand_binary(*category[:binary])
  category[:values].each do |value|
    if possible_values[value][:category]
      raise "Value #{value.to_s(2)} already has error #{possible_values[value][:category]}"
    end
    possible_values[value][:category] = name;
  end
  category[:required_bits] = (0..8).map { || 0 }
  category[:bits] = (0..8).map { || 0 }
end

# high1 = [102, 102, 102, 102, 102, 102, 102, 102, 128, 128, 128, 128, 37, 1, 21, 89]
# low1  = [231, 163, 131, 131, 139, 203, 203, 203, 203, 203, 203, 203, 203, 219, 203, 203]
# high2 = [25, 25, 25, 25, 25, 25, 25, 25, 230, 174, 186, 186, 25, 25, 25, 25]
TOO_SHORT   = 1<<0
TOO_LONG    = 1<<1
OVERLONG_3  = 1<<2
SURROGATE   = 1<<4
OVERLONG_2  = 1<<5
TWO_CONTS   = 1<<7
TOO_LARGE   = 1<<3
TOO_LARGE_1000 = 1<<6
OVERLONG_4  = 1<<6
high1 = [
  TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
  TOO_LONG, TOO_LONG, TOO_LONG, TOO_LONG,
  TWO_CONTS, TWO_CONTS, TWO_CONTS, TWO_CONTS,
  TOO_SHORT | OVERLONG_2,
  TOO_SHORT,
  TOO_SHORT | OVERLONG_3 | SURROGATE,
  TOO_SHORT | TOO_LARGE | TOO_LARGE_1000 | OVERLONG_4
]
CARRY = TOO_SHORT | TOO_LONG | TWO_CONTS
low1 = [
  CARRY | OVERLONG_3 | OVERLONG_2 | OVERLONG_4,
  CARRY | OVERLONG_2,
  CARRY,
  CARRY,
  CARRY | TOO_LARGE,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000 | SURROGATE,
  CARRY | TOO_LARGE | TOO_LARGE_1000,
  CARRY | TOO_LARGE | TOO_LARGE_1000
]
high2 = [
  TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
  TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT,
  TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE_1000 | OVERLONG_4,
  TOO_LONG | OVERLONG_2 | TWO_CONTS | OVERLONG_3 | TOO_LARGE,
  TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
  TOO_LONG | OVERLONG_2 | TWO_CONTS | SURROGATE  | TOO_LARGE,
  TOO_SHORT, TOO_SHORT, TOO_SHORT, TOO_SHORT
]
0.upto(MAX-1) do |value|
  lookup = high1[(value >> 8) & 0x0F] & low1[(value >> 4) & 0x0F] & high2[value & 0x0F]
  category = possible_values[value][:category]
  if lookup != 0
    if !category
      raise "#{value.to_s(2).rjust(12, "0")} is not an error or two-cont value, but matches #{lookup.to_s(2).rjust(8, "0")}!"
    end
    possible_values[value][:bits] = (0..8).select { |bit| (lookup & (1 << bit)) != 0 }
    if possible_values[value][:bits].size == 1
      categories[category][:required_bits][possible_values[value][:bits][0]] += 1
    end
    possible_values[value][:bits].each do |bit|
      categories[category][:bits][bit] += 1
    end
  else
    if category
      raise "#{value.to_s(2).rjust(12, "0")} is a #{category}, but does not match the lookup tables!"
    end
  end
end

# We now know which bits are required for each category.

bits = (0..8).map { || { required_for: {} } }

print "| Bit "
categories.each do |(name, category)|
  print "| #{name} (#{category[:values].size})"
end
puts "|"

print "|---"
categories.each do ||
  print "|---"
end
puts "|"

0.upto(7) do |bit|
  print "| #{bit}"
  categories.each do |(name,category)|
    print " | "
    if category[:required_bits][bit] > 0
      if category[:required_bits][bit] == category[:values].size
        print "REQ "
      else
        print "REQ #{category[:required_bits][bit]} / "
      end
    end
    if category[:bits][bit] == category[:values].size
      print "FULL"
    elsif category[:bits][bit] > 0
      print "#{category[:bits][bit]}"
    end
  end
  puts " |"
end
# Find out which categories are *covered* by 
# Find which bits are *required* for each category
# 



# bits.each_with_index do |bit, bit_value|
#   if bit_value[:categories].size == 1
# end
# actual_matches.