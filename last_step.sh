#!/bin/sh

node raw2bvp.js -m 1 -i model_combined_raw/red117,77,45.bin -d 117x77x45 -f R8 -G false -m 2 -i model_combined_raw/green117,77,45.bin -d 117x77x45 -f R8 -G false -m 3 -i model_combined_raw/blue117,77,45.bin -d 117x77x45 -f R8 -G false -o output.bvp
