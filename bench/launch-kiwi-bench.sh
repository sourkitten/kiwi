#!/bin/sh

# Simple bash script that launches the bench,
# returns only statistics results and syntax errors

echo "If this script doesn't work, please use the ./kiwi-bench binary directly,"
echo "just add this after your arguments:  2> >(grep Usage) | grep -e \"done\" -e \"Usage:\""
echo

# Redirect stderr to stdout, greping the word Usage, in case of syntax error
# Then grab only statistics (cost:) and syntax errors (Usage:) 
./kiwi-bench $1 $2 $3 $4 $5 $6 2> >(grep Usage) | grep -e "cost:" -e "Usage:"