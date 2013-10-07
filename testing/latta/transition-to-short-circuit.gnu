set terminal png;
set output "Figure-6.png";
plot [1300:3000] "results.data" u 1:11 w l title "J_net[Na]", \
    "results.data" u 1:12 w l title "J_net[K]";

set yrange [0:18];
set y2range [58:74];
set y2tics 70,4;
set ytics nomirror;
set ylabel "Na, Cl";
set y2label "K";
set terminal png;
set output "Figure-7.png";
set key outside horizontal;
plot [1300:3000] "results.data" u 1:6 w l title "C_c[Na]", \
    "results.data" u 1:8 w l title "C_c[Cl]", \
    "results.data" u 1:7 axes x1y2 w l title "C_c[K]";

set output;
