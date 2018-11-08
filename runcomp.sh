#! /bin/bash

gcc -std=gnu11 main.c ari.c -lm $1 $2 $3

echo "processing 1"
time ./a.out -c ./tests/test_1 ./out/out1
echo "processing 2"
time ./a.out -c ./tests/test_2 ./out/out2
echo "processing 3"
time ./a.out -c ./tests/test_3 ./out/out3
echo "processing 4"
time ./a.out -c ./tests/test_4 ./out/out4
echo "processing 5"
time ./a.out -c ./tests/test_5 ./out/out5
echo "processing 6"
time ./a.out -c ./tests/test_6 ./out/out6
echo "processing 7"
time ./a.out -c ./tests/test_7 ./out/out7
echo "processing 8"
time ./a.out -c ./tests/test_8 ./out/out8

ls -al out
