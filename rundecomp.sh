#! /bin/bash

gcc -std=gnu11 main.c ari.c -g -lm $1

echo "processing 1"
time ./a.out -d ./out/out1 ./outdecomp/out1
cmp ./tests/test_1 ./outdecomp/out1
echo "processing 2"
time ./a.out -d ./out/out2 ./outdecomp/out2
cmp ./tests/test_2 ./outdecomp/out2
echo "processing 3"
time ./a.out -d ./out/out3 ./outdecomp/out3
cmp ./tests/test_3 ./outdecomp/out3
echo "processing 4"
time ./a.out -d ./out/out4 ./outdecomp/out4
cmp ./tests/test_4 ./outdecomp/out4
echo "processing 5"
time ./a.out -d ./out/out5 ./outdecomp/out5
cmp ./tests/test_5 ./outdecomp/out5
echo "processing 6"
time ./a.out -d ./out/out6 ./outdecomp/out6
cmp ./tests/test_6 ./outdecomp/out6
echo "processing 7"
time ./a.out -d ./out/out7 ./outdecomp/out7 
cmp ./tests/test_7 ./outdecomp/out7
echo "processing 8"
time ./a.out -d ./out/out8 ./outdecomp/out8
cmp ./tests/test_8 ./outdecomp/out8
