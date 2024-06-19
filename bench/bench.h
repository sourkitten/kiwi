#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define KSIZE (16)
#define VSIZE (1000)

#define LINE "+-----------------------------+----------------+------------------------------+-------------------+\n"
#define LINE1 "---------------------------------------------------------------------------------------------------\n"

long long get_ustime_sec(void);
void _random_key(char *key,int length);
void* _write_test(void *_args); //needed for the p_thread_create function
void* _read_test(void *_args); //needed for the p_thread_create function
//void _read_test(void *argsin); //required this in order for the pthread_create to work and do the routine
