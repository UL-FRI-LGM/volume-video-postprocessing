#!/bin/sh

node raw2bvp.js -m 1 -i raw/0/red126,125,123.bin -d 126x125x123 -f R8 -G false -m 2 -i raw/0/green126,125,123.bin -d 126x125x123 -f R8 -G false -m 3 -i raw/0/blue126,125,123.bin -d 126x125x123 -f R8 -G false -o output.bvp
