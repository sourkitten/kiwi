#include <string.h>
#include "../engine/db.h"
#include "../engine/variant.h"
#include "bench.h"

#define DATAS ("testdb")

void _write_test(long int count, int r)
{
	int i;
	double cost;
	long long start,end;
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
		// MULTI THREADING STARTS HERE
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

void _read_test(long int count, int r)
{
	int i;
	int ret;
	int found = 0;
	double cost;
	long long start,end;

	// THESE NEED TO BE IN EVERY THREAD
	Variant sk;
	Variant sv;
	DB* db; // GLOBAL
	char key[KSIZE + 1];

	db = db_open(DATAS);
	start = get_ustime_sec();
	for (i = 0; i < count; i++) { // COPY i IN THREAD
		memset(key, 0, KSIZE + 1);

		/* if you want to test random write, use the following */
		// MULTITHREADING STARTS HERE
		if (r)
			_random_key(key, KSIZE);
		else
			snprintf(key, KSIZE, "key-%d", i);
		fprintf(stderr, "%d searching %s\n", i, key);
		sk.length = KSIZE;
		sk.mem = key;
		ret = db_get(db, &sk, &sv);
		if (ret) {
			//db_free_data(sv.mem);
			found++;
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
	// WAIT FOR ALL THREADS TO FINISH
	db_close(db);

	end = get_ustime_sec();
	cost = end - start;
	printf(LINE);
	printf("|Random-Read	(done:%ld, found:%d): %.6f sec/op; %.1f reads /sec(estimated); cost:%.3f(sec)\n",
		count, found,
		(double)(cost / count),
		(double)(count / cost),
		cost);
}
