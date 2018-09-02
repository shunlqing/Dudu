#!/bin/sh

set -x

killall Dudu
timeout=${timeout:-10}
#bufsize=${bufsize:-16384}
nothreads=1

for bufsize in 1024 2048 4096 8192 16384 81920; do
for nosessions in 1 10 100 1000 10000; do
  echo "==> Bufsize: $bufsize Threads: $nothreads Sessions: $nosessions"
  taskset -c 0 ../Dudu $nothreads & srvpid=$!
  sleep 1
  taskset -c 1 ../cli 127.0.0.1 8088 $nothreads $bufsize $nosessions $timeout
  kill -9 $srvpid
  sleep 5
done
done