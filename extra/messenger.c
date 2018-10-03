#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CHARS 4096
#define NUM_MEMBERS 2
#define NUM_SEGMENTS 3
#define PATH "./writer.c"
#define PROJ_ID 1

/*
 * Writes messages to a shared memory segments.
 * @Version 1.0
 * @Author Troy Madsen
 * @Author Tim DeVries
 * @Date 2018/09/27
 */

/* Prototypes */
void set_shm_segment();
void get_number();
void* malloc_shared( int size, key_t key );
void set_signal_handlers();
static void sig_handler( int signum, siginfo_t* siginfo, void* context );
void detach_shared();
void message();
void* read_messages();

/* Globals */

/* Size of shared memory */
int shm_size;

/* Shared memory segment for memory segment addresses */
char* shm_segment;
int remove_id;

/* Shared memory segment for message */
char* shm_message;

/* Shared memory segment for flags */
int* shm_flags;

/* Shared memory segment for PIDS */
pid_t* shm_pids;

/* Number of this reader */
int reader_num = -1;

/*
 * Sends messages to listening readers.
 */
int main() {
	/* Set size of shared memory segment */
	shm_size = ( MAX_CHARS * sizeof( char ) ) + ( NUM_MEMBERS * sizeof( int ) ) + ( (NUM_MEMBERS + 1) * sizeof( pid_t ) );

	/* Get reader number */
	get_number();

	/* Set up shared memory segment */
	set_shm_segment();

	/* Store my PID for others */
	shm_pids[0] = getpid();

	/* Set up signal handlers */
	set_signal_handlers();

	//FIXME
	/* Set read flags */
	for ( int i = 0; i < NUM_MEMBERS; i++ ) {
		shm_flags[i] = 1;
	}

	/* Start thread to listen for messages */
	pthread_t thread;
	if ( pthread_create( &thread, NULL, &read_messages, NULL ) ) {
		perror( "Thread creation error" );
		exit( 1 );
	}

	/* Sending message to readers */
	message();

	return 0;
}

/*
 * Creates a shared memory segment and additional segments for all processes to communicate.
 */
void set_shm_segment() {
	/* Create similar key between processes */
	key_t key = ftok( PATH, PROJ_ID );

	/* Allocate shared memory segment for communication */
	shm_segment = (char*)malloc_shared( shm_size, key );

	/* Get address for message */
	shm_message = (char*)( shm_segment + 0 );

	/* Get address for flags */
	shm_flags = (int*)( shm_message + ( MAX_CHARS * sizeof( char ) ) );

	/* Get address for PIDs */
	shm_pids = (pid_t*)( shm_flags + ( NUM_MEMBERS * sizeof( int ) ) );
}

/*
 * Gets the number of this reader from the user.
 */
void get_number() {
	/* Get reader number from user */
	char input[16];
	while ( 0 > reader_num || reader_num > NUM_MEMBERS - 1 ) {
		printf( "Enter reader number (0 - %d): ", NUM_MEMBERS - 1 );
		fgets( input, 16, stdin );
		reader_num = atoi( input );
	}
}


/*
 * Allocates a shared memory segment of the specified size in bytes.
 * @param size Number of bytes to allocate of shared memory.
 * @param key Key identifier of the shared memory segment.
 * @return The address of the shared memory segment.
 */
void* malloc_shared( int size, key_t key ) {
	/* Memory segment ID */
	int shm_id;

	/* Address of memory segment */
	pid_t* return_ptr;

	/* Allocate shared memory */
	if ( ( shm_id = shmget( key, size, ( IPC_CREAT | S_IRUSR | S_IWUSR ) ) ) < 0 ) {
		perror( "Shared memory allocation failure" );
		exit( 1 );
	}

	/* Attach to the shared memory segment */
	if ( ( return_ptr = (void*)shmat( shm_id, NULL, 0 ) ) == (void*)-1 ) {
		perror( "Shared memory attach failure" );
		exit( 1 );
	}
	printf( "Attached to shared memory segment ID: %d\n", shm_id );

	/* Save segment ID for removal later */
	remove_id = shm_id;

	return return_ptr;
}

/*
 * Set up signal handlers.
 */
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
		printf( "\nSignalling others to shut down\n" );
		for ( int i = 0; i < NUM_MEMBERS + 1; i++ ) {
			if ( i != reader_num ) {
				kill( shm_pids[i], SIGUSR1 );
			}
		}

		printf( "Shutting down\n" );
	
		/* Set memory to be removed upon program termination */
		shmctl( remove_id, IPC_RMID, (struct shmid_ds *)NULL );

		/* Detach from shared memory segment */
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
	/* Detach from shared memory segment for segments */
	if ( shmdt( shm_segment ) == -1 ) {
		perror( "Shared memory segment segment detach failure\n" );
	}
}

/*
 * Continually reads user input and sends message to readers.
 */
void message() {
	char* input;
	int all_read;

	/* Run until shutdown */
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
		all_read = 0;
		while ( !all_read ) {
			all_read = 1;
			for ( i = 0; i < NUM_MEMBERS; i++ ) {
				all_read = all_read & shm_flags[i];
			}
		}

		/* Write message to shared memory segment */
		strcpy( shm_message, input );

		/* Set reader flags */
		for ( i = 0; i < NUM_MEMBERS; i++ ) {
			shm_flags[i] = 0;
		}
	}
}

/*
 * Reads message from others.
 */
void* read_messages() {
	/* Run until shut down */
	while ( 1 ) {

		/* Wait for message */
		while ( shm_flags[reader_num] );

		/* Read message */
		printf( "%s\n", shm_message );

		/* Set flag indicating message read */
		shm_flags[reader_num] = 1;
	}
}
