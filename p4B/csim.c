////////////////////////////////////////////////////////////////////////////////
// Main File:        (csim.c)
// This File:        (csim.c)
// Other Files:      (name of all other files if any)
// Semester:         CS 354 Lecture 001 Spring 2023
// Instructor:       deppeler
// 
// Author:           (Mohammud Ibrahim)
// Email:            (Mmibrahim2@wisc.edu)
// CS Login:         (Mohammud)
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          Identify persons by name, relationship to you, and email.
//                   Describe in detail the the ideas and help they provided.
//
// Online sources:   avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description of 
//                   of any information you find.
//////////////////////////// 80 columns wide ///////////////////////////////////
//ADD YOUR HEADER COMMENT

////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2013,2019-2020
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission for Spring 2023
//
////////////////////////////////////////////////////////////////////////////////

/*
 * csim.c:  
 * A cache simulator that can replay traces (from Valgrind) and output
 * statistics for the number of hits, misses, and evictions.
 * The replacement policy is LRU.
 *
 * Implementation and assumptions:
 *  1. Each load/store can cause at most one cache miss plus a possible eviction.
 *  2. Instruction loads (I) are ignored.
 *  3. Data modify (M) is treated as a load followed by a store to the same
 *  address. Hence, an M operation can result in two cache hits, or a miss and a
 *  hit plus a possible eviction.
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
#include <stdbool.h>

/******************************************************************************/
/* DO NOT MODIFY THESE VARIABLES **********************************************/

//Globals set by command line args.
int b = 0; //number of block (b) bits
int s = 0; //number of set (s) bits
int E = 0; //number of lines per set

//Globals derived from command line args.
int B; //block size in bytes: B = 2^b
int S; //number of sets: S = 2^s

//Global counters to track cache statistics in access_data().
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;

//Global to control trace output
int verbosity = 0; //print trace if set
/******************************************************************************/
//placeholder
int LRU_counter;
//Type mem_addr_t: Use when dealing with addresses or address masks.
typedef unsigned long long int mem_addr_t;

//Type cache_line_t: Use when dealing with cache lines.
//TODO - COMPLETE THIS TYPE
typedef struct cache_line {                    
    char valid;
    mem_addr_t tag;
    //Add a data member as needed by your implementation for LRU tracking.
	int l;
} cache_line_t;

//Type cache_set_t: Use when dealing with cache sets
//Note: Each set is a pointer to a heap array of one or more cache lines.
typedef cache_line_t* cache_set_t;

//Type cache_t: Use when dealing with the cache.
//Note: A cache is a pointer to a heap array of one or more sets.
typedef cache_set_t* cache_t;

// Create the cache we're simulating. 
//Note: A cache is a pointer to a heap array of one or more cache sets.
cache_t cache;  

/* TODO - COMPLETE THIS FUNCTION
 * init_cache:
 * Allocates the data structure for a cache with S sets and E lines per set.
 * Initializes all valid bits and tags with 0s.
 */                    
void init_cache() {
	// calculate number of sets in the cache
	S = pow(2,s);
	// calculate the block size
	B = pow(2,b);
	// Allocate memory
	cache = (cache_t)malloc(sizeof(cache_set_t)*S);
	// loop through each set
	for (int i = 0; i < S; i++) {
		// allocate for each line
		*(cache+i)= (cache_set_t)malloc(sizeof(cache_line_t)*E); 
		for (int j = 0; j < E; j++) {
            // valid bit to 0
     	    (*(*(cache+i)+j)).valid = 0; 
            // tag bits to 0
      		(*(*(cache+i)+j)).tag = 0; 
			(*(*(cache+i)+j)).l = 0; 
			}
  	}	        
}
  

/* TODO - COMPLETE THIS FUNCTION 
 * free_cache:
 * Frees all heap allocated memory used by the cache.
 */                    
void free_cache() {
 	// loop through
	for (int i = S-1; i >= 0; i--) {
		// free each individual row of
		free(cache[i]);
		// should this be null
		cache[i] = 0;
	} 
	free(cache);
	// set 0
	// should this be null
	cache = 0;             
}


/* TODO - COMPLETE THIS FUNCTION 
 * access_data:
 * Simulates data access at given "addr" memory address in the cache.
 *
 * If already in cache, increment hit_cnt
 * If not in cache, cache it (set tag), increment miss_cnt
 * If a line is evicted, increment evict_cnt
 */                    
void access_data(mem_addr_t addr) { 	
    // calculate tag bits by shift 
	mem_addr_t tag = addr >> (s+b);
	// calculate set index bits by masking
	int set = (addr - (tag << (s+b))) >> b;	
    // loop through set for a tag match
    for (int i = 0; i < E; i++) {
		// check if in cache
		if (cache[set][i].valid && cache[set][i].tag == tag) {
          // hit then increment 
			hit_cnt++;
			// set LRU counter to current
			cache[set][i].l = LRU_counter;
			LRU_counter++;
			return;
			}
		}
		// increment miss
		miss_cnt++;
	//loop to check each line in the set for an invalid line
	for (int i = 0; i < E; i++) {
        if (!cache[set][i].valid) { 
			// use it
			cache[set][i].valid = 1; 
			cache[set][i].l = LRU_counter; // set LRU counter
			cache[set][i].tag = tag; // set tag 
			return;
		}  
     }
	// if no invalid lines are found, choose LRU line for evictions
	// increment
	evict_cnt++;
	// initialize
    int eviction_line = 0; 
    for(int i = 0; i < E; i++){
        if (cache[set][i].l < cache[set][eviction_line].l)
            // found the line to change
            eviction_line = i;
    }
    // update counter and tag
		cache[set][eviction_line].l = LRU_counter; 
		cache[set][eviction_line].tag = tag;      
}
  
  
/* TODO - FILL IN THE MISSING CODE
 * replay_trace:
 * Replays the given trace file against the cache.
 *
 * Reads the input trace file line by line.
 * Extracts the type of each memory access : L/S/M
 * TRANSLATE each "L" as a load i.e. 1 memory access
 * TRANSLATE each "S" as a store i.e. 1 memory access
 * TRANSLATE each "M" as a load followed by a store i.e. 2 memory accesses 
 */                    
void replay_trace(char* trace_fn) {           
    char buf[1000];  
    mem_addr_t addr = 0;
    unsigned int len = 0;
    FILE* trace_fp = fopen(trace_fn, "r"); 

    if (!trace_fp) { 
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);   
    }

    while (fgets(buf, 1000, trace_fp) != NULL) {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
      
            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);

            // TODO - MISSING CODE
            // GIVEN: 1. addr has the address to be accessed
            //        2. buf[1] has type of acccess(S/L/M)
            // call access_data function here depending on type of access
			// check L
			if (buf[1] == 'L') { 
				access_data(addr); 
			} 
			// check S
			else if (buf[1] == 'S') { 
				access_data(addr);
			}
			// check M
			else if(buf[1] == 'M'){
				//load 
				access_data(addr);
				//store 
                access_data(addr);
			}
            if (verbosity)
                printf("\n");
        }
    }

    fclose(trace_fp);
}  
  
  
/*
 * print_usage:
 * Print information on how to use csim to standard output.
 */                    
void print_usage(char* argv[]) {                 
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of s bits for set index.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of b bits for block offsets.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}  
  
  
/*
 * print_summary:
 * Prints a summary of the cache simulation statistics to a file.
 */                    
void print_summary(int hits, int misses, int evictions) {                
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}  
  
  
/*
 * main:
 * Main parses command line args, makes the cache, replays the memory accesses
 * free the cache and print the summary statistics.  
 */                    
int main(int argc, char* argv[]) {                      
    char* trace_file = NULL;
    char c;
    
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t 
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'h':
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            default:
                print_usage(argv);
                exit(1);
        }
    }

    //Make sure that all required command line args were specified.
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv);
        exit(1);
    }

    //Initialize cache.
    init_cache();

    //Replay the memory access trace.
    replay_trace(trace_file);

    //Free memory allocated for cache.
    free_cache();

    //Print the statistics to a file.
    //DO NOT REMOVE: This function must be called for test_csim to work.
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    return 0;   
}  

// Spring 202301
