def gen_seeds(start_blocks, repeat_blocks, end_blocks)
    total_size = 640*1000
    total_blocks = total_size/64
    seed_space = 1..1000000
    target_blocks = total_blocks*0.5
    target_flips = total_blocks*0.25
    percent_flips = 0.25*repeat_blocks

    puts "Seeds for #{start_blocks} start blocks, #{end_blocks} end blocks and #{repeat_blocks} repeat blocks: #{percent_flips*100}% flips"
    closest_flips = nil
    closest_seeds = []
    seed_space.each do |seed|
        r = Random.new(seed)
        # First block is always type 1
        flips = 0
        type1 = true
        type1_blocks = start_blocks
        finished_blocks = start_blocks
        last_repeat = total_blocks-end_blocks
        while finished_blocks < last_repeat
            if r.rand < percent_flips
                flips += 1
                type1 = !type1
            end
            type1_blocks += repeat_blocks if type1
            finished_blocks += repeat_blocks
        end

        # Last one is always type 1
        flips += 1 if !type1
        type1 = true
        type1_blocks += end_blocks
        finished_blocks += end_blocks

        raise "simulated the wrong number of blocks #{finished_blocks}" if finished_blocks != total_blocks

        if type1_blocks == target_blocks
            if flips == target_flips
                puts seed
                closest_seeds << seed
            end
        end
    end
    puts closest_seeds
end

gen_seeds(1,1,1)
gen_seeds(1,1,2)
gen_seeds(2,2,4)
