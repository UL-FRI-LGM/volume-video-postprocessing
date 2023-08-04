#!/bin/sh
mkdir bvp
for ((i=0; i<60; i++)); do
  node raw2bvp.js -m 1 -i raw/$i/red126,125,123.raw -d 126x125x123 -f R8 -G false -m 2 -i raw/$i/green126,125,123.raw -d 126x125x123 -f R8 -G false -m 3 -i raw/$i/blue126,125,123.raw -d 126x125x123 -f R8 -G false -o bvp/$i.bvp
done