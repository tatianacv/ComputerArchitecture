/*Tatiana Castro Velez
 * 15 de noviembre de 2016
 * Arquitectura de Computadoras
 * Dr. Megret
 * loginId: tcastro
 */

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "cachelab.h"

/* Globals set by command line args */
int verbosity = 0; /* print trace if set */
char* trace_file = NULL;

typedef struct{
    int hit; //counter for cache hits
    int miss;   //counter for cache misses
    int evictions;  //counter for cache evictions
    int s;  //sets
    int E;  //lines per set
    int b;  //block offset
} parameters;
/*
 * printUsage - Print usage info
 */
void printUsage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}

/*Structure to save all the informartion of the line wich includes the tag, the valid bit and the block */
typedef struct{
    unsigned long long tag;
    int validBit;
    char *block;
    int lru;
} line;

/*Structure to have the information the set of cache */
typedef struct{
    line *lines;
} set;

/*Structure that has the information of the sets of the cache */
typedef struct{
    set *sets;
} cache;

cache create_cache(unsigned long long numberSets, int linesperSet, unsigned long long blockSize){
    cache new_cache;
    set cache_set;
    line cache_line;

    new_cache.sets= (set*)malloc(sizeof(cache_set)*numberSets); //creates all sets of the cache
    //To insert the data into the cache (array)
    for(int i=0; i<numberSets; i++){
        cache_set.lines= (line*)malloc(sizeof(cache_line)*linesperSet); //creates all the lines per set
        new_cache.sets[i]=cache_set; //assigning the cache the corresponding set
        //To insert the data of the lines
        for(int j=0; j<linesperSet; j++){
            cache_line.validBit=0;  //initializing the validBit
            cache_line.tag=0;   //initializing the tag
            cache_line.lru=0;   //counter for least recently used
            cache_set.lines[j] = cache_line;
        }
    }

    return new_cache;
}


//Calculates the least recently used of the block and returns its index
int LRU (set new_set, parameters cache_parameters, int *used_lines) {
    int num_lines = cache_parameters.E;

    // initialize both the most and least frequently used lines equal to the given set's last used line

    int max_used = new_set.lines[0].lru;
    int min_used = new_set.lines[0].lru;

    /* initialize and keep track of the LRU to return */
    int min_used_index = 0;

    line linea;

    //Search through all the lines of the set
    for (int lineIndex = 1; lineIndex < num_lines; lineIndex ++) {

        linea = new_set.lines[lineIndex];

        /* if the current min used line is greater than the number of times this line has been accessed,
         * store the index of this line to be returned later
         * and update value of min used line
         */
        if (min_used > linea.lru) {
            min_used_index = lineIndex;
            min_used = linea.lru;
        }

        /* if the current max used line is less than the current line,
         * update the value of the max used line
         */
        if (max_used < linea.lru) {
            max_used = linea.lru;
        }
    }

    used_lines[0] = min_used;
    used_lines[1] = max_used;

    /* return the index of the LRU line from the inputted set */
    return min_used_index;

}

//Cache Simulation

//This to get the empty line for the evictions
int get_empty_line(set s, parameters cache_parameters) {

    line linea;
    int numberLines = cache_parameters.E; //E gets the number of lines

    //Runs through all of the lines
    for (int i = 0; i < numberLines; i ++) {
        linea = s.lines[i];
        //if validBit is cero then we can overwrite the value, so we return the index of that value
        if (linea.validBit == 0) {
            return i;
        }
    }

    return 0;
}

parameters simulation(cache new_cache, parameters cache_parameters, unsigned long long address){

    int numberLines = cache_parameters.E;   //E gives you the number of lines
    int tag_bits = (64 - (cache_parameters.s + cache_parameters.b)); //t=m-(s+b)
    int cmpr4misses = cache_parameters.hit; //the use of this variable is to compare the hits I had before vs. the hits I have now, to calculate my misses
    long setIndex = (address << (tag_bits)) >> (tag_bits + cache_parameters.b); //gets the index of the set
    int cache_full = 1; //checks if the cache is full

    long input_tag= address >> (cache_parameters.s + cache_parameters.b);   //extracts the tag

    set cmpr_set = new_cache.sets[setIndex];

    for( int li=0; li< numberLines; li++){
        line linea= cmpr_set.lines[li];

        if(linea.validBit==1){

            //hit
            if(linea.tag == input_tag) {
                linea.lru++;
                cache_parameters.hit++;
                cmpr_set.lines[li]=linea;
            }
        } else if(!(linea.validBit) && (cache_full)){
            cache_full=0;
        }
    }

    if (cmpr4misses == cache_parameters.hit){
        cache_parameters.miss++;
    }
    else {
        return cache_parameters;
    }
    /*This happens if there is a miss
     * we have to see which line is empty and overwrite
     * if there is not an empty one, we have to overwrite the least frequently used
     */

    int *used_lines = (int*) malloc(sizeof(int) * 2);
    int min_used_index = LRU(cmpr_set, cache_parameters, used_lines); //gets the index of the LRU

    // if there are no empty lines in cache, must overwrite
        if (cache_full==1) {

            cache_parameters.evictions++;

            /* write and replace LRU */
            cmpr_set.lines[min_used_index].tag = input_tag;
            cmpr_set.lines[min_used_index].lru = used_lines[1] + 1;
        }
        else {
            /* there is at least one empty line we can write to */

            int empty_line_index = get_empty_line(cmpr_set, cache_parameters);

            // update valid/ tag bits with the input cache's at the empty line
            cmpr_set.lines[empty_line_index].tag = input_tag;
            cmpr_set.lines[empty_line_index].validBit = 1;
            cmpr_set.lines[empty_line_index].lru = used_lines[1] + 1;
        }

        free(used_lines);
        return cache_parameters;

}

/*
 * main - Main routine
 */
int main(int argc, char* argv[])
{
    cache new_cache;    //creates the cache
    parameters cache_parameters;    //creates the parameters
    char c; //takes the value of the parameters of the input

    while( (c=getopt(argc,argv,"s:E:b:t:vh")) != -1){
        switch(c){
        case 's':
            cache_parameters.s = atoi(optarg);
            break;
        case 'E':
            cache_parameters.E = atoi(optarg);
            break;
        case 'b':
            cache_parameters.b = atoi(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbosity = 1;
            break;
        case 'h':
            printUsage(argv);
            exit(0);
        default:
            printUsage(argv);
            exit(1);
        }
    }


    long long num_sets = pow(2.0, cache_parameters.s);  //number of sets S=2^s
    long long block_size = pow(2.0, cache_parameters.b); //block size B=2^b
    cache_parameters.hit = 0;   //initializing the hits
    cache_parameters.miss = 0;  //initializing the misses
    cache_parameters.evictions = 0; //initializing the evictions
    
    //Creates the cache with the given parameters
    new_cache = create_cache(num_sets, cache_parameters.E, block_size);
    
    /* run the simulation */
    FILE *read_trace;
    char letter;
    long long address;
    int size;
    

    //Opens file
    read_trace = fopen(trace_file,"r");

    //Reads the file per line
    while (fscanf(read_trace, " %c %llx,%d", &letter, &address, &size) == 3) {
        switch(letter) {
            case 'I':
                break;
            case 'L':
                cache_parameters = simulation(new_cache, cache_parameters, address);
                break;
            case 'S':
                cache_parameters = simulation(new_cache, cache_parameters, address);
                break;
            case 'M':
                cache_parameters = simulation(new_cache, cache_parameters, address);
                cache_parameters = simulation(new_cache, cache_parameters, address);
                break;
            default:
                break;
        }
    }


    printSummary(cache_parameters.hit, cache_parameters.miss, cache_parameters.evictions);
    return 0;
}


