#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
#include <limits.h>

typedef struct Triple {
    int max;
    int mult;
    int null;
    int size;
} Triple;


int compress_ari(uint16_t*, char*, int, int, int, double*, int, int, int*);

char test_name[100];
char out_name[100];

void read_file(uint16_t *some) {
    FILE *ifp = fopen(test_name, "r");
    
    uint16_t now_char;
    int i;

    for(i = 0;(now_char = getc(ifp)) != UINT16_MAX; i++) {
        some[i] = now_char;
    }
    some[i] = UINT16_MAX;
}


int main(int argc, char *argv[]) {
    if(argc < 6) return 1;
    snprintf(test_name, 100, "./tests/test_%s", argv[1]);
    snprintf(out_name, 100, "./out/out%s", argv[1]);
    
    uint16_t *some = calloc(30000000, sizeof(*some));

    read_file(some);

    int big_beg = atoi(argv[2]);
    int big_end = atoi(argv[3]);
    int mult_cof = atoi(argv[4]);
    int up_add_const = atoi(argv[5]);
    //int up_add_cof = atoi(argv[6]);
    int coof = (big_end - big_beg) / 6;

    printf("%d\n", coof);


    for (int i = 0; i < 6; i++){
        int begin = coof * i + big_beg;
        int end = coof * (i + 1) + big_beg;
        if (!fork()) {
        int freq_max_min = begin == 0 ? 100 : begin;
        int freq_mult_min = 1;
        int null_const_min = 2;
        int up_cof_max = 2;
        int up_const_max = 2;

        int min_file_size = -1;
        double max_wait = 0;


        for (int freq_max = freq_max_min; freq_max < end; freq_max += coof / 10) {
            if (freq_max % 100 == 0) fprintf(stderr,"MAX FREQ: %d\n", freq_max);
            fflush(stdout);
            for (int freq_mult = 1; freq_mult < 72; freq_mult += mult_cof) {
                for (int null_const = 2; null_const < 73; null_const += mult_cof) {
                    for (int up_const = 2000; up_const < 2060; up_const += up_add_const) {
                    for (int up_cof = 1; up_cof <= 8; up_cof += 1) {
                    double mt_wt;
                    int status = 0;
                    int size = compress_ari(some, out_name, freq_max, freq_mult, null_const, &mt_wt, up_const, up_cof, &status);

                    if (status > 0) break;
                    if ((size < min_file_size)|| min_file_size == -1) {
                        min_file_size = size;
                        freq_max_min = freq_max;
                        freq_mult_min = freq_mult;
                        null_const_min = null_const;
                        up_cof_max = up_cof;
                        up_const_max = up_const;
                        //fprintf(stderr, "MAX WAIT was: %lf new: %lf\n", max_wait, mt_wt);
                        max_wait = mt_wt;

                        fprintf(stderr, "max:%d mult:%d null:%d up_const:%d up_cof:%d size:%d\n", freq_max, freq_mult, null_const, up_const, up_cof, size);
                    }
                    }
                }
                }
            }
        }

        printf("max:%d mult:%d null:%d up_const:%d up_cof:%d size:%d: approx:%lf\n", freq_max_min, freq_mult_min, null_const_min, up_const_max, up_cof_max, min_file_size, max_wait);
        break;
        } else {
        }
    }
    
    while (wait(NULL) != -1);
    free(some);
}
