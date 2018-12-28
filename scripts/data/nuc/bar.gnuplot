set term pdfcairo fontscale 0.65 noenhanced
! python pasteandrotate.py > gbps.txt
set output "gbps.pdf"
set boxwidth 0.8
set style fill solid
set ylabel "parsing speed (GB/s)"


#set style line 80 lt rgb "#000000"

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
set style data histogram
set style histogram cluster gap 1

#set style line 1 lt rgb "#A0A0A0" lw 1 pt 1 ps 1
#set key autotitle columnhead

color1 = "#ff0c18";color2 = "#000000"; color3 = "#0c24ff";
set xtic rotate by 300 scale 1

plot "gbps.txt" using 2:xtic(1)  title "simdjson" linecolor rgb color1, \
"" using 3 title "RapidJSON" linecolor rgb color2, \
"" using 4 title "sajson" linecolor rgb color3
