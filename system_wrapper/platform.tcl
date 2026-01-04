# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\Users\pason\vitis_workspace\system_wrapper\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\Users\pason\vitis_workspace\system_wrapper\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

catch {platform remove system_wrapper}
catch {platform remove system_wrapper}
platform create -name {system_wrapper}\
-hw {C:\fpga\Zynq+AXI\system_wrapper.xsa}\
-out {C:/Users/pason/vitis_workspace}

platform write
domain create -name {standalone_ps7_cortexa9_0} -display-name {standalone_ps7_cortexa9_0} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {hello_world}
platform generate -domains 
platform active {system_wrapper}
domain active {zynq_fsbl}
domain active {standalone_ps7_cortexa9_0}
platform generate -quick
catch {platform remove my_clean_platform}
domain active {zynq_fsbl}
bsp reload
domain active {standalone_ps7_cortexa9_0}
bsp reload
bsp setdriver -ip riscv_axi_wrapper_0 -driver generic -ver 3.1
bsp write
bsp reload
catch {bsp regenerate}
domain active {zynq_fsbl}
bsp setdriver -ip riscv_axi_wrapper_0 -driver generic -ver 3.1
bsp write
bsp reload
catch {bsp regenerate}
platform generate
platform clean
platform generate
platform config -updatehw {C:/fpga/Zynq+AXI/system_wrapper_v2.xsa}
bsp reload
domain active {standalone_ps7_cortexa9_0}
bsp reload
platform generate -domains 
