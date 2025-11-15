# ------------------------------------------------------------------------------
# Top Level Simulation Script to source msim_setup.tcl
# ------------------------------------------------------------------------------
set QSYS_SIMDIR obj/default/runtime/sim
source msim_setup.tcl
# Copy generated memory initialization hex and dat file(s) to current directory
file copy -force C:/Users/nicva/Empotrados/Proyecto2-Empotrados/SystemOnChip/software/prueba_SoC/mem_init/hdl_sim/system_on_chip_RAM.dat ./ 
file copy -force C:/Users/nicva/Empotrados/Proyecto2-Empotrados/SystemOnChip/software/prueba_SoC/mem_init/system_on_chip_RAM.hex ./ 
