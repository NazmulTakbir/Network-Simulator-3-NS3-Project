#!/bin/bash

rm *.flowmonitor

declare -a nodes=(1, 4)
declare -a error_rate=(0, 0.02, 0.04, 0.06, 0.08, 0.10)

for n in ${nodes[@]}; do
    for er in ${error_rate[@]}; do
        ./waf --run "scratch/mywpanB --congestionAlgo=TcpNewReno --n_nodes=$n --error_rate=$er --duration=100"
    done
done

for n in ${nodes[@]}; do
    for er in ${error_rate[@]}; do
        ./waf --run "scratch/mywpanB --congestionAlgo=TcpVegas --n_nodes=$n --error_rate=$er --duration=100"
    done
done

for n in ${nodes[@]}; do
    for er in ${error_rate[@]}; do
        ./waf --run "scratch/mywpanB --congestionAlgo=TcpLrNewReno --n_nodes=$n --error_rate=$er --duration=100"
    done
done 

python3 processFlowB.py

libreoffice --headless --convert-to xlsx:"Calc MS Excel 2007 XML" results.csv