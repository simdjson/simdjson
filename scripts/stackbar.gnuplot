set term pdfcairo fontscale 0.7 noenhanced
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
#set key right top;
set key  samplen 1 spacing 1.5 # font ",12"
set key width -2 
set key maxrows 4
set key noenhanced

set key Left  at 14,4.2 #at 5.5,4
#set key right top
#set key outside
set key invert
set style data histograms
set style histogram rowstacked
set xtic rotate by 300 scale 1

set style line 1 lt rgb "#A00000" 
set style line 2 lt rgb "#00A000"
set style line 3 lt rgb "orange" 
set style line 4 lt rgb "#5060D0"
set style line 5 lt rgb "#0099FB"# "#00008B"
set style line 6 lt rgb "black" 
set style line 7 lt rgb "blue" lw 1 pt 1 ps 1
set style line 8 lt rgb "#5060D0" lw 1 pt 1 ps 1

clamp(a) = (a < 0) ? 0 : a

# plot filename using 3 t "stage 1" ls 2, '' using 4 t "stage 2" ls 3, '' using 5:xtic(1) t "stage 3" ls 1
plot filename using 9 t "1: no utf8" ls 1,\
 '' using (clamp($3-$9))  t "1: just utf8" ls 2,\
 '' using 4 t "stage 2" ls 3,\
 '' using (clamp($23 + $17 - $5)) t "3: core" ls 4,\
 '' using (clamp($5 - $23)) t "3: strings" ls 5,\
 '' using (clamp($5 - $17)):xtic(1) t "3: numbers" ls 6



#plot filename using (clamp($5 - $23)):xtic(1)  t "stage 3: just string parsing" ls 5
# 1, 2 mem, 3 st1, 4 st2, 5  st3, 6 total
# 7, 8 mem, 9 st1, 10 st2, 11  st3, 12 total // noutf8
# 13, 14 mem, 15 st1, 16 st2, 17  st3, 18 total // nonumber
# 19, 20 mem, 21 st1, 22 st2, 23  st3, 24 total // nostring
# string: $5 - $23 , number $5 - $17, no string no number $5 - ($5 - $20) - ($5 - $15) = $23 + $17 - $5
