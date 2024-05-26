//for the scanf_s warning
#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS

//libraries
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

//we have a set block size of 64B
#define BLOCK_SIZE 64

//functions for either FIFO or LRU replacement policies
void FIFO(long long int** cache_tags, int** dirty_bits, int set_num, int cols, int WB, int op, long long int tag_address);
void LRU(long long int** cache_tags, int** replace_metadata, int** dirty_bits, int set_num, int cols, int WB, int op, long long int tag_address);

//global vars for relevant info
int HITS = 0, MISSES = 0, READS = 0, WRITES = 0, FIFO_INDEX = 0;

int main(int argc, char* argv[])
{
	// Parsing part
	int ASSOC = atoi(argv[2]);
	int CACHE_SIZE = atoi(argv[1]);
	int REPLACEMENT = atoi(argv[3]);
	int WB = atoi(argv[4]);
	char *TRACE_FILE = argv[5];
	FILE* fp = fopen(TRACE_FILE, "r");
	//number of rows will be number of sets, number of columns is
	//associativity
	int num_sets = CACHE_SIZE / (ASSOC * BLOCK_SIZE);
	//allocate memory, initialie each index to -1 so we know if an index is unused
	//long long int to hold...long long ints
	long long int** cache_tags = (long long int**)malloc(sizeof(long long int*) * num_sets);
	for (int i = 0; i < num_sets; i++) {
		cache_tags[i] = (long long int*)malloc(sizeof(long long int) * ASSOC);
		for (int j = 0; j < ASSOC; j++) {
			cache_tags[i][j] = -1;
		}
	}
	//allocate memory, initialie each index to -1 so we know if an index is unused
	int** replace_metadata = (int**)malloc(sizeof(int*) * num_sets);
	for (int i = 0; i < num_sets; i++) {
		replace_metadata[i] = (int*)malloc(sizeof(int) * ASSOC);
		for (int j = 0; j < ASSOC; j++) {
			replace_metadata[i][j] = -1;
		}
	}
	//allocate memory, initialie each index to -1 so we know if an index is unused
	//holds the locations of dirty bits for replacement
	int** dirty_bits = (int**)malloc(sizeof(int*) * num_sets);
	for (int i = 0; i < num_sets; i++) {
		dirty_bits[i] = (int*)malloc(sizeof(int) * ASSOC);
		for (int j = 0; j < ASSOC; j++) {
			dirty_bits[i][j] = -1;
		}
	}
	//char for the read/write operation
	char op = ' ';
	//while we arent at the end of the file
	while (op != EOF)
	{
		//get the operation
		op = fgetc(fp);
		// to eliminate the " 0x"
		fgetc(fp);
		fgetc(fp);
		fgetc(fp);
		//address
		long long int address;
		// parse line
		//get the line, convert it from string to hex to long long int
		char line[256];
		fgets(line, sizeof(line), fp);
		address = (long long int)strtol(line, NULL, 16);
		//get the set num and tag address as specified in skeleton 1
		int set_num = (address / BLOCK_SIZE) % num_sets;
		long long int tag_address = address / BLOCK_SIZE;
		//did we hit? and if so, where?
		bool access_hit = false;
		int index_hit = -1;
		//if we're writing through, just chalk it up as a write
		if (op == 'W' && WB == 0) {
			WRITES++;
		}
		//for each column in the set
		for (int i = 0; i < ASSOC; i++)
		{
			//if we hit, iterate HITS, say we hit, mark the index
			if (tag_address == cache_tags[set_num][i])
			{
				HITS++;
				access_hit = true;
				index_hit = i;
				//if write back and we're doing a write, set the dirty bit
				if (op == 'W' && WB == 1) {
					dirty_bits[set_num][i] = 1;
				}
				//exit loop
				break;
			}
		}
		//if we hit
		if(access_hit)
		{
			//for each column in the set, if the index HAS been accessed, add 1 to indicate increased position to be removed in case of LRU
			for (int i = 0; i < ASSOC; i++)
				if (replace_metadata[set_num][i] != -1)
					replace_metadata[set_num][i]++;
			//most recently used gets set to 0
			replace_metadata[set_num][index_hit] = 0;
			//if write back and we're doing a write, set the dirty bit
			//this is in case the previous store in cache was a Read
			if (op == 'W' && WB == 1) {
				dirty_bits[set_num][index_hit] = 1;
			}
		}
		//if we miss
		else {
			//increase READS and MISSES (we do a READ from memory every time we miss)
			READS++;
			MISSES++;
			//if LRU
			if (REPLACEMENT == 0)
				LRU(cache_tags, replace_metadata, dirty_bits, set_num, ASSOC, WB, op, tag_address);
			//if FIFO
			else if (REPLACEMENT == 1)
				FIFO(cache_tags, dirty_bits, set_num, ASSOC, WB, op, tag_address);
		}
	}
	//show the reads and writes
	printf("Reads: %d\nWrites: %d\n", READS, WRITES);
	//miss ratio = misses / accesses
	float ratio = (float)MISSES / (HITS + MISSES);
	printf("Ratio: %f\n", ratio);
}

//first-in first-out: we need the tags, the dirty bits, the set, the cols, WB policy, operation, and the current tag
void FIFO(long long int** cache_tags, int** dirty_bits, int set_num, int cols, int WB, int op, long long int tag_address) {
	//if we're writing to a dirty bit, increase the writes
	if (dirty_bits[set_num][FIFO_INDEX] == 1 && WB == 1) {
		WRITES++;
		dirty_bits[set_num][FIFO_INDEX] = -1;
	}
	//if we're writing to an empty spot, set the dirty bit
	if (dirty_bits[set_num][FIFO_INDEX] == -1 && WB == 1 && op == 'W') {
		dirty_bits[set_num][FIFO_INDEX] = 1;
	}
	//set the new tag address, increase the FIFO, and if we have to return to the start of the FIFO cycle, do that
	cache_tags[set_num][FIFO_INDEX] = tag_address;
	FIFO_INDEX++;
	if (FIFO_INDEX >= cols) FIFO_INDEX = 0;
}

//least recent policy: we need tags, metadata (recency), dirty bits, set number, columns, WB policy, oepration, and tag address
void LRU(long long int** cache_tags, int** replace_metadata, int** dirty_bits, int set_num, int cols, int WB, int op, long long int tag_address) {
	//starting at the first index, find out with index was least recently used and store its index; if we havent used an index, just go there
	int least_used = replace_metadata[set_num][0];
	int index = 0;
	for (int j = 0; j < cols; j++) {
		if (replace_metadata[set_num][j] == -1) {
			index = j;
			break;
		}
		if (replace_metadata[set_num][j] > least_used) {
			least_used = replace_metadata[set_num][j];
			index = j;
		}
	
	//if we're writing to a dirty bit, increase the writes
	if (dirty_bits[set_num][index] == 1 && WB == 1) {
		WRITES++;
		dirty_bits[set_num][index] = -1;
	}
	//if we're writing to an empty spot, set the dirty bit
	if (dirty_bits[set_num][index] == -1 && WB == 1 && op == 'W') {
		dirty_bits[set_num][index] = 1;
	}
	//set the new tag address
	cache_tags[set_num][index] = tag_address;
	//increase recency indices and reset the MRU index
	for (int i = 0; i < cols; i++) {
		if(replace_metadata[set_num][i] != -1)
			replace_metadata[set_num][i]++;
	}
	replace_metadata[set_num][index] = 0;
}