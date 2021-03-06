#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <signal.h>
#include "../src/procstat.h"

static struct procstat_context *context;

void* fuse_loop(void *arg)
{
	procstat_loop(context);
	return NULL;
}

static void fuse_destroy(int sig)
{
	printf("Destroying ...\n");
	procstat_destroy(context);
	printf("Destroyed ...\n");
	context = NULL;
}


static int set_signal_handlers()
{
	struct sigaction sa;
	struct sigaction old_sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sa.sa_handler = fuse_destroy;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, NULL);
	return 0;
}


static void test_create_dirs()
{
	struct procstat_item *item;
	struct procstat_item *item2;

	item = procstat_create_directory(context, procstat_root(context), "dir1");
	assert(item);

	item2 = procstat_create_directory(context, procstat_root(context), "dir1");
	assert(!item2);
	assert(errno == EEXIST);

	procstat_remove(context, item);

	item = procstat_create_directory(context, NULL, "dir1");
	assert(item);

	procstat_remove_by_name(context, NULL, "dir1");

	item = procstat_create_directory(context, procstat_root(context), "dir1");
	assert(item);

	item = procstat_create_directory(context, NULL, "veryveryverty-longlonglongnamemamemeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
	assert(item);

	procstat_remove(context, item);
	item = procstat_create_directory(context, NULL, "veryveryverty-longlonglongnamemamemeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
	assert(item);
}



void inc_values_32(uint32_t *vals)
{
	int i;
	for (i = 0; i < 10; ++i)
		++vals[i];
}

void inc_values_64(uint64_t *vals)
{
	int i;
	for (i = 0; i < 10; ++i)
		++vals[i];
}


static void fetch_getter(uint16_t *object, uint16_t *out)
{
	*out = *object;
}



DEFINE_PROCSTAT_FORMATTER(uint16_t, "%u\n", decimal);
DEFINE_PROCSTAT_SIMPLE_ATTRIBUTE(uint16_t);
DEFINE_PROCSTAT_CUSTOM_FORMATTER(fetch, fetch_getter, uint16_t, "%u");



static void create_multiple_simple_stats(struct procstat_item *root, uint32_t *value_32, uint64_t *value_64, uint16_t *val16)
{
	struct procstat_item *item;
	struct procstat_simple_handle descriptors[] = {{"val_32", value_32, procstat_format_u32_decimal},
						       {"val_64", value_64, procstat_format_u64_decimal},
							"val_16", val16, procstat_format_uint16_t_fetch};
	int error;

	item = procstat_create_directory(context, root, "multiple-simple");
	assert(item);

	error = procstat_create_simple(context, item, descriptors, 3);
	assert(!error);
	error = procstat_create_uint16_t(context, root, "val16_special", val16);
	assert(!error);
}




static void create_multiple_start_end_stats(struct procstat_item *root)
{
	struct procstat_item *item;
	struct procstat_start_end_u64 start_end_u64[] = {{1,2},
							 {3,4}};

	struct procstat_start_end_u32 start_end_u32[] = {{5,6},
						         {7,8}};

	struct procstat_start_end_handle descriptors[] = {procstat_start_end_u64_handle("s1", start_end_u64[0]),
							  procstat_start_end_u64_handle("s2", start_end_u64[1]),
							  procstat_start_end_u32_handle("s3", start_end_u32[0]),
							  procstat_start_end_u32_handle("s4", start_end_u32[1])};

	int error;

	item = procstat_create_directory(context, root, "start-end");
	assert(item);

	error = procstat_create_start_end(context, item, descriptors, 4);
	assert(!error);

	error = procstat_create_start_end(context, item, descriptors, 4);
	assert(error);


	printf("Press enter Delete start end \n");
	getchar();

	procstat_remove(context, item);

	printf("Removed ... \n");
}


static void create_multiple_series(struct procstat_item *root)
{
	struct procstat_item *item;
	struct procstat_series_u64 series[10];

	struct procstat_series_u64_handle des[] = { {"s1", &series[0]},
						    {"s2", &series[1]},
						    {"s3", &series[2]},
						    {"s4", &series[3]},
						    {"s5", &series[4]},
						    {"s6", &series[5]},
						    {"s7", &series[6]},
						    {"s8", &series[7]},
						    {"s9", &series[8]},
						    {"s10", &series[9] } };

	struct procstat_series_u64_handle bad_des[] = { {"bad_s1", &series[0]},
						     	{"bad_s1", &series[1]} };

	memset(series, 0, sizeof(series));
	int error;


	item = procstat_create_directory(context, root, "series");
	assert(item);

	error = procstat_create_multiple_u64_series(context, item, des, 10);
	assert(!error);


	error = procstat_create_multiple_u64_series(context, item, bad_des, 2);
	assert(error);

	printf("Press enter to Delete series stats \n");
	getchar();

	procstat_remove(context, item);
}


static inline unsigned long long rdtsc(void)
{
	unsigned long low, high;

	asm volatile("rdtsc" : "=a" (low), "=d" (high));

	return ((low) | (high) << 32);
}



static void create_time_series(struct procstat_item *root)
{
	struct procstat_item *item;
	struct procstat_series_u64 series;
	uint64_t start;
	int i;
	int error;

	memset(&series, 0, sizeof(series));
	item = procstat_create_directory(context, root, "time_series");
	assert(item);

	error = procstat_create_u64_series(context, item, "time1", &series);
	assert(!error);

	printf("Going to submit several timepoints \n");
	getchar();
	for (i = 0; i < 20; ++i) {
		start = rdtsc();
		sleep(1);
		procstat_u64_series_add_point(&series, rdtsc() - start);
	}

	printf("Press enter to Delete series stats \n");
	getchar();

	procstat_remove(context, item);
}

static void test_create_multiple_dirs_and_files(struct procstat_item *root, uint32_t *values_32)
{
	struct procstat_item *item;
	char buffer[100];
	int i, j, k;
	int error;

	for ( i = 0; i < 10; ++i) {
		struct procstat_item *outer;
		sprintf(buffer, "outer-%d", i);

		outer = procstat_create_directory(context, root, buffer);
		assert(outer);
		for (j = 0; j < 10; ++j) {
			struct procstat_item *inner;
			sprintf(buffer, "inner-%d", j);
			inner = procstat_create_directory(context, outer, buffer);
			assert(inner);
			for (k = 0; k < 10; ++k) {
				sprintf(buffer, "value-%d", k);
				error = procstat_create_u32(context, inner, buffer , &values_32[k]);
				assert(!error);
			}
		}
	}

	printf("Press enter To inc values\n");
	getchar();
	inc_values_32(values_32);

	printf("Delete several directories outer-0s\n");

	procstat_remove_by_name(context, root, "outer-0");
	item = procstat_create_directory(context, root, "outer-0");
	assert(item);
}




int main(int argc, char **argv) {
	struct procstat_item *item;
	uint32_t values_32[10];
	uint64_t values_64[10];
	uint16_t values_16[10];
	int i;

	context = procstat_create("/tmp/blabla");
	assert(context);
	set_signal_handlers();

	for (i = 0; i < 10; ++i) {
		values_32[i] = i;
		values_64[i] = i;
		values_16[i] = i;
	}

	pthread_t inc_x_thread;
	pthread_create(&inc_x_thread, NULL, fuse_loop, context);
	test_create_dirs(context);
	test_create_multiple_dirs_and_files(NULL, values_32);
	create_multiple_simple_stats(NULL, &values_32[0], &values_64[0], &values_16[0]);
	create_multiple_start_end_stats(NULL);
	create_multiple_series(NULL);
	create_time_series(NULL);

	pthread_join(inc_x_thread, NULL);
	printf("HERE DONE!!!!\n");
	return 0;
}
