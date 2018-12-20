set term pdfcairo fontscale 0.5 noenhanced
set output name
set boxwidth 0.8
set style fill solid 
set ylabel "cycles per input byte"


set style line 80 lt rgb "#000000"

# Line style for grid
set style line 81 lt 0  # dashed
set style line 81 lt rgb "#808080"  # grey

set grid back linestyle 81
set border 3 back linestyle 80 # Remove border on top and right.  These
             # borders are useless and make it harder
             # to see plotted lines near the border.
    # Also, put it in grey; no need for so much emphasis on a border.
set xtics nomirror
set ytics nomirror

set yrange [0:]

#set key right
set key outside
set style data histograms
set style histogram rowstacked
set xtic rotate by 300 scale 1

set style line 1 lt rgb "#A00000" lw 1 pt 1 ps 1
set style line 2 lt rgb "#00A000" lw 1 pt 1 ps 1
set style line 3 lt rgb "#5060D0" lw 1 pt 1 ps 1
set style line 4 lt rgb "red" lw 1 pt 1 ps 1
set style line 5 lt rgb "#808000" lw 1 pt 1 ps 1
set style line 6 lt rgb "#00008B" lw 1 pt 1 ps 1
set style line 7 lt rgb "black" lw 1 pt 1 ps 1
set style line 8 lt rgb "blue" lw 1 pt 1 ps 1
set style line 9 lt rgb "violet" lw 1 pt 1 ps 1

# plot filename using 3 t "stage 1" ls 2, '' using 4 t "stage 2" ls 3, '' using 5:xtic(1) t "stage 3" ls 1
plot filename using 8 t "stage 1 without utf8 validation" ls 1, '' using ($3-$8)  t "utf8 validation (stage 1)" ls 2, '' using 4 t "stage 2" ls 3, '' using ($20 + $15 - $5) t "stage 3 (no number or string)" ls 4, '' using ($5 - $20) t "string parsing (stage 3)" ls 5,  '' using ($5 - $15):xtic(1) t "number parsing (stage 3)" ls 6



# 1, 2 mem, 3 st1, 4 st2, 5  st3
# 6, 7 mem, 8 st1, 9 st2, 10  st3 // noutf8
# 11, 12 mem, 13 st1, 14 st2, 15  st3 // nonumber
# 16, 17 mem, 18 st1, 19 st2, 20  st3 // nostring
# string: $5 - $20 , number $5 - $15, no string no number $5 - ($5 - $20) - ($5 - $15) = $20 + $15 - $5
