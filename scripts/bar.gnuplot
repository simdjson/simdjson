set term pdfcairo fontscale 0.5
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
set format y "%0.1f";

set style line 1 lt rgb "#A0A0A0" lw 1 pt 1 ps 1

plot filename using 0:2:xtic(1) with boxes notitle ls 1, '' using 0:(1):(sprintf("%.2g", $2)) with labels notitle
