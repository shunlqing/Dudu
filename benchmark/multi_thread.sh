#!/bin/sh

killall Dudu
timeout=${timeout:-10}
bufsize=${bufsize:-16384}

for nosessions in 100 1000 10000; do
  for nothreads in 1 2 3 4; do
    sleep 2
    echo "Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
    taskset -c 0 ./../cmake-build-debug/Dudu $nothreads & srvpid=$!
    sleep 1
    taskset -c 1 ./../cmake-build-debug/cli 127.0.0.1 8088 $nothreads $bufsize $nosessions $timeout
    kill -9 $srvpid
  done
done