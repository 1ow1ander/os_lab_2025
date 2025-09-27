#!/bin/bash
if [ $# -eq 0 ]; then
    echo "Количество: 0"
    echo "Среднее: 0"
    exit 0
fi


sum=0
count=0
for num in "$@"; do
    sum=$((sum + num))
    count=$((count + 1))
done


average=$((sum / count))
echo "Количество: $count"
echo "Среднее: $average"