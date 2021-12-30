set title 'Output video sizes (5 seconds)'
set ylabel "Size (KB)"
set xlabel "Compression"
set grid
show grid
set colorsequence default
set style line 1 linewidth 1 pointtype 7 pointsize 1 linecolor rgb "red"
set style line 2 linewidth 1 pointtype 7 pointsize 1 linecolor rgb "green"
set style line 3 linewidth 1 pointtype 7 pointsize 1 linecolor rgb "purple"
set style line 4 linewidth 1 pointtype 7 pointsize 1 linecolor rgb "blue"
plot    'graphData.txt' using 1:2 with linespoints linestyle 1 title "Quality: 0.3", \
        'graphData.txt' using 1:3 with linespoints linestyle 2 title "Quality: 0.5", \
        'graphData.txt' using 1:4 with linespoints linestyle 3 title "Quality: 0.7", \
        'graphData.txt' using 1:5 with linespoints linestyle 4 title "Quality: 1.0"
pause -1