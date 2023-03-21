#!/bin/csh
# Read heater voltage
# This is on the utility board, Y:2
set done = 0
while ( $done == 0 )
    set result_string = `ccd_read_memory -interface_device pci -board utility -space y -address 2 | grep Result | sed "s/Result = \(.*\)/\1/g"`
    echo "${result_string}"
    sleep 1
end
