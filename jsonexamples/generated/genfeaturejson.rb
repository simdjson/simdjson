# encoding: utf-8

class ChunkWriter
    def initialize(output_dir, miss_templates, file_size=640*1000, block_size=64)
        @@output_dir = output_dir
        @@miss_templates = miss_templates
        @@file_size = file_size
        @@block_size = block_size
    end

    def prepare_chunk(chunks, include_newline)
        Array(chunks).map do |chunk|
            "#{chunk}#{' '*(@@block_size-chunk.bytesize-1)}#{include_newline ? "\n" : " "}"
        end.join("")
    end

    def write_files(filename, start1, repeat1, end1, repeat2='', include_newline=true)
        start1  = prepare_chunk(start1, include_newline)
        repeat1 = prepare_chunk(repeat1, include_newline)
        end1    = prepare_chunk(end1, include_newline)
        write_full(File.join(@@output_dir, "#{filename}-full.json"), start1, repeat1, end1)

        repeat2 = prepare_chunk(repeat2, include_newline)
        repeat2 = repeat2 * (repeat1.bytesize/repeat2.bytesize)
        write_half(File.join(@@output_dir, "#{filename}.json"), start1, repeat1, end1, repeat2)
        write_half_miss(File.join(@@output_dir, "#{filename}-miss.json"), start1, repeat1, end1, repeat2)
    end

    def write_full(filename, start1, repeat1, end1)
        puts "Writing #{filename} ..."
        File.open(filename, "wb") do |file|
            write_chunks(file, start1, repeat1, end1, @@file_size)
        end
        raise "OMG wrong file size #{File.size(filename)} (should be #{@@file_size})" if File.size(filename) != @@file_size
    end

    def write_half(filename, start1, repeat1, end1, repeat2)
        # repeat1 is already represented in start1 and end1, so it doesn't need quite
        # half the iterations.
        repeat1_len = (@@file_size/2) - start1.bytesize - end1.bytesize
        halfway_point = start1.bytesize + repeat1_len + repeat2.bytesize

        puts "Writing #{filename} ..."
        File.open(filename, "wb") do |file|
            write_chunks(file, start1,  repeat1, repeat2, halfway_point)
            write_chunks(file, repeat2, repeat2, end1,    @@file_size-halfway_point)
        end
        raise "OMG wrong file size #{File.size(filename)} (should be #{@@file_size})" if File.size(filename) != @@file_size
    end

    def write_half_miss(filename, start1, repeat1, end1, repeat2)
        miss_template = Array(File.read(File.join(@@miss_templates, "#{repeat1.bytesize}.txt")).chomp.split("\n"))
        # Take the start and end out of the template
        repeat_template = miss_template[(start1.bytesize/64)..(-end1.bytesize/64-1)]
        # If repeat is 128 bytes, each *pair* of elements is set. Use that.
        repeat_chunks = repeat1.bytesize/64
        repeat_template = (repeat_chunks - 1).step(repeat_template.size - 1, repeat_chunks).map { |i| repeat_template[i] }

        puts "Writing #{filename} ..."
        File.open(filename, "wb") do |file|
            file.write(start1)
            repeat_template.each do |should_repeat|
                file.write(should_repeat == "1" ? repeat1 : repeat2)
            end
            file.write(end1)
        end
        raise "OMG wrong file size #{File.size(filename)} (should be #{@@file_size})" if File.size(filename) != @@file_size
    end

    def write_chunks(file, start1, repeat1, end1, size)
        pos = 0
        file.write(start1)
        pos += start1.bytesize

        repeat_end = size-end1.bytesize
        loop do
            file.write(repeat1)
            pos += repeat1.bytesize
            break if pos >= repeat_end
        end

        file.write(end1)
        pos += end1.bytesize
        return pos
    end
end

output_dir = ARGV[0] || File.expand_path(".", File.dirname(__FILE__))
miss_templates = File.expand_path("miss-templates", File.dirname(__FILE__))
Dir.mkdir(output_dir) unless File.directory?(output_dir)
w = ChunkWriter.new(output_dir, miss_templates)
w.write_files "utf-8",          '["֏","֏",{}', ',"֏","֏",{}', ',"֏","֏","֏"]', ',"ab","ab",{}'
w.write_files "escape",         '["\\"","\\"",{}', ',"\\"","\\"",{}', ',"\\"","\\"","\\""]', ',"ab","ab",{}'
w.write_files "0-structurals",  '"ab"', '',  ''
# w.write_files "1-structurals",  [ '[', '"ab"' ], [ ',', '"ab"' ], [ ',', '{', '}', ']' ]
# w.write_files "2-structurals",  '["ab"', ',"ab"', [',{', '}]']
# w.write_files "3-structurals",  '[{}', ',{}', ',"ab"]'
# w.write_files "4-structurals",  '["ab","ab"', ',"ab","ab"', ',{}]'
# w.write_files "5-structurals",  '["ab",{}', ',"ab",{}', ',"ab","ab"]'
# w.write_files "6-structurals",  '["ab","ab","ab"', ',"ab","ab","ab"', ',"ab",{}]'
w.write_files "7-structurals",  '["ab","ab",{}', ',"ab","ab",{}', ',"ab","ab","ab"]'
# w.write_files "8-structurals",  '["ab","ab","ab","ab"', ',"ab","ab","ab","ab"', ',"ab","ab",{}]'
# w.write_files "9-structurals",  '["ab","ab","ab",{}', ',"ab","ab","ab",{}', ',"ab","ab","ab","ab"]'
# w.write_files "10-structurals", '["ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab"', ',"ab","ab","ab",{}]'
# w.write_files "11-structurals", '["ab","ab","ab","ab",{}', ',"ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab"]'
# w.write_files "12-structurals", '["ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab",{}]'
# w.write_files "13-structurals", '["ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab"]'
# w.write_files "14-structurals", '["ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab",{}]'
w.write_files "15-structurals", '["ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab"]'
# w.write_files "16-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab",{}]'
# w.write_files "17-structurals", '["ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab"]'
# w.write_files "18-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab",{}]'
# w.write_files "19-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab"]'
# w.write_files "20-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab","ab",{}]'
# w.write_files "21-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab","ab"]'
# w.write_files "22-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab","ab","ab"', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab",{}]'
w.write_files "23-structurals", '["ab","ab","ab","ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab","ab",{}', ',"ab","ab","ab","ab","ab","ab","ab","ab","ab","ab","ab"]'
