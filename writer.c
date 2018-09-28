#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CHARS 4096
#define NUM_READERS 2

/*
 * Writes messages to a shared memory segments.
 * @Version 1.0
 * @Author Troy Madsen
 * @Author Tim DeVries
 * @Date 2018/09/27
 */

/* Prototypes */
void set_shm_segment();
void* malloc_shared( int size, key_t key );
void set_signal_handlers();
static void sig_handler( int signum, siginfo_t* siginfo, void* context );

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

/*FIXME */
int main() {
	/* Set up shared memory segment */
	set_shm_segment();

	/* Store my PID for others */
	shm_pids[0] = getpid();

	/* Set up signal handlers */
	set_signal_handlers();

	/* Getting and sending input to readers */
	char* input;
	int not_read;
	while ( 1 ) {
		/* Get user input */
		input = (char *)malloc( MAX_CHARS * sizeof( char * ) );
		printf( ">" );
		fgets( input, MAX_CHARS, stdin );

		/* Trim \n off input */
		int i = 0;
		while ( input[i++] != '\n' );
		input[i - 1] = '\0';

		/* Wait until previous message read by all readers */
		/* Reset not_read */
		not_read = 0;

		while ( not_read ) {
			for ( i = 0; i < NUM_READERS; i++ ) {
				not_read = not_read | shm_flags[i];
			}
		}

		/* Set reader flags */
		for ( i = 0; i < NUM_READERS; i++ ) {
			shm_flags[i] = 1;
		}

		/* Write message to shared memory segment */
		shm_message = input;
	}

	return 0;
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
	shm_constants[0] = MAX_CHARS;
	shm_constants[1] = NUM_READERS;

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

/* FIXME */
void* malloc_shared( int size, key_t key ) {
	/* Memory segment ID */
	int shm_id;

	/* Address of memory segment */
	void* return_ptr;

	/* Allocate shared memory */
	if ( (shm_id = shmget( key, size, ( S_IRUSR | S_IWUSR ) ) ) < 0 ) {
		perror( "Shared memory allocation failure" );
		exit( 1 );
	}

	/* Attach to the shared memory segment */
	if ( ( return_ptr = (void*)shmat( shm_id, 0, 0 ) ) == (void*)-1 ) {
		perror( "Shared memory attach failure" );
		exit( 1 );
	}

	/* Set memory to be removed upon program termination */
	shmctl( shm_id, IPC_RMID, NULL );

	return return_ptr;
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
		for ( int i = 1; i < NUM_READERS + 1; i++ ) {
			kill( shm_pids[i], SIGUSR1 );
		}

		printf( "Shutting down\n" );
		exit( 0 );
	}

	/* We are told to shut down */
	if ( signum == SIGUSR1 ) {
		printf( "\nShutting down\n" );
		exit( 0 );
	}
}
