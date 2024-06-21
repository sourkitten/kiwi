#include <string.h>
#include "bench.h" 
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//--- Added Code!! ---//
extern pthread_mutex_t WRlock; // lock that allows either read or write

// Store global variables in struct
struct Global {
	pthread_mutex_t mutex; // mutex for locking the found counter
	DB* db; 			   // common database
	int found;			   // global found counter
	long int count;		   // count number
	int r;				   // random flag
} global;

// initialize global struct
struct Global gVariables; //global variables for reader threads

//--- End of Added Code!! ---//

void* _write_test(void *_args)
{
	int i;
	double cost;
	long long start,end;
	//unload the arguments from pthread_create
	struct thread_inputs *writer = (struct thread_inputs*) _args;


	Variant sk, sv;
	DB* db;

	char key[KSIZE + 1];
	char val[VSIZE + 1];
	char sbuf[1024];

	memset(key, 0, KSIZE + 1);
	memset(val, 0, VSIZE + 1);
	memset(sbuf, 0, 1024);

	db = writer->db; //unload the database 

	
	start = get_ustime_sec();
	for (i = writer->id; i < writer->count; i += THREADS) { // change count to writer->count (from args)
		if (writer->random) // change r to writer->r (from args)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d adding %s\n", i, key);
		snprintf(val, VSIZE, "val-%d", i);

		sk.length = KSIZE;
		sk.mem = key;
		sv.length = VSIZE;
		sv.mem = val;

		// Lock for thread safety
		pthread_mutex_lock(&WRlock); //lock for either write or read
		db_add(db, &sk, &sv);
		pthread_mutex_unlock(&WRlock);

		if ((i % 10000) == 0) {
			fprintf(stderr,"random write finished %d ops%30s\r", 
					i, 
					"");

			fflush(stderr);
		}
	}

	end = get_ustime_sec();
	cost = end -start;
	
	printf(LINE);
	printf("|Random-Write	(done:%ld): %.6f sec/op; %.1f writes/sec(estimated); cost:%.3f(sec);\n"
		,writer->count, (double)(cost / writer->count) //count -> count_th
		,(double)(writer->count / cost) //count -> count_th
		,cost);	

	free(_args); //free the arguments with the dynamic memory allocation (for synchronization purposes)

	
	sleep(3);
	sleep(1); // Fixes segmentation fault
	pthread_exit(NULL);
}

//--- Added Code!! ---//
void* _read_test_thread(void *args)
{
	int i;
	int ret;
	int localfound = 0; // Count keys found per thread

	struct thread_inputs *reader = (struct thread_inputs *) args; // pass parameters into new structure

	// Returns loaded threads
	// printf("Thread %d reporting!: c:%d\n", parameters->id, gVariables.count);

	Variant sk;
	Variant sv;
	char key[KSIZE + 1];

	// every thread reads the (m*THREADS + id) key. Eg: 3, 13, 23... (for THREADS = 10)
	for (i = reader->id; i < gVariables.count; i += THREADS) { // skip ${THREADS} steps
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
void* _read_test(void *_args)
{
	// lock that allows either read or write
	pthread_mutex_lock(&WRlock); 
	// unload the arguments from pthread_create
	struct thread_inputs *reader = (struct thread_inputs*) _args; 
	
	// GLOBAL VARIABLES
	// Initiate / Reset global variables
	gVariables.found = 0;	// initialize global found counter to 0
	gVariables.count = 0;	// put on 0 (didn't work otherwise,
							// because threads would just stop)
	gVariables.count = reader->count; //input count from arguments
	gVariables.db = reader->db; 		 //input db from arguments
	gVariables.r = reader->random;		 //input r from arguments

	// LOCAL VARIABLES
	int i;
	double cost;
	long long start, end;

	start = get_ustime_sec();
	/* Check if keys are less than ${THREADS} number of threads
	 * If so, spawn "number of keys" threads instead of ${THREADS}
	 */
	int a = THREADS;
	if (gVariables.count < THREADS) { // Less than ${THREADS} keys
		a = gVariables.count;
	}
	pthread_t readers[a]; // create array of threads
	pthread_mutex_init (&gVariables.mutex , NULL); // initiate the mutex

	// spawn the threads
	for (i = 0; i < a; i++) {
		// allocate parameter struct dynamically - helps prevent data corruption
		struct thread_inputs *reader = malloc(sizeof(struct thread_inputs));
		reader->id = i; // give id number as parameter
		pthread_create(&readers[i], NULL, _read_test_thread, reader); // create the thread
	}

	// wait for all the threads
	for (i = 0; i < a; i++) {
		pthread_join(readers[i], NULL);
	}



	end = get_ustime_sec();
	cost = end - start;
	printf(LINE);
	printf("|Random-Read	(done:%ld, found:%d): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n",
		gVariables.count, gVariables.found,
		(double)(cost / gVariables.count),
		(double)(gVariables.count / cost),
		cost);
	free(_args);  //free the arguments with the dynamic memory allocation (for synchronization purposes)
	
	pthread_mutex_unlock(&WRlock);
	sleep(1); // Fixes segmentation fault, if readers go first
	pthread_exit(NULL);
}
