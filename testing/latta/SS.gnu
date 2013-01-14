set yrange [0:18];
set y2range [58:74];
set y2tics 70,4;
set ytics nomirror;
set ylabel "Na, Cl";
set y2label "K";
set terminal png;
set key outside horizontal;
set output "Figure-5.png";
plot [:1500] "results.data" u 1:6 w l title "C_c[Na]", \
    "results.data" u 1:8 w l title "C_c[Cl]", \
    "results.data" u 1:7 axes x1y2 w l title "C_c[K]";

set output;
