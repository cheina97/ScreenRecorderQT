#!/bin/bash
./main 100 100 0 0 0 5 300 1 1 out_300s_5fps.mp4
./main 100 100 0 0 0 10 300 1 1 out_300s_10fps.mp4
./main 100 100 0 0 0 20 300 1 1 out_300s_20fps.mp4
./main 100 100 0 0 0 30 300 1 1 out_300s_30fps.mp4
./main 100 100 0 0 0 40 300 1 1 out_300s_40fps.mp4
./main 100 100 0 0 0 50 300 1 1 out_300s_50fps.mp4
./main 100 100 0 0 0 60 300 1 1 out_300s_60fps.mp4

echo
echo out_300s_5fps.mp4
mio_videolength out_300s_5fps.mp4

echo
echo out_300s_10fps.mp4
mio_videolength out_300s_10fps.mp4

echo
echo out_300s_20fps.mp4
mio_videolength out_300s_20fps.mp4

echo
echo out_300s_30fps.mp4
mio_videolength out_300s_30fps.mp4

echo
echo out_300s_40fps.mp4
mio_videolength out_300s_40fps.mp4

echo
echo out_300s_50fps.mp4
mio_videolength out_300s_50fps.mp4

echo
echo out_300s_60fps.mp4
mio_videolength out_300s_60fps.mp4


