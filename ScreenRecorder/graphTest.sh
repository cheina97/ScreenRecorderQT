#!/usr/bin/env bash

q_start=0.3

for c in $(seq 1 9); do
    for q in 0.3 0.5 0.7 1; do
        ./main 1920 1080 0 0 0 30 20 $q 0 $c ./graphTest/c${c}q${q}.mp4
    done
done
lines_number=$(ls -l graphTest |wc -l)
let lines_number=lines_number-1

ls -lh --block-size=K ./graphTest|tail -n $lines_number|while read line; do
    size=$(echo $line|cut -d " " -f 5|cut -d "K" -f 1)
    filename=$(echo $line|cut -d " " -f 9)
    compression=$(echo $filename| cut -d "q" -f 1|cut -d "c" -f 2)
    quality=$(echo $filename| cut -d "q" -f 2|cut -d "m" -f 1)
    quality=${quality::-1}
    
    if [ "$quality" == "$q_start" ]; then
        echo -n "$compression $size "
    else
        if [ "$quality" == "1" ]; then
            echo $size
        else
            echo -n "$size "
        fi
    fi
done > graphData.txt
