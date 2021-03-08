/*
 *  Find duplicate files.
 */

#define _GNU_SOURCE
#define _DEFAULT_SOURCE

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>

#include "mem.h"
#include "log.h"
#include "timer.h"
#include "util.h"

#include "compare.h"
#include "find.h"
#include "list-file.h"

#if !defined(PACKAGE_NAME) || !defined(PACKAGE_VERSION)
# error PACKAGE_VERSION not defined.
#endif

#if !defined(PACKAGE_BUGREPORT)
# error PACKAGE_BUGREPORT not defined.
#endif

const char *version_string = "find-dupes (" PACKAGE_NAME ") " PACKAGE_VERSION;

static void print_version(void)
{
	printf("%s\n", version_string);
}

static void print_bugreport(void)
{
	fprintf(stderr, "Send bug reports to: " PACKAGE_BUGREPORT ".\n");
}

enum opt_value {opt_undef = 0, opt_yes, opt_no};

struct opts {
	char *list_dir;
	enum opt_value file_list;
	unsigned int jobs;
	unsigned int buckets;
	enum opt_value help;
	enum opt_value verbose;
	enum opt_value version;
	struct list src_dir_list;
};

static void print_usage(const struct opts *opts)
{
	print_version();

	fprintf(stderr,
		"find-dupes - Search directories and generate lists of unique and duplicate files found.\n"
		"Usage: find-dupes [flags] src-directory [src-directory]...\n"
		"Option flags:\n"
		"  -l --list-dir   - Output lists to this directory. Default: '%s'.\n"
		"  -f --file-list  - Generate a list of all files found.\n"
		"  -j --jobs       - Number of jobs to run in parallel. Default: '%u'.\n"
		"  -b --buckets    - Hash bucket scale factor. Default: '%u'.\n"
		"  -h --help       - Show this help and exit.\n"
		"  -v --verbose    - Verbose execution.\n"
		"  -V --version    - Display the program version number.\n"
		, opts->list_dir, opts->jobs, opts->buckets);

	print_bugreport();
}

struct src_dir {
	struct list_entry list_entry;
	char path[];
};

static void src_dir_add(struct list *src_dir_list, const char *path)
{
	struct src_dir *sd;
	size_t path_len = strlen(path);

	sd = mem_alloc_zero(sizeof(*sd) + path_len + 1);

	memcpy(sd->path, path, path_len);
	debug("'%s'\n", sd->path);

	list_add_tail(src_dir_list, &sd->list_entry);
}

static void opts_init(struct opts *opts, const char *date)
{
	int result;

	*opts = (struct opts) {
		.list_dir = NULL,
		.file_list = opt_no,
		.buckets = 1,
		.help = opt_no,
		.verbose = opt_no,
		.version = opt_no,
	};

	opts->list_dir = mem_strdupcat("/tmp/find-dupes-", date);

	result = sysconf(_SC_NPROCESSORS_ONLN);

	if (result < 0) {
		log("ERROR: NPROCESSORS_ONLN failed: %s\n", strerror(errno));
		opts->jobs = 1;
	} else {
		opts->jobs = result;
	}	
}

static int opts_parse(struct opts *opts, int argc, char *argv[])
{
	static const struct option long_options[] = {
		{"list-dir",   required_argument, NULL, 'l'},
		{"file-list",  no_argument,       NULL, 'f'},
		{"jobs",       required_argument, NULL, 'j'},
		{"buckets",    required_argument, NULL, 'b'},
		{"help",       no_argument,       NULL, 'h'},
		{"verbose",    no_argument,       NULL, 'v'},
		{"version",    no_argument,       NULL, 'V'},
		{ NULL,        0,                 NULL, 0},
	};
	static const char short_options[] = "l:fj:b:hvV";

	if (1) {
		int i;

		debug("argc = %d\n", argc);
		for (i = 0; i < argc; i++) {
			debug("  %d: %p = '%s'\n", i, &argv[i], argv[i]);
		}
	}

	while (1) {
		int c = getopt_long(argc, argv, short_options, long_options,
			NULL);

		if (c == EOF) {
			//debug("  getopt_long: 'EOF'\n");
			break;
		}
		//debug("  getopt_long: '%c'\n", c);

		switch (c) {
		case 'l': {
			//debug("  b: %p = '%s'\n", optarg, optarg);

			if (!optarg) {
				fprintf(stderr,
					"find-dupes: ERROR: Missing required argument <list_dir>.'\n");
				opts->help = opt_yes;
				return -1;
			}

			mem_free(opts->list_dir);
			opts->list_dir = mem_strdup(optarg);
			break;
		}
		case 'f':
			opts->file_list = opt_yes;
			break;
		case 'j':
			opts->jobs = to_unsigned(optarg);
			if (opts->jobs == UINT_MAX) {
				opts->help = opt_yes;
				return -1;
			}
			break;
		case 'b':
			opts->buckets = to_unsigned(optarg);
			if (opts->buckets == UINT_MAX) {
				opts->help = opt_yes;
				return -1;
			}
			break;
		case 'h':
			opts->help = opt_yes;
			break;
		case 'v':
			opts->verbose = opt_yes;
			set_debug_on(true);
			break;
		case 'V':
			opts->version = opt_yes;
			break;
		default:
			log("Internal error: %c:   %p = '%s'\n", c, optarg, optarg);
			assert(0);
			exit(EXIT_FAILURE);
		}
	}

	list_init(&opts->src_dir_list, "src_dir_list");

	for ( ; optind < argc; optind++) {
		src_dir_add(&opts->src_dir_list, argv[optind]);
	}

	if (0) {
		struct src_dir *sd;

		list_for_each(&opts->src_dir_list, sd, list_entry) {
			debug("src: '%s'\n", sd->path);
		}
	}

	return 0;
}

struct sig_events {
	volatile sig_atomic_t alarm;
	volatile sig_atomic_t term;
};

static struct sig_events sig_events = {
	.alarm = 0,
	.term = 0,
};

static void SIGALRM_handler(int signum)
{
	//debug("SIGALRM\n");
	sig_events.alarm = 1;
	signal(signum, SIGALRM_handler);
}

static void SIGINT_handler(int signum)
{
	//debug("SIGINT\n");
	sig_events.term = 1;
	signal(signum, SIGINT_handler);
}

static void SIGTERM_handler(int signum)
{
	//debug("SIGTERM\n");
	sig_events.term = 1;
	signal(signum, SIGTERM_handler);
}

static bool check_for_signals(void)
{
	if (sig_events.term) {
		//debug("term\n");
		return true;
	}

	if (sig_events.alarm) {
		sig_events.alarm = 0;
		//debug("alarm\n");
	}
	return false;
}

static void print_file_header(FILE *fp, const char *str)
{
	fprintf(fp, "# %s\n# ", version_string);
	print_current_time(fp);
	fprintf(fp, "\n# %s\n\n", str);
}

static void print_file_header_count(FILE *fp, const char *str,
	unsigned int count)
{
	fprintf(fp, "# %s\n# ", version_string);
	print_current_time(fp);
	fprintf(fp, "\n# %s - %u files.\n\n", str, count);
}

static void print_result(const char *result, const struct timer *timer)
{
	char str[64];
	
	timer_duration_str(timer, str, sizeof(str));

	fprintf(stderr, "find-dupes: Done: %s, %s.\n\n", result, str);
}

static void empty_list_print(struct list *empty_list, FILE *fp, bool size)
{
	struct hash_table_entry *hte;

	list_for_each(empty_list, hte, list_entry) {
		struct file_data *data = (struct file_data *)hte->data;

		fprintf(fp, "%s%s\n", (size ? "0 " : ""), data->name);
	}
}

static void empty_list_clean(struct list *empty_list)
{
	struct hash_table_entry *hte;
	struct hash_table_entry *hte_safe;

	list_for_each_safe(empty_list, hte, hte_safe, list_entry) {
		file_table_entry_clean(hte);
	}
}

static void compare_queue_print(struct work_queue *wq, unsigned int empty_count)
{
	struct compare_counts totals = {0};
	struct work_item *wi;

	if (!list_is_empty(&wq->ready_list)) {
		list_for_each(&wq->ready_list, wi, list_entry) {
			log("ready_list: wi = %u\n", wi->id);
		}
		on_error("ready_list not empty.\n");
	}

	list_for_each(&wq->done_list, wi, list_entry) {
		struct compare_counts *result;

		assert(wi->result);
		result = wi->result;

		if (0) {
			debug("wi-%u result: total = %u, dupe = %u, unique = %u\n",
				wi->id, result->total, result->dupes,
				result->unique);
		}

		totals.total += result->total;
		totals.dupes += result->dupes;
		totals.unique += result->unique;
	}

	//debug("Processed %u files.\n", totals.total);

	fprintf(stderr, "find-dupes: Found %u unique files, %u duplicate files, %u empty files.\n",
		totals.unique, totals.dupes, empty_count);
}

static void compare_queue_clean(struct work_queue *wq)
{
	struct work_item *wi_safe;
	struct work_item *wi;

	if (!list_is_empty(&wq->ready_list)) {
		list_for_each(&wq->ready_list, wi, list_entry) {
			log("ready_list: wi = %u\n", wi->id);
		}
		on_error("ready_list not empty.\n");
	}

	list_for_each_safe(&wq->done_list, wi, wi_safe, list_entry) {
		mem_free(wi);
	}
}

static unsigned int get_sleep_time(unsigned int file_count)
{
	if (file_count < 15000) {
		return 1;
	}
	if (file_count < 50000) {
		return 2;
	}
	if (file_count < 100000) {
		return 3;
	}
	if (file_count < 500000) {
		return 4;
	}
	if (file_count < 1000000) {
		return 5;
	}
	return 6;
}

int main(int argc, char *argv[])
{
	struct src_dir *sd_safe;
	struct src_dir *sd;
	struct work_queue *wq;
	struct hash_table *ht;
	struct timer timer;
	struct opts opts;
	unsigned int i;
	int result;

	timer_start(&timer);
	opts_init(&opts, timer_start_str(&timer));

	if (opts_parse(&opts, argc, argv)) {
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	if (opts.help == opt_yes) {
		print_usage(&opts);
		return EXIT_SUCCESS;
	}

	if (opts.version == opt_yes) {
		print_version();
		return EXIT_SUCCESS;
	}

	if (!opts.list_dir || !opts.list_dir[0]) {
		fprintf(stderr,
			"find-dupes: ERROR: Missing required flag --list-dir.'\n");
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	if (access(opts.list_dir, F_OK)) {
		result = mkdir(opts.list_dir, S_IRWXU | S_IRWXG | S_IRWXO);

		if (result) {
			fprintf(stderr, "ERROR: mkdir '%s' failed: %s\n",
				opts.list_dir, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	if (0) {
		thread_pool_test();
		exit(EXIT_SUCCESS);
	}

	if (0) {
		work_queue_test();
		exit(EXIT_SUCCESS);
	}

	if (0) {
		timer_duration_test();
		exit(EXIT_SUCCESS);
	}

	if (0) {
		char *log_path;

		log_path = mem_strdupcat(opts.list_dir, "/find-dupes.log");
		set_log_path(log_path);
		mem_free(log_path);
	}

	if (list_is_empty(&opts.src_dir_list)) {
		fprintf(stderr,
			"find-dupes: ERROR: Missing source directories.'\n");
		print_usage(&opts);
		return EXIT_FAILURE;
	}

	list_for_each(&opts.src_dir_list, sd, list_entry) {
		check_exists(sd->path);
	}

	signal(SIGALRM, SIGALRM_handler);
	signal(SIGINT, SIGINT_handler);
	signal(SIGTERM, SIGTERM_handler);

	if (1) {
		ht = hash_table_init(1024UL * opts.buckets);
		wq = work_queue_alloc(opts.jobs);
	} else {
		debug("jobs = 1, hash count = 10\n");
		ht = hash_table_init(10);
		wq = work_queue_alloc(1);
	}

	fprintf(stderr, "find-dupes: Finding files...\n");

	list_for_each(&opts.src_dir_list, sd, list_entry) {

		result = find_files(wq, ht, check_for_signals, sd->path);

		if (result) {
			debug("find_files failed: '%s', %d\n", sd->path, result);
			goto exit_clean;
		}
		//debug("find_files OK: '%s'\n", sd->path);
	}

	list_for_each_safe(&opts.src_dir_list, sd, sd_safe, list_entry) {
		list_remove(&sd->list_entry);
		mem_free(sd);
	}

	i = 0;
	while (!list_is_empty(&wq->ready_list)) {
		i++;
		if (check_for_signals()) {
			debug("find wait %u (got signal)\n", i);
			sleep(1);
		} else {
			debug("find wait %u\n", i);
			sleep(1);
		}
	}

	if (check_for_signals()) {
		debug("find signal cleanup\n");
		work_queue_empty_ready_list(wq);
		result = -1;
		goto exit_clean;
	}

	if (1) {
		FILE *empty_fp = list_file_open(opts.list_dir, "/empty.lst");

		print_file_header_count(empty_fp, "Empty List",
			list_item_count(&ht->extras));

		empty_list_print(&ht->extras, empty_fp, false);

		fclose(empty_fp);
	}

	if (opts.file_list == opt_yes) {
		FILE *files_fp = list_file_open(opts.list_dir, "/files.lst");

		print_file_header(files_fp, "Files List");

		empty_list_print(&ht->extras, files_fp, true);
		result = list_file_print(ht, files_fp);

		if (result) {
			fclose(files_fp);
			goto exit_clean;
		}
	}

	if (1) {
		struct compare_file_pointers fps;
		unsigned int empty_count;
		unsigned int sleep_time;
		unsigned int f_count;

		f_count = file_count(ht);

		sleep_time = get_sleep_time(f_count);

		fprintf(stderr, "find-dupes: Comparing %u files...\n",
			f_count);

		fps.dupes = list_file_open(opts.list_dir, "/dupes.lst");
		print_file_header(fps.dupes, "Dupes List");

		fps.unique = list_file_open(opts.list_dir, "/unique.lst");
		print_file_header(fps.unique, "Unique List");

		compare_files(wq, ht, check_for_signals, &fps);

		i = 0;
		while (!list_is_empty(&wq->ready_list)) {
			i++;
			if (check_for_signals()) {
				debug("compare wait %u (got signal)\n", i);
				sleep(1);
			} else {
				
				debug("compare wait %u\n", i);
				sleep(sleep_time);
			}
		}

		fclose(fps.dupes);
		fclose(fps.unique);

		if (check_for_signals()) {
			debug("compare signal cleanup\n");
			work_queue_empty_ready_list(wq);
			result = -1;
			goto exit_clean;
		}

		empty_count = list_item_count(&ht->extras);
		empty_list_clean(&ht->extras);

		compare_queue_print(wq, empty_count);
	}

exit_clean:
	debug("exit_clean\n");

	compare_queue_clean(wq);
	work_queue_delete(wq);

	timer_stop(&timer);

	if (result) {
		print_result("Failed", &timer);
		return EXIT_FAILURE;
	}

	fprintf(stderr, "find-dupes: Lists in '%s'.\n", opts.list_dir);
	print_result("Success", &timer);
	return EXIT_SUCCESS;
}
