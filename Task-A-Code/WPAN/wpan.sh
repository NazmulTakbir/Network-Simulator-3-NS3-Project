#!/bin/bash

rm *.flowmonitor

declare -a nodes=(2, 4, 6, 8, 10)
declare -a flows=(2, 6, 10, 14, 18)
declare -a pps=(100, 200, 300, 400, 500)

for n in ${nodes[@]}; do
    ./waf --run "scratch/mywpan4 --n_nodes=$n --n_flows=4 --pkts_ps=200 --duration=100"
done

for f in ${flows[@]}; do
    ./waf --run "scratch/mywpan4 --n_nodes=2 --n_flows=$f --pkts_ps=200 --duration=100"
done

for p in ${pps[@]}; do
    ./waf --run "scratch/mywpan4 --n_nodes=2 --n_flows=5 --pkts_ps=$p --duration=100"
done

python3 processFlowA_wpan.py

libreoffice --headless --convert-to xlsx:"Calc MS Excel 2007 XML" results.csv