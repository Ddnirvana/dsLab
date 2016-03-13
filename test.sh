#!/bin/bash
rm result
for ((i=0;i<2000;i++)); do
echo Test:$i;
#echo Test:$i >> result_all;
echo Test:$i >> result;
#./rdt_sim 1000 0.1 100 0.3 0.3 0.3 0 >> result_all 2>&1
./rdt_sim 1000 0.1 100 0.3 0.3 0.3 0 >> result 2>&1
done
