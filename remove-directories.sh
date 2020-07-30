#!/bin/bash

# Removes directories containing the word "mirror" or "input"

find . -name "*mirror*" -type d -print0|xargs -0 -r rm -r --

find . -name "*input*" -type d -print0|xargs -0 -r rm -r --