#include <string.h>
#include "bench.h"
#include "../engine/db.h" //for the database
#include "../engine/variant.h" //for the database
#include <pthread.h> //to create threads
#include <stdio.h> 
#include <stdlib.h>

#define THREADS 5
#define DATAS ("testdb") //for the database

pthread_mutex_t WRlock = PTHREAD_MUTEX_INITIALIZER; //initialization for the lock of write-readers
struct thdata { //struct to push argumnets into the threads
	long int count_th; //count value for the thread arguments
	int r_th; //r value for the thread arguments
	DB* db; //common database for the thread arguments
};

struct thread_inputs {
	long int thcount; // the value count
	int thid; // the id of each thread
	int thr; //the value r
	DB* thdb; //the common database for all threads to read
	
};

void _random_key(char *key,int length) {
	int i;
	char salt[36]= "abcdefghijklmnopqrstuvwxyz0123456789";

	for (i = 0; i < length; i++)
		key[i] = salt[rand() % 36];
}

void _print_header(int count)
{
	double index_size = (double)((double)(KSIZE + 8 + 1) * count) / 1048576.0;
	double data_size = (double)((double)(VSIZE + 4) * count) / 1048576.0;

	printf("Keys:\t\t%d bytes each\n", 
			KSIZE);
	printf("Values: \t%d bytes each\n", 
			VSIZE);
	printf("Entries:\t%d\n", 
			count);
	printf("IndexSize:\t%.1f MB (estimated)\n",
			index_size);
	printf("DataSize:\t%.1f MB (estimated)\n",
			data_size);

	printf(LINE1);
}

void _print_environment()
{
	time_t now = time(NULL);

	printf("Date:\t\t%s", 
			(char*)ctime(&now));

	int num_cpus = 0;
	char cpu_type[256] = {0};
	char cache_size[256] = {0};

	FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo) {
		char line[1024] = {0};
		while (fgets(line, sizeof(line), cpuinfo) != NULL) {
			const char* sep = strchr(line, ':');
			if (sep == NULL || strlen(sep) < 10)
				continue;

			char key[1024] = {0};
			char val[1024] = {0};
			strncpy(key, line, sep-1-line);
			strncpy(val, sep+1, strlen(sep)-1);
			if (strcmp("model name", key) == 0) {
				num_cpus++;
				strcpy(cpu_type, val);
			}
			else if (strcmp("cache size", key) == 0)
				strncpy(cache_size, val + 1, strlen(val) - 1);	
		}

		fclose(cpuinfo);
		printf("CPU:\t\t%d * %s", 
				num_cpus, 
				cpu_type);

		printf("CPUCache:\t%s\n", 
				cache_size);
	}
}

int main(int argc,char** argv)
{
	long int count;
	DB* db; //added the database in bench.c to push it into the main write and read thread

	srand(time(NULL));
	if (argc < 3) {
		fprintf(stderr,"Usage: db-bench <write | read | write-read> <count> [write%% read%%] <random>\n"); //change the description for the write-read function
		exit(1);
	}
	db = db_open(DATAS); //open the database
	if (strcmp(argv[1], "write") == 0) {
		int r = 0;

		count = atoi(argv[2]);

		struct thdata *writer_args = malloc(sizeof(struct thdata)); //initiate the struct as arguments for writer thread
		pthread_t writer; // initiate writer thread

		_print_header(count);
		_print_environment();
		count = atoi(argv[2]);
		// start of new code
		DB* db;
		db = db_open(DATAS);
		if (argc == 4)
			r = 1;
		int wr = THREADS; //number of writers
		pthread_t writers[THREADS]; 
		if ( count < THREADS ){
			wr = count; //if writes are lesser than THREADS, create up to that number instead of threads
		}
		for(int i = 0; i < wr; i++){
			struct thread_inputs *args = malloc(sizeof(struct thread_inputs)); //dynamic allocation for synchronization
			args->thcount = count; //input values
			args->thid = i; //input values
			args->thr = r; //input values
			args->thdb = db; //input database
			pthread_create(&writers[i], NULL, &_write_test, args); //create the threads to do the routine
			free(args);
		}
		for(int i = 0; i < wr; i++){
			pthread_join(writers[i], NULL); //join the threads
		}
		db_close(db);
		// finish of new code
		_print_header(count);
		_print_environment();
		
		writer_args->count_th = count; //input count to arguments
		writer_args->r_th = r; //input r(random) to arguments
		writer_args->db = db; //input database to arguments
		
		pthread_create(&writer, NULL, &_write_test, writer_args); //create thread and pass arguments
		pthread_join(writer, NULL); //wait for thread to join to continue
		free(writer_args);
	} else if (strcmp(argv[1], "read") == 0) {
		int r = 0;

		count = atoi(argv[2]);

		struct thdata *reader_args = malloc(sizeof(struct thdata)); //initiate the struct as arguments for reader thread
		pthread_t reader; // initiate reader thread

		_print_header(count);
		_print_environment();
		if (argc == 4)
			r = 1;
		
		reader_args->count_th = count; //input count to arguments
		reader_args->r_th = r; //input r(random) to arguments
		reader_args->db = db; //input database to arguments
		
		pthread_create(&reader, NULL, &_read_test, reader_args); //create thread and pass arguments
		pthread_join(reader, NULL); //wait for thread to join to continue
		free(reader_args);
	} else if (strcmp(argv[1], "write-read") == 0) {
		int r = 0;
		int perc1; //percentage for writers
		int perc2; //percentage for readers

		struct thdata *reader_args = malloc(sizeof(struct thdata)); //initiate the struct as arguments for reader thread
		pthread_t reader; // initiate reader thread

		struct thdata *writer_args = malloc(sizeof(struct thdata)); //initiate the struct as arguments for writer thread
		pthread_t writer; // initiate writer thread

		count = atoi(argv[2]);
		_print_header(count);
		_print_environment();

		perc1 = atoi(argv[3]); //percentage for writer (assuming it's 100 > perc1 > 0)
		perc2 = atoi(argv[4]); //percentage for readers (assuming it's 100 > perc2 > 0)
		if (argc == 6)
			r = 1;
		
		reader_args->count_th = (long int) (count*perc2/100); //input count to arguments
		reader_args->r_th = r; //input r(random) to arguments
		reader_args->db = db; //input database to arguments

		writer_args->count_th = (long int) (count*perc1/100); //input count to arguments
		writer_args->r_th = r; //input r(random) to arguments
		writer_args->db = db; //input database to arguments
	
		pthread_create(&writer, NULL, &_write_test, writer_args); //create thread and pass arguments
		pthread_create(&reader, NULL, &_read_test, reader_args); //create thread and pass arguments
		pthread_join(writer, NULL); //wait for thread to join to continue
		pthread_join(reader, NULL); //wait for thread to join to continue
		free(reader_args);
		free(writer_args);
	} else {
		fprintf(stderr,"Usage: db-bench <write | read | write-read> <count> [write%% read%%] <random>\n"); //changed for the write read function
		db_close(db);
		exit(1);
	}
	db_close(db); //close database
	return 1;
}
