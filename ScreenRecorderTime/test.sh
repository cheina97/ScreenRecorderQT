#!/bin/bash
#./main 100 100 0 0 0 5 300 1 1 out_300s_5fps.mp4
#./main 100 100 0 0 0 10 300 1 1 out_300s_10fps.mp4
#./main 100 100 0 0 0 20 300 1 1 out_300s_20fps.mp4
#./main 100 100 0 0 0 30 300 1 1 out_300s_30fps.mp4
#./main 100 100 0 0 0 40 300 1 1 out_300s_40fps.mp4
#./main 100 100 0 0 0 50 300 1 1 out_300s_50fps.mp4
#./main 100 100 0 0 0 60 300 1 1 out_300s_60fps.mp4
#
#echo
#echo out_300s_5fps.mp4
#mio_videolength out_300s_5fps.mp4
#
#echo
#echo out_300s_10fps.mp4
#mio_videolength out_300s_10fps.mp4
#
#echo
#echo out_300s_20fps.mp4
#mio_videolength out_300s_20fps.mp4
#
#echo
#echo out_300s_30fps.mp4
#mio_videolength out_300s_30fps.mp4
#
#echo
#echo out_300s_40fps.mp4
#mio_videolength out_300s_40fps.mp4
#
#echo
#echo out_300s_50fps.mp4
#mio_videolength out_300s_50fps.mp4
#
#echo
#echo out_300s_60fps.mp4
#mio_videolength out_300s_60fps.mp4


#./main 1920 1080 0 0 0 30 15 1 1 0 ./tests/out_15s_compression0.mp4
#./main 1920 1080 0 0 0 30 15 1 1 1 ./tests/out_15s_compression1.mp4
#./main 1920 1080 0 0 0 30 15 1 1 2 ./tests/out_15s_compression2.mp4
#./main 1920 1080 0 0 0 30 15 1 1 3 ./tests/out_15s_compression3.mp4
#./main 1920 1080 0 0 0 30 15 1 1 4 ./tests/out_15s_compression4.mp4
#./main 1920 1080 0 0 0 30 15 1 1 5 ./tests/out_15s_compression5.mp4
#./main 1920 1080 0 0 0 30 15 1 1 6 ./tests/out_15s_compression6.mp4
#./main 1920 1080 0 0 0 30 15 1 1 7 ./tests/out_15s_compression7.mp4
#./main 1920 1080 0 0 0 30 15 1 1 8 ./tests/out_15s_compression8.mp4
#./main 1920 1080 0 0 0 30 15 1 1 9 ./tests/out_15s_compression9.mp4
#
#du -h ./tests/out_15s_compression*

./main 1920 1080 0 0 0 30 5 0.7 1 8 ./tests/out_5s_position1.mp4
./main 1920 1080 0 0 0 30 5 1 1 8 ./tests/out_5s_position2.mp4
./main 1920 1080 0 0 0 30 5 0.9 1 7 ./tests/out_5s_position3.mp4
./main 1920 1080 0 0 0 30 5 1 1 7 ./tests/out_5s_position4.mp4
./main 1920 1080 0 0 0 30 5 1 1 6 ./tests/out_5s_position5.mp4
./main 1920 1080 0 0 0 30 5 1 1 5 ./tests/out_5s_position6.mp4
./main 1920 1080 0 0 0 30 5 1 1 4 ./tests/out_5s_position7.mp4

du -h ./tests/out_5s_position*
