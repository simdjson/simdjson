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

set key right
set style data histograms
set style histogram rowstacked
set xtic rotate by 300 scale 1

plot filename using 2 t "allocation", '' using 3 t "stage 1", '' using 4 t "stage 2", '' using 5:xtic(1) t "stage 3"
