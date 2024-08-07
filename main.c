#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <termios.h>

#define exit_if_true(a,b, ...) ({\
	if (a) {\
		printf((b), ##__VA_ARGS__); \
		exit(1); \
	}\
})

#define exit_if_null(a,b, ...) ({\
	if (a == NULL) {\
		printf((b), ##__VA_ARGS__); \
		exit(1); \
	}\
})

#define exit_if_m1(a,b, ...) ({\
	if (a == -1) {\
		printf((b), ##__VA_ARGS__); \
		exit(1); \
	}\
})

struct Pattern {
	size_t size;
	char *pattern;
};

struct Processors {
	size_t size;
	size_t capacity;
	volatile size_t cursor;
};

FILE *output;
size_t *offsets; 
char *source_path, *patterns_path;
size_t patterns_len = 0, exact = 0, not_print_patterns=0;
struct Pattern *patterns;
struct Processors *processors;

void usage() {
	printf("Usage:\n\n-s <SOURCE> -p <PATTERNS> -o <OUTPUT> [OPTIONS]\n\nOptions:\n\n-t\t\tnum of threads\n-e\t\tuse strcmp for patterns (default strstr)\n-x\t\tdon't print patterns\n-h\t\thelp\n");
	exit(0);
}

void init_source() {
	FILE *file = fopen(source_path,"r"); 
	exit_if_null(file,"source file can't open");
	exit_if_true(fseek(file, 0L ,SEEK_END),"fseek error on source file\n");

	long size = ftell(file);
	exit_if_m1(size,"cant get size of source file\n");
	rewind(file);

	printf("The file size is %ld bytes\n",size);

	offsets = (size_t*) malloc(sizeof(size_t) * (processors->capacity));
	exit_if_null(offsets,"malloc error on line %d",__LINE__);
	size_t factor = size / processors->capacity;

	offsets[0] = factor;

	for (size_t i=1; i < processors->capacity; i++) {
		offsets[i] = factor * (i+1);
	}

	for (size_t i=1;i < processors->size; i++) {
		for (size_t ii=offsets[i]; ii < size ; ii++) {
			fseek(file,ii,SEEK_SET);
			if (fgetc(file) == '\n') {
				offsets[i] = ii+1;
				if (offsets[i+1] < offsets[i]) offsets[i+1] = offsets[i]; 
				break;
			}
		}
	}

	fclose(file);
}

void init_patterns() {
	FILE *file_ = fopen(patterns_path,"r");

	exit_if_null(file_,"can't open patterns file\n");
	int file = fileno(file_);

	char buff[1] = {0};

	while (read(file, &buff, 1)) {
		if (buff[0] == '\n') {
			patterns_len++;
		}
	}

	patterns = (struct Pattern*) malloc(patterns_len * sizeof(struct Pattern));

	exit_if_null(patterns,"malloc error on line %d\n",__LINE__);
	rewind(file_);

	size_t len = 0, counter = 0, begin = 0;

	while (read(file, &buff, 1)) {
		len++;
		if (buff[0] == '\n') {
			char *p = malloc(len);
			exit_if_null(p,"malloc error on line %d\n",__LINE__);
			patterns[counter] = (struct Pattern) {len,p};
			exit_if_m1(pread(file,patterns[counter].pattern,len,begin),"can't read the pattern\n");
			p[len-1] = '\0';
			begin += len;
			counter++;
			len = 0;
		}
	}

	if (!not_print_patterns) {
		for(size_t i=0;i<patterns_len;i++) {
			printf("pattern[%ld] %s\n",i,patterns[i].pattern);
		}
	}

	fclose(file_);
}

struct termios term, restore;
size_t cursor_x = 0, cursor_y = 0;

void fetch_pos() {
	tcgetattr(0, &term);
	tcgetattr(0, &restore);
	term.c_lflag &= ~(ICANON|ECHO);
	tcsetattr(0, TCSANOW, &term);


	write(1, "\033[6n", 4);
	size_t buff[100] = {0};
	size_t c,counter = 0;

	do {
		read(0,&c,1);
		buff[counter] = c;
		counter++;
	}
	while(c != 'R');

	tcsetattr(0, TCSANOW, &restore);

	size_t i = counter,pow;

	for(i -= 2, pow = 1; buff[i] != ';'; i--, pow *= 10){
		cursor_x = cursor_x + ( buff[i] - '0' ) * pow;
	}

	for(i--, pow = 1; buff[i] != '['; i--, pow *= 10){
		cursor_y = cursor_y + ( buff[i] - '0' ) * pow;
	}
}

volatile size_t *status;

void* status_init() {
	size_t ok = 0;

	while(ok < processors->capacity) {
		for(size_t i=0;i < processors->capacity; i++) {		
			ok += status[i];
		}

		sleep(1);
	}
	fflush(stdout);

	for (size_t i=0;i < processors->capacity;i++) {		
		printf("Thread[%ld] %ld\n",i, processors[i].cursor);
	}

	fflush(stdout);

	fetch_pos();

	while (1) {
		sleep(1);

		for (size_t i=0;i < processors->capacity;i++) {
			size_t ii = processors->capacity - i;
			printf("\x1b[%ld;0H\x1b[2KThread[%ld] %ld",cursor_y - ii, i, processors[i].cursor);
		}

		fflush(stdout);
	}


	return NULL;
}

void check(char *src,size_t cursor, size_t len) {
	for (size_t i = 0; i < patterns_len; i++) {

		if (exact && strcmp(src,patterns[i].pattern) != 0) continue;

		if(!exact && strstr(src,patterns[i].pattern) == NULL) continue;

		fprintf(output,"[%ld]\n%s\n\n",cursor - len,src);
	}
}

void *worker(void *_i) {
	size_t i = (size_t)_i;
	size_t begin, end;

	FILE *file = fopen(source_path,"rb"); 
	exit_if_null(file,"source file can't open\n");

	if (i == 0) {
		begin = 0;
		end = offsets[0];
	}
	else if(i == processors->size) {
		begin = offsets[processors->size - 1];
		end = offsets[processors->size];
	}
	else {
		begin = offsets[i-1];
		end = offsets[i];
	}

	exit_if_true(fseek(file, begin ,SEEK_SET),"fseek error on line %d\n",__LINE__);

	printf("Thread[%zu] searchs %zu to %zu\n",i, begin,end);

	char *src = malloc(1);
	if (src == NULL) {
		printf("malloc error on thread %ld. The thread terminated\n",i);
		return NULL;
	}

	size_t len = 0, cursor = begin;
	int fd = fileno(file);
	char c;

	status[i] = 1;

	size_t previous_src_len = 0;

	while ((c = fgetc(file)) != -1) {
		if (c == EOF) {
			break;
		}

		if (cursor > end) {
			break;
		}

		len++;

		if (c == '\n') {

			cursor += len;
			if (previous_src_len < len) {
				src = realloc(src,len);
				if (src == NULL) {
					printf("\nrealloc error at %ld",cursor);
					len = 0;
					continue;
				}
			}

			if (pread(fd,src,len,cursor) == -1) {
				printf("\ncan't read the line at %ld",cursor);
				len = 0;
				continue;
			}

			src[len-1] = '\0';
			processors[i].cursor = cursor;
			check(src,cursor,len);
			previous_src_len = len;
			len = 0;

		}
	}

	fclose(file);
	free(src);

	return NULL;
}

int main(int argc, char *argv[]) { 

	int opt;
	size_t t = 0;

	while ((opt = getopt(argc, argv, "s:p:o:t:exh")) != -1 ) {
		switch (opt) {
			case 's':
				source_path = optarg;
				break;
			case 'p':
				patterns_path = optarg;
				break;
			case 'o':
				output = fopen(optarg,"wr");	
				exit_if_null(output,"output file error\n");
				break;
			case 't':
				t = atoi(optarg);
				break;
			case 'e':
				exact = 1;
				break;
			case 'x':
				not_print_patterns = 1;
				break;
			case 'h':
				usage();
				break;

		}

	}

	if (source_path == NULL || patterns_path == NULL || output == NULL) {
		usage();
	}

	processors = malloc(sizeof(struct Processors));

	processors->capacity = sysconf(_SC_NPROCESSORS_ONLN);

	if (t > 0 && t < processors->capacity) processors->capacity = t;

	processors->size = processors->capacity - 1;
	status = calloc(processors->capacity,sizeof(size_t));

	init_source();
	init_patterns();

	pthread_t thread_id_;
	pthread_create(&thread_id_, NULL, status_init,NULL);

	if (processors->capacity > 1) {
		pthread_t threads[processors->capacity];

		for (size_t i=0;i < processors->capacity;i++) {
			pthread_t thread_id;
			exit_if_true(pthread_create(&thread_id, NULL, worker,(void*) i),"thread[%ld] not created\n",i);
			threads[i] = thread_id;
			while(!status[i]) usleep(50);
		}

		for (size_t i=0;i < processors->capacity;i++) {
			exit_if_true(pthread_join(threads[i],NULL),"not joined to thread[%ld]\n",i);
		}
	}else {
		worker(0);
	}

	for (size_t i=0;i < processors->capacity;i++) {
		printf("\nThread[%ld] done at %ld",i, processors[i].cursor);
	}

	free(patterns);

	printf("\n");

	return 0;	
}
