#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>


//#define uint64_t unsigned
//#define int64_t int

//#include "ari.h"

enum {
    CODE_BITS = 44,
    TABLES_COUNT = 1,
};

typedef struct Table {
    uint16_t *char_to_index;
    int *index_to_char;
    int *char_freq;
    int *char_sum_freq;
    int is_work;

    int max_freq;
    int null_const;
    int freq_mult;
    int to_update;
    int up_const;
    int target_up;
    int next_update;
} Table;

typedef struct File_work_model {
    uint32_t buffer;
    int now_bit;
    FILE *file;
    int add_bits;
} File_work_model;

//int max_freqs[] = {2000, 300, 39000, 10000, 4000};
//int freqs_mult[] = {1, 5, 15, 17, 64};
//int null_consts[] = {2, 8, 2, 2, 64};

//int max_freqs[] = {13332, 1744, 55000, 64900, 64000, 3500};
//int freqs_mult[] = {21, 7, 21, 11, 66, 71};
//int null_consts[] = {2, 2, 2, 2, 2, 2};


int max_freqs[] = {10000, 1000, 1000, 5000, 5000, 5000, 20000, 20000, 20000, 40000, 40000, 60000, 60000};
int freqs_mult[] = {1, 30, 60, 2, 30, 60, 2, 30, 60, 30, 30, 30,  60};
int null_consts[] = {2, 2, 10, 2, 2, 10, 2, 2, 2, 2, 2, 2, 2};

uint64_t TOP_VALUE = (1LL << CODE_BITS) - 1;
int CHAR_COUNT = 256;
int char_count = 0;
int UPDATE_CONST = 2000;

void write_bit(char bit, File_work_model *fwm_out) {
    fwm_out->buffer >>= 1;
    if (bit) fwm_out->buffer |= 0x80;
    (fwm_out->now_bit)--;

    if (fwm_out->now_bit == 0) {
        fwm_out->now_bit = 8;
        uint8_t now = fwm_out->buffer & 0xFF;
        fwrite(&now, 1, sizeof(now), fwm_out->file);
        fwm_out->buffer = 0;
    }
}

void bits_plus_follow(char bit, File_work_model *fwm_out) {
    write_bit(bit, fwm_out);

    bit = !bit;
    for (; fwm_out->add_bits > 0; fwm_out->add_bits--) {
        write_bit(bit, fwm_out);
    }
}

void encode_char(int char_index, uint64_t *right, uint64_t *left, int *char_sum_freq, File_work_model *fwm_out) {
    uint64_t first_qtr = TOP_VALUE / 4 + 1;
    uint64_t half = first_qtr * 2;
    uint64_t third_qtr = first_qtr * 3;

    uint64_t range = (*right - *left) + 1;

    *right = *left + (range * char_sum_freq[char_index - 1]) / char_sum_freq[0] - 1;
    *left = *left + (range * char_sum_freq[char_index]) / char_sum_freq[0];

    if (*right < *left) {
        int a = char_sum_freq[char_index];
        int b = char_sum_freq[char_index-1];
        printf("ERROR: %ld %ld %d %d %d\n", *left, *right, char_index, a, b);
        exit(1);
    }

    for (int i = 1;; i++) {
        if (*right < half) {
            bits_plus_follow(0, fwm_out);
        } else if (*left >= half) {
            bits_plus_follow(1, fwm_out);
            *left -= half;
            *right -= half;
        } else if (*left >= first_qtr && *right < third_qtr) {
            fwm_out->add_bits += 1;
            *left -= first_qtr;
            *right -= first_qtr;
        } else break;

        *left += *left;
        *right += *right + 1;
    }
}

void print_arr(int *array) {
    for (int i = 0; i < CHAR_COUNT + 1; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
}

uint64_t mat_wait(Table *table) {
    uint64_t res = 0;

    for (int i = 0; i < CHAR_COUNT + 1; i++) {
        res += ((table->char_freq[i] / (table->char_sum_freq[0] + 0.0)) * table->index_to_char[i]);
    }
    return res;
}

double approx(Table *table) {
    double res = 0;
    int *char_freq = table->char_freq;
    int char_sum = table->char_sum_freq[0];


    for (int i = 0; i < CHAR_COUNT + 1; i++) {
        res += (char_freq[i] * log(char_freq[i]/(double)char_sum));
    }
    res *= -1;

    return res / (double)char_sum;
}

int update_count(Table *table, int missing) {
    if (table->next_update) {  /* we have some more before actual rescaling */
        table->to_update = table->next_update;
        table->next_update = 0;
        return -1;
    }
    if (table->up_const < table->target_up) { /* double rescale interval if needed */   
        table->up_const <<= 1;
        if (table->up_const > table->target_up) {
            table->up_const = table->target_up;
        }
    }
    table->next_update = missing % table->up_const;
    table->to_update = table->up_const - table->next_update;
    return 0;
}


double update_table(int char_index, Table *table) {
    if (table->to_update > 0) {
        table->to_update -= 1;
        table->char_freq[char_index] += table->freq_mult;
        return -1;
    }
    int *char_freq = table->char_freq;
    int *sum_freq = table->char_sum_freq;
    int null_const = table->null_const;
    int freq_mult = table->freq_mult;
    int missing = *sum_freq;

    int sum = 0;
    for (int i = CHAR_COUNT + 1; i >= 0; i--) {
        sum_freq[i] = sum;
        sum += char_freq[i];
    }

    missing -= *sum_freq;
    if (update_count(table, missing) < 0) return -1;
    if (*sum_freq >= table->max_freq){
        sum = 0;
        for (int i = CHAR_COUNT + 1; i >= 0; i--) {
            char_freq[i] = (char_freq[i] + null_const / 2 ) / null_const;
            if (char_freq[i] == 0) { 
                char_freq[i] = 1; 
            }

            sum_freq[i] = sum;
            sum += char_freq[i];
        }
    }
    int min_index = char_index;

    char_freq[min_index] += freq_mult;

    while (min_index > 0) {
        min_index -= 1;
        sum_freq[min_index] += freq_mult;
    }

    double mt_wt = approx(table);
    //fprintf(stderr, "%lf\n", mt_wt);

    return mt_wt;
}

void init_table(Table *table, int index) {
    table->char_to_index = calloc(CHAR_COUNT,     sizeof(*(table->char_to_index)));
    table->index_to_char = calloc(CHAR_COUNT + 2, sizeof(*(table->index_to_char)));
    table->char_freq     = calloc(CHAR_COUNT + 2, sizeof(*(table->char_freq)));
    table->char_sum_freq = calloc(CHAR_COUNT + 2, sizeof(*(table->char_sum_freq)));
    

    for (int i = 0; i < CHAR_COUNT; i++) {
        table->char_to_index[i] = i + 1;
        table->index_to_char[i + 1] = i;
    }

    for (int i = 0; i < CHAR_COUNT + 1; i++) {
        table->char_freq[i] = 1;
        table->char_sum_freq[i] = CHAR_COUNT + 1 - i;
    }
    
    table->char_freq[0] = 0;
    table->max_freq = max_freqs[index];
    table->null_const = null_consts[index];
    table->freq_mult = freqs_mult[index];
    table->target_up  = UPDATE_CONST;
    table->up_const = CHAR_COUNT >> 1;
}


void rel_buffer(File_work_model *fwm_out) {
    fwm_out->buffer >>= fwm_out->now_bit;
    uint8_t tmp = fwm_out->buffer & 0xFF;
    fwrite(&tmp, 1, sizeof(tmp), fwm_out->file);
}


void end_encoding(uint64_t left, File_work_model *fwm_out) {
    fwm_out->add_bits += 1;
    if (left < TOP_VALUE / 4) bits_plus_follow(0, fwm_out);
    else bits_plus_follow(1, fwm_out);
    rel_buffer(fwm_out);
}


void destroy_table(Table *tables) {
    for (int i = 0; i < TABLES_COUNT; i++) {
        free(tables[i].char_to_index);
        free(tables[i].index_to_char);
        free(tables[i].char_freq);
        free(tables[i].char_sum_freq);
    }
    free(tables);
}

void add_char(Table *table, int char_index) {
    if(table->char_freq[char_index] != 0) return;
    table->char_freq[char_index] += 1;

    for (int i = char_index; i >= 0; i--) {
        table->char_sum_freq[i] += 1;
    }
}


void compress_text(FILE *ifp, FILE *ofp) {
    Table *tables = calloc(TABLES_COUNT, sizeof(*tables));
    File_work_model *fwm_out = calloc(1, sizeof(*fwm_out));

    fwm_out->now_bit = 8;
    fwm_out->buffer = 0;
    fwm_out->file = ofp;
    fwm_out->add_bits = 0;

    uint16_t now_char;
    uint64_t left = 0;
    uint64_t right = TOP_VALUE;

    int work_index = 0;
    double coof = 0.05;

    for (int i = 0; i < TABLES_COUNT; i++) {
        init_table(&tables[i], i);
    }
    while ((now_char = getc(ifp)) != UINT16_MAX) {
        char_count += 1;
        int char_index = tables[work_index].char_to_index[now_char];
        /*for (int i = 0; i < TABLES_COUNT; i++) {
            add_char(&(tables[i]), char_index);
        }*/
        encode_char(char_index, &right, &left, tables[work_index].char_sum_freq, fwm_out);
        double max_disp = 0;
        int max_index = -1;


        for (int i = 0; i < TABLES_COUNT; i++) {
            double now_disp = update_table(char_index, &(tables[i]));
        
            if ((now_disp + coof < max_disp || max_index == -1) && now_disp != -1) {
                max_index = i;
                max_disp = now_disp;
            }
        }
        
        if (max_index != -1) work_index = max_index;
    }
    encode_char(CHAR_COUNT + 1, &right, &left, tables[work_index].char_sum_freq, fwm_out);
    tables[work_index].to_update = 0;
    tables[work_index].next_update = 0;

    double now_disp = update_table(CHAR_COUNT + 1, &(tables[work_index]));
    fprintf(stderr, "%lf\n", now_disp);
    end_encoding(left, fwm_out);
    destroy_table(tables);
    free(fwm_out);
}

int read_bit(File_work_model *fwm_in) {
    if (fwm_in->now_bit != 0) {
        int tmp = fwm_in->buffer & 1;
        fwm_in->buffer >>= 1;
        fwm_in->now_bit -= 1;
        return tmp;
    }

    fwm_in->buffer = getc(fwm_in->file);
    if(fwm_in->buffer == EOF) {
        fwm_in->add_bits += 1;
        if (fwm_in->add_bits > CODE_BITS - 2) {
            return -1;
        }
    }

    fwm_in->now_bit = 8;

    return read_bit(fwm_in);
}


int decode_char(int64_t *left, int64_t *right, uint64_t *value, int *char_sum_freq, File_work_model *fwm_in) {
    uint64_t first_qtr = TOP_VALUE / 4 + 1;
    uint64_t half = first_qtr * 2;
    uint64_t third_qtr = first_qtr * 3;
    
    uint64_t range = (*right - *left) + 1;
    uint64_t freq = (((*value - *left) + 1) * char_sum_freq[0] - 1) / range;
    
    int char_index = 1;
    
    for (;char_sum_freq[char_index] > freq; char_index++);

    *right = *left + (range * char_sum_freq[char_index - 1]) / char_sum_freq[0] - 1;
    *left = *left + (range * char_sum_freq[char_index]) / char_sum_freq[0];

    for (;;) {

        if (*right < half) {
        } else if (*left >= half) {
            *value -= half;
            *left -= half;
            *right -= half;
        } else if (*left >= first_qtr && *right < third_qtr) {
            *value -= first_qtr;
            *left -= first_qtr;
            *right -= first_qtr;
        } else break;

        *left += *left;
        *right += *right + 1;
        int tmp = read_bit(fwm_in);
        if (tmp == -1) return -1;
        *value += *value + tmp;
    }
    return char_index;
}

void read_first(uint64_t *value, File_work_model *fwm_in) {
    for (int i = 0; i < CODE_BITS; i++) {
        *value += *value + read_bit(fwm_in);
    }
}


void decompress_text(FILE *ifp, FILE *ofp) {
    Table *tables = calloc(TABLES_COUNT, sizeof(*tables)); 
    File_work_model *fwm_in = calloc(1, sizeof(*fwm_in));
    
    fwm_in->now_bit = 0;
    fwm_in->buffer = 0;
    fwm_in->add_bits = 0;
    fwm_in->file = ifp;

    uint64_t left = 0;
    uint64_t right = TOP_VALUE;
    uint64_t value = 0;
    int work_index = 0;
    
    for (int i = 0; i < TABLES_COUNT; i++) {
        init_table(&tables[i], i);
    }
    read_first(&value, fwm_in);
    for(;;) {
        int char_index = decode_char(&left, &right,&value, tables[work_index].char_sum_freq, fwm_in);
        if (char_index == -1) break;
        if (char_index == CHAR_COUNT + 1)break;
        uint16_t now_char = tables[work_index].index_to_char[char_index];
        putc(now_char, ofp);
        double max_disp = 0;
        int max_index = -1;
        double coof = 0.05;

        for (int i = 0; i < TABLES_COUNT; i++) {
            double now_disp = update_table(char_index, &(tables[i]));
        
            if ((now_disp + coof < max_disp || max_index == -1) && now_disp != -1) {
                max_index = i;
                max_disp = now_disp;
            }
        }
        
        if (max_index != -1) work_index = max_index;
    }

    destroy_table(tables);
    free(fwm_in);
}





void compress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    compress_text(ifp, ofp);


    fclose(ifp);
    fclose(ofp);
}


void decompress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    decompress_text(ifp, ofp);

    fclose(ifp);
    fclose(ofp);
}

