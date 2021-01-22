#!/bin/bash
while [ 1 ]; do
    top -H -u iwia021h -b -n 1 >> top.txt
    for i in $(top -H -u iwia021h -b -n 1 | grep blur_aot | awk '{ print $1 }'); do taskset -pc $i >> top.txt; done
    sleep 1
done
