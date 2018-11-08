#include <stdio.h>
#include <string.h>
#include <stdlib.h>


void compress_ari(char*, char*);
void decompress_ari(char*, char*);


int main(int argc, char *argv[]) {
    if (argc < 4) return 1;

    if (!strcmp("-d", argv[1])) {
        decompress_ari(argv[2], argv[3]);
    } else {
        compress_ari(argv[2], argv[3]);
    }
    
    return 0;
}
