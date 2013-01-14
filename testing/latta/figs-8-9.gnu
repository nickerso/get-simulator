set xlabel "Time [seconds]";
set yrange [0:85];
set ylabel "Activity [mM]";
set terminal png;
set key outside horizontal;
set output "Figure-8.png";
plot [3400:] "results.data" u 1:6 w l title "C_c[Na]", \
    "results.data" u 1:8 w l title "C_c[Cl]";

set output "Figure-9.png"
set nokey;
set yrange [-25:5];
set xzeroaxis;
set ylabel "E_t [mV]";
plot [3400:] "results.data" u 1:3 w lines;
set output;
