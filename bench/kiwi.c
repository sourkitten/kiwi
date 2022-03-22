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
struct Global {
	pthread_mutex_t mutex; 
	DB* db;
	int found;
	int count;
	int r;
} global;

struct Global gVariables;

struct parameters {
		int id;
};

void _write_test(long int count, int r)
{
	int i;
	double cost;
	long long start,end;


	// MULTI THREADING STARTS HERE
	// THESE NEED TO BE IN EVERY THREAD
	Variant sk, sv;
	DB* db; // GLOBAL

	char key[KSIZE + 1];
	char val[VSIZE + 1];
	char sbuf[1024];

	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf, 0, 1024);

	db = db_open(DATAS);

	start = get_ustime_sec();
	for (i = 0; i < count; i++) { // COPY i IN THREAD
		if (r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d adding %s\n", i, key);
		snprintf(val, VSIZE, "val-%d", i);

		sk.length = KSIZE;
		sk.mem = key;
		sv.length = VSIZE;
		sv.mem = val;

		// PAUSE OTHER THREADS WHEN MERGING
		// LOCK DATABASE WHEN ADDING
		db_add(db, &sk, &sv);
		if ((i % 10000) == 0) {
			fprintf(stderr,"random write finished %d ops%30s\r", 
					i, 
					"");

			fflush(stderr);
		}
		// MULTI THREADING ENDS HERE
	}
	// WAIT FOR ALL THREADS TO FINISH
	db_close(db);

	end = get_ustime_sec();
	cost = end -start;

	printf(LINE);
	printf("|Random-Write	(done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n"
		,count, (double)(cost / count)
		,(double)(count / cost)
		,cost);	
}

//--- Added Code!! ---//
void* _read_test_thread(void *args)
{
	int i;
	int ret;

	//gVariables variables;
	struct parameters *parameters = (struct parameters *) args;

	printf("Thread %d reporting!: c:%d\n", parameters->id, gVariables.count);

	Variant sk;
	Variant sv;
	char key[KSIZE + 1];

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
		
		//pthread_mutex_lock(&(variables.mutex)); // lock the variable
		ret = db_get(gVariables.db, &sk, &sv);
		//printf("%d: %d\n", i, ret);
		//pthread_mutex_unlock(&(variables.mutex)); // unlock the variable

		if (ret) {
			//db_free_data(sv.mem);
			pthread_mutex_lock(&(gVariables.mutex)); // lock the variable
			gVariables.found++; // increment the value - critical!!!!
			pthread_mutex_unlock(&(gVariables.mutex)); // unlock the variable
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
		// MULTITHREADING ENDS HERE
	}
	free(args);
	pthread_exit(NULL);
}

//--- Added Code!! ---//
void _read_test(long int count, int r)
{
	//gVariables variables;

	// GLOBAL VARIABLES
	gVariables.found = 0;
	gVariables.count = 0;
	gVariables.count = count;
	// printf("var.c: %d\n", gVariables.count);
	gVariables.db = db_open(DATAS);
	gVariables.r = r;

	// LOCAL VARIABLES
	int i;
	double cost;
	long long start, end;

	start = get_ustime_sec();
	
	int a = THREADS;
	if (count < THREADS) { // Less than ${THREADS} keys - No need for ${THREADS} threads
		a = count;
	}
	pthread_t readers[a]; // up to ${THREADS} threads
	pthread_mutex_init (&gVariables.mutex , NULL); // initiate the mutex

	for (i = 0; i < a; i++) { // spawn up to ${THREADS} threads
		struct parameters *params = malloc(sizeof(struct parameters)); // helps prevent data corruption
		params->id = i;
		pthread_create(&readers[i], NULL, _read_test_thread, params);
	}
	for (i = 0; i < a; i++) { // wait for up to ${THREADS} threads
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
