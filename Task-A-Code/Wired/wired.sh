#!/bin/bash

rm *.flowmonitor

declare -a nodes=(20, 40, 60, 80, 100)
declare -a flows=(10, 20, 30, 40, 50)
# declare -a pps=(100, 200, 300, 400, 500)
declare -a pps=(10, 20, 30, 40, 50)

for n in ${nodes[@]}; do
    ./waf --run "scratch/wired --n_nodes=$n --n_flows=20 --pkts_ps=20 --duration=20"
done

for f in ${flows[@]}; do
    ./waf --run "scratch/wired --n_nodes=60 --n_flows=$f --pkts_ps=20 --duration=20"
done

for p in ${pps[@]}; do
    ./waf --run "scratch/wired --n_nodes=60 --n_flows=30 --pkts_ps=$p --duration=20"
done

python3 processFlowA_wired.py

libreoffice --headless --convert-to xlsx:"Calc MS Excel 2007 XML" results.csv
