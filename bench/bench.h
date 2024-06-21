#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "../engine/db.h" //for the database
#include "../engine/variant.h" //for the database

#define KSIZE (16)
#define VSIZE (1000)

#define LINE "+-----------------------------+----------------+------------------------------+-------------------+\n"
#define LINE1 "---------------------------------------------------------------------------------------------------\n"

#define THREADS 5
#define DATAS ("testdb") //for the database

long long get_ustime_sec(void);
void _random_key(char *key,int length);
void* _write_test(void *_args); //needed for the p_thread_create function
void* _read_test(void *_args); //needed for the p_thread_create function

struct thread_inputs { //struct to push arguments into the threads
	int id; //id of the thread
	long int count; // count value
	int random; // r value
	DB* db; // common database
};