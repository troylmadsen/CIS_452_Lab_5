#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CHARS 4096
#define NUM_READERS 2
#define NUM_SEGMENTS 3

/*
 * Awaits messages stored to a shared memory segments.
 * @Version 1.0
 * @Author Troy Madsen
 * @Author Tim DeVries
 * @Date 2018/09/27
 */

/* Prototypes */
void set_shm_segment();
void set_signal_handlers();
static void sig_handler( int signum, siginfo_t* siginfo, void* context );
void detach_shared();

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

	/* Memory segment ID */
	int shm_id;

	/* Address of memory segment */
	void* return_ptr;

	/* Retrieve shared memory segment for memory segment addresses */
	if ( ( shm_id = shmget( key, 0, 0 ) ) < 0 ) {
		perror( "Shared memory retrieval failure" );
		exit( 1 );
	}

	/* Attach to the shared memory segment */
	if ( ( return_ptr = (void*)shmat( shm_id, 0, 0 ) ) == (void*)-1 ) {
		perror( "Shared memory attach failure" );
		exit( 1 );
	}

	/* Retrieve shared memory segment for messages */
	shm_message = (char*)shm_segment[0];

	/* Retrieve shared memory segment for flags */
	shm_flags = (int*)shm_segment[1];

	/* Retrieve shared memory segment for PIDs */
	shm_pids = (pid_t*)shm_segment[2];
}

/* FIXME */
void set_signal_handlers() {
	/* Signal handlers */
	struct sigaction sa;

	sigemptyset( &sa.sa_mask );
	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &sig_handler;

	if ( sigaction( SIGINT, &sa, NULL ) == -1 ) {
		perror( "sigaction failure" );
		exit( 1 );
	}

	if ( sigaction( SIGUSR1, &sa, NULL ) == -1 ) {
		perror( "sigaction failure" );
		exit( 1 );
	}
}

/*
 * Handles the signals sent to the program.
 * @param signum Signal number
 * @param siginfo Signal info
 * @param context Function to call at signal delivery
 */
static void sig_handler( int signum, siginfo_t* siginfo, void* context ) {
	/* Shut down others */
	if ( signum == SIGINT ) {
		printf( "\nSignalling other to shut down\n" );
		for ( int i = 0; i < NUM_READERS + 1; i++ ) {
			if ( i == reader_num ) {
				kill( shm_pids[i], SIGUSR1 );
			}
		}

		printf( "Shutting down\n" );

		detach_shared();

		exit( 0 );
	}

	/* We are told to shut down */
	if ( signum == SIGUSR1 ) {
		printf( "\nShutting down\n" );

		detach_shared();

		exit( 0 );
	}
}

/*
 * Detaches from all shared memory segments.
 */
void detach_shared() {
	/* Detach from shared memory segment for message */
	if ( shmdt( shm_message ) == -1 ) {
		perror( "Shared memory segment message detach failure\n" );
	}
	
	/* Detach from shared memory segment for flags */
	if ( shmdt( shm_flags ) == -1 ) {
		perror( "Shared memory segment flags detach failure\n" );
	}

	/* Detach from shared memory segment for pids */
	if ( shmdt( shm_pids ) == -1 ) {
		perror( "Shared memory segment pids detach failure\n" );
	}

	/* Detach from shared memory segment for segments */
	if ( shmdt( shm_segment ) == -1 ) {
		perror( "Shared memory segment segment detach failure\n" );
	}
}
