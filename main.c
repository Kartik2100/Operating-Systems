//Libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Declaring functions
void binaryConvert(int logical, int *availableFrame, int *available, int *entries, int *faults, int *hit, FILE *backing);
void faultsHits(int faults, int hits, int count);

/*
The main parses the addresses file and passes it through the function.
The any malloc and opening files are done here.
*/
int main(int argc, char *argv[]){
    FILE *fp;
    char buf[200];
    int count = 0;
    int *value;
    int *copy;
    int i = 0;
    int *arrPageNum;
    int *arrOffset;
    int available = 0;
    int availableFrame = 0;
    int faults = 0;
    int hit = 0;
    int entries = 0;
    FILE *backing;

    if(argc > 2 || argc == 1 || argc < 0){
        fprintf(stderr,"Invalid arguments, usage: %s filenameoflogicaladdresses\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    fp = fopen(argv[1], "r");

    if(fp == NULL){
        fprintf(stderr, "Error in file opening addresses.txt\n");
        exit(EXIT_FAILURE);
    }

    backing = fopen("BACKING_STORE.bin", "rb");
    if(backing == NULL){
        fprintf(stderr, "Error in file opening BACKING_STORE.bin\n");
        exit(EXIT_FAILURE);
    }

    while(fgets(buf, 200, fp) != NULL){
        count++;
    }
    
    value = malloc(sizeof(int) * count);
    arrOffset = malloc(sizeof(int) * count);
    arrPageNum = malloc(sizeof(int) * count);
    fseek(fp, 0, SEEK_SET);
    while(fgets(buf, 200, fp) != NULL){
        value[i] = atoi(buf);
        i++;
    }

    copy = malloc(sizeof(int) * count);

    for(i = 0; i < count; i++){
        copy[i] = value[i];
        binaryConvert(copy[i], &availableFrame, &available, &entries, &faults, &hit, backing);
    }
    faultsHits(faults, hit - 1, count);

    free(value);
    free(copy);
    free(arrOffset);
    free(arrPageNum);
    fclose(fp);
    fclose(backing);
    return 0;
}

/*
This function is the main address translation where it performs FIFO
in order to convert the addresses to physical addresses and finds the value
for each address.
*/
void binaryConvert(int logical, int *availableFrame, int *available, int *entries, int *faults, int *hit, FILE *backing){
    int TLBpage[16];
    int TLBframe[16];
    int pageNum[256];
    int pageFrames[256];
    signed char buf[256];
    int physical[256][256];
    int i = 0;    
    int page = 0;
    int offset = 0;
    signed char value = 0;
    int logicalHold = 0;
    int temp = 0;
    
    logicalHold = logical;
    
    page = (logical & 0xFFFF) >> 8;
    offset = (logical & 0xFF);
    
    int conditionalNum = -1;

    for(i = 0; i < 16; i++){
        if(TLBpage[i] == page){
            conditionalNum = TLBframe[i];
            *hit = *hit + 1;
        }
    } 
    if(conditionalNum == -1){
        for(i = 0; i < *available; i++){
            if(pageNum[i] == page){
                conditionalNum = pageFrames[i];
            }
        }
        if(conditionalNum == -1){
            if(fseek(backing, page*256, SEEK_SET) != 0){
                fprintf(stderr, "Error in seeking to beginning of file BACKING_STORE.bin\n");
                exit(EXIT_FAILURE);
            }
            if(fread(buf, sizeof(signed char), 256, backing) == 0){
                fprintf(stderr, "Error in reading file BACKING_STORE.bin\n");
                exit(EXIT_FAILURE);
            }

            for(i = 0; i < 256; i++){
                physical[*availableFrame][i] = buf[i];
            }
            pageNum[*available] = page;
            pageFrames[*available] = *availableFrame;

            *available = *available + 1;
            *availableFrame = *availableFrame + 1;
            *faults = *faults + 1;
            conditionalNum = *availableFrame - 1;

        }
    }
    for(i = 0; i < *entries; i++){
        if(TLBpage[i] == page){
            break;
        }
    }

    if(i == *entries){
        if(*entries < 16){
            TLBpage[*entries] = page;
            TLBframe[*entries] = conditionalNum;
        }
        else{
            for(i = 0;i < 15; i++){
                TLBpage[i] = TLBpage[i+1];
                TLBframe[i] = TLBframe[i+1]; 
            }
            TLBpage[*entries - 1] = page;
            TLBframe[*entries - 1] = conditionalNum;
        }
    }

    else{
        while(i < *entries-1){
            TLBpage[i] = TLBpage[i + 1];
            TLBframe[i] = TLBframe[i + 1];
            i++;
        }
        if(*entries < 16){
            TLBpage[*entries] = page;
            TLBframe[*entries] = conditionalNum;
        }
        else{
            TLBpage[*entries - 1] = page;
            TLBframe[*entries - 1] = conditionalNum;
        }
    }
    if(*entries < 16){
        *entries = *entries + 1;
    }
    if(conditionalNum <= 256 && conditionalNum >= 0){
        value = physical[conditionalNum][offset];
    }
    temp = (conditionalNum << 8) | offset;
    printf("Virtual address: %d Physical address: %d Value %d\n", logicalHold, temp, value);
    
}

/*
This function creates the statistics and outputs them
*/
void faultsHits(int faults, int hits, int count){
    float faultRate = 0;
    float hitRate = 0;
    float fault = faults;
    float counts = count;
    float hit = hits;

    faultRate = fault / counts;
    hitRate = hit / counts;

    printf("Number of Translated Addresses = %d\n", count);
    printf("Page Faults = %d\n", faults);
    printf("Page Fault Rate = %.3f\n", faultRate);
    printf("TLB Hits = %d\n", hits);
    printf("TLB Hit Rate = %.3f\n", hitRate);

}

