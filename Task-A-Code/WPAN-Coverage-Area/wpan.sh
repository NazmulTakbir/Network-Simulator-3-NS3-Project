#!/bin/bash

rm *.flowmonitor

declare -a maxRange=(1, 2, 3, 4, 5)

for r in ${maxRange[@]}; do
    ./waf --run "scratch/mywpan5  --maxRange=$r --duration=100"
done

python3 processFlowA_wpan2.py

libreoffice --headless --convert-to xlsx:"Calc MS Excel 2007 XML" results.csv