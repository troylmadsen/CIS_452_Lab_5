#include <signal.h>
#include <sys/shm.h>
#include <sys/stat.h>

/*
 * Awaits messages stored to a shared memory segments.
 * @Version 1.0
 * @Author Troy Madsen
 * @Author Tim DeVries
 * @Date 2018/09/27
 */

/* Globals */

/* Shared memory segment for memory segment addresses */
void** shm_segment;
int num_shm_segments = 4;

/* Shared memory segment for constants */
int* shm_constants;
int num_constants = 2;

/* Shared memory segment for message */
char* shm_message;

/* Shared memory segment for flags */
int* shm_flags;

/* Shared memory segment for PIDS */
pid_t* shm_pids;

/* Constant from writer for max message size */
int MAX_CHARS;

/* Constant from writer for number of readers */
int NUM_READERS;

/* Number of this reader */
int reader_num;

/* FIXME */
int main() {
	/* Set up shared memory segment */
	set_shm_segment();

	/* Get reader number */
	//FIXME

	/* Store my PID for others */
	shm_pids[reader_num] = getpid();

	/* Set up signal handlers */
	set_signal_handlers();

	//FIXME
}

/* FIXME */
void set_shm_segment() {
	/* Create shared memory segment ID */
	char* path = "./writer.c";
	int proj_id = 1;
	key_t key = ftok( path, proj_id );

	/* Allocate shared memory segment for memory segment addresses */
	shm_segment = (void**)malloc_shared( num_shm_segments * sizeof( void* ), key );

	/* Allocates shared memory segment for constants */
	shm_segment[0] = malloc_shared( num_constants * sizeof( int ), IPC_PRIVATE );
	shm_constants = (int*)shm_segment[0];
	MAX_CHARS = shm_constants[0];
	NUM_READERS = shm_constants[1];

	/* Allocate shared memory segment for messages */
	shm_segment[1] = malloc_shared( MAX_CHARS * sizeof( char ), IPC_PRIVATE );
	shm_message = (char*)shm_segment[1];

	/* Allocate shared memory segment for flags */
	shm_segment[2] = malloc_shared( NUM_READERS * sizeof( int ), IPC_PRIVATE );
	shm_flags = (int*)shm_segment[2];

	/* Allocate shared memory segment for PIDs */
	shm_segment[3] = malloc_shared( ( NUM_READERS + 1 ) * sizeof( pid_t ), IPC_PRIVATE );
	shm_pids = (pid_t*)shm_segment[3];
}
