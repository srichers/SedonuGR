set pointsize 0.25

set log x
set log y
set title "Varying Density"
set xlabel "Density (g/ccm)"
set ylabel "BB Energy Density (erg/ccm)"
set term pdf
set output "rho.pdf"
plot 'predicted.dat' u 2:((abs($3-TEMP_HERE)/$3<1e-4)&&(abs($4-YE_HERE)/$4<1e-4)?$1:1/0) w l title "Predicted", \
     'predicted.dat' u 2:((abs($3-TEMP_HERE)/$3<1e-4)&&(abs($4-YE_HERE)/$4<1e-4)?$5:1/0) w l title "Predicted (nonrel)", \
     'results.dat'   u 3:((abs($4-TEMP_HERE)/$4<1e-4)&&(abs($5-YE_HERE)/$5<1e-4)?$12+$13+$14:1/0)     title "Sedonu"

set log x
set log y
set title "Varying Temperature"
set xlabel "Temperature (MeV)"
set ylabel "BB Energy Density (erg/ccm)"
set term pdf
set output "temp.pdf"
plot 'predicted.dat' u 3:((abs($2-RHO_HERE)/$2<1e-4)&&(abs($4-YE_HERE)/$4<1e-4)?$1:1/0) w l title "Predicted", \
     'predicted.dat' u 3:((abs($2-RHO_HERE)/$2<1e-4)&&(abs($4-YE_HERE)/$4<1e-4)?$5:1/0) w l title "Predicted (nonrel)", \
     'results.dat'   u 4:((abs($3-RHO_HERE)/$3<1e-4)&&(abs($5-YE_HERE)/$5<1e-4)?$12+$13+$14:1/0)     title "Sedonu"

unset log x
set log y
set title "Varying Electron Fraction"
set xlabel "Ye"
set ylabel "BB Energy Density (erg/ccm)"
set term pdf
set output "ye.pdf"
plot 'predicted.dat' u 4:((abs($3-TEMP_HERE)/$3<1e-4)&&(abs($2-RHO_HERE)/$2<1e-4)?$1:1/0) w l title "Predicted", \
     'predicted.dat' u 4:((abs($3-TEMP_HERE)/$3<1e-4)&&(abs($2-RHO_HERE)/$2<1e-4)?$5:1/0) w l title "Predicted (nonrel)", \
     'results.dat'   u 5:((abs($4-TEMP_HERE)/$4<1e-4)&&(abs($3-RHO_HERE)/$3<1e-4)?$12+$13+$14:1/0)     title "Sedonu"

set log x
set log y
set title "Varying Density"
set xlabel "Density (g/ccm)"
set ylabel "Invariant Energy Density (erg/ccm/MeV^3)"
set term pdf
set output "rho_invar.pdf"
plot 'results.dat'   u 3:((abs($4-TEMP_HERE)/$4<1e-4)&&(abs($5-YE_HERE)/$5<1e-4)?$18:1/0)     title "Sedonu"
    
    

set log x
set log y
set title "Varying Temperature"
set xlabel "Temperature (MeV)"
set ylabel "Invariant Energy Density (erg/ccm/MeV^3)"
set term pdf
set output "temp_invar.pdf"
plot 'results.dat'   u 4:((abs($3-RHO_HERE)/$3<1e-4)&&(abs($5-YE_HERE)/$5<1e-4)?$18:1/0)     title "Sedonu"

unset log x
set log y
set title "Varying Electron Fraction"
set xlabel "Ye"
set ylabel "Invariant Energy Density (erg/ccm/MeV^3)"
set term pdf
set output "ye_invar.pdf"
plot 'results.dat'   u 5:((abs($4-TEMP_HERE)/$4<1e-4)&&(abs($3-RHO_HERE)/$3<1e-4)?$18:1/0)     title "Sedonu"
