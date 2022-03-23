#include <string.h>
#include "../engine/db.h"
#include "../engine/variant.h"
#include "bench.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define DATAS ("testdb")
#define THREADS 20

//--- Added Code!! ---//
// Store global variables in struct
struct Global {
	pthread_mutex_t mutex; 
	DB* db;
	int found;
	int count;
	int r;
} global;

// initialize global struct
struct Global gVariables;

// store thread parameters in struct
struct parameters {
		int id;
};
//--- End of Added Code!! ---//

//--- New code start ---//
void* _write_test_thread(void* args){

	struct parameters *parameters = (struct parameters *) args; // pass parameters into new structure (only id needed here)
	
	int i;
	Variant sk, sv;

	char key[KSIZE + 1]; 
	char val[VSIZE + 1];
	char sbuf[1024];
	
	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf, 0, 1024);


	for (i = parameters->id; i < gVariables.count; i += 20) { // COPY i IN THREAD
		if (gVariables.r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d adding %s\n", i, key);
		snprintf(val, VSIZE, "val-%d", i);

		sk.length = KSIZE;
		sk.mem = key;
		sv.length = VSIZE;
		sv.mem = val;

		// LOCK DATABASE WHEN ADDING
		pthread_mutex_lock(&(gVariables.mutex));
		db_add(gVariables.db, &sk, &sv);
		pthread_mutex_unlock(&(gVariables.mutex));
		// UNLOCK DATABASE AFTER ADDING
		if ((i % 10000) == 0) {
			fprintf(stderr,"random write finished %d ops%30s\r", 
					i, 
					"");

			fflush(stderr);
		}
	}
	free(args); //Free the space before exiting 
	pthread_exit(NULL);
}

void _write_test(long int count, int r)
{
	int i;
	double cost;
	long long start,end;

	

	gVariables.count = 0; //reset the value
	gVariables.count = count; //insert the value from bench.c
	gVariables.db = db_open(DATAS); //open the database
	gVariables.r = r; //random keys flag

	start = get_ustime_sec();
	int a = THREADS;
	if (count < THREADS) { // Less than ${THREADS} keys - No need for ${THREADS} threads
		a = count;
	}
	pthread_t writers[a]; // create array of threads
	pthread_mutex_init (&gVariables.mutex , NULL); // initiate the mutex

	// spawn the threads
	for (i = 0; i < a; i++) {
		// allocate parameter struct dynamically - helps prevent data corruption
		struct parameters *params = malloc(sizeof(struct parameters));
		params->id = i; // give id number as parameter
		pthread_create(&writers[i], NULL, _write_test_thread, params); // create the thread
	}

	// wait for all the threads
	for (i = 0; i < a; i++) {
		pthread_join(writers[i], NULL);
	}
	pthread_mutex_destroy (&gVariables.mutex); // destroy the mutex

	db_close(gVariables.db);

	end = get_ustime_sec();
	cost = end -start;

	printf(LINE);
	printf("|Random-Write	(done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n"
		,count, (double)(cost / count)
		,(double)(count / cost)
		,cost);	
}
//--- New code finish ---//


//--- Added Code!! ---//
void* _read_test_thread(void *args)
{
	int i;
	int ret;
	int localfound = 0; // Count keys found per thread

	struct parameters *parameters = (struct parameters *) args; // pass parameters into new structure

	// Returns loaded threads
	// printf("Thread %d reporting!: c:%d\n", parameters->id, gVariables.count);

	Variant sk;
	Variant sv;
	char key[KSIZE + 1];

	// every thread reads the (i*20 + id) key. Eg: 3, 23, 43... (count-1) + 3
	for (i = parameters->id; i < gVariables.count; i += THREADS) { // skip ${THREADS} steps
		memset(key, 0, KSIZE + 1);

		/* if you want to test random write, use the following */
		if (gVariables.r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d searching %s\n", i, key);
		sk.length = KSIZE;
		sk.mem = key;
		
		ret = db_get(gVariables.db, &sk, &sv);

		if (ret) {
			//db_free_data(sv.mem);
			localfound++; // keep the found key count local
		} else {
			INFO("not found key#%s", 
					sk.mem);
    	}

		if ((i % 10000) == 0) {
			fprintf(stderr,"random read finished %d ops%30s\r", 
					i, 
					"");

			fflush(stderr);
		}
	}
	// Add all keys found locally to global counter
	pthread_mutex_lock(&(gVariables.mutex)); // lock the variable
	gVariables.found += localfound; // increase the value - critical!!!!
	pthread_mutex_unlock(&(gVariables.mutex)); // unlock the variable
	
	// Exit the thread
	free(args);
	pthread_exit(NULL);
}
//--- End of Added Code!! ---//

//--- Added Code!! ---//
void _read_test(long int count, int r)
{
	// GLOBAL VARIABLES
	// Initiate / Reset global variables
	gVariables.found = 0;
	gVariables.count = 0;
	gVariables.count = count;
	gVariables.db = db_open(DATAS);
	gVariables.r = r;

	// LOCAL VARIABLES
	int i;
	double cost;
	long long start, end;

	start = get_ustime_sec();
	
	/* Check if keys are less than ${THREADS} number of threads
	 * If so, spawn "number of keys" threads instead of ${THREADS}
	 */
	int a = THREADS;
	if (count < THREADS) { // Less than ${THREADS} keys - No need for ${THREADS} threads
		a = count;
	}
	pthread_t readers[a]; // create array of threads
	pthread_mutex_init (&gVariables.mutex , NULL); // initiate the mutex

	// spawn the threads
	for (i = 0; i < a; i++) {
		// allocate parameter struct dynamically - helps prevent data corruption
		struct parameters *params = malloc(sizeof(struct parameters));
		params->id = i; // give id number as parameter
		pthread_create(&readers[i], NULL, _read_test_thread, params); // create the thread
	}

	// wait for all the threads
	for (i = 0; i < a; i++) {
		pthread_join(readers[i], NULL);
	}
	pthread_mutex_destroy (&gVariables.mutex); // destroy the mutex

	db_close(gVariables.db);

	end = get_ustime_sec();
	cost = end - start;
	printf(LINE);
	printf("|Random-Read	(done:%ld, found:%d): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n",
		count, gVariables.found,
		(double)(cost / count),
		(double)(count / cost),
		cost);
}
