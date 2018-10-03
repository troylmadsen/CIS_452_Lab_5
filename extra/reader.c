#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_CHARS 4096
#define NUM_READERS 2
#define NUM_SEGMENTS 3
#define PATH "./writer.c"
#define PROJ_ID 1

/*
 * Awaits messages stored to a shared memory segment.
 * @Version 1.0
 * @Author Troy Madsen
 * @Author Tim DeVries
 * @Date 2018/09/27
 */

/* Prototypes */
void set_shm_segment();
void get_number();
void set_signal_handlers();
static void sig_handler( int signum, siginfo_t* siginfo, void* context );
void detach_shared();
void read_messages();

/* Globals */

/* Size of shared memory */
int shm_size;

/* Shared memory segment for memory segment addresses */
char* shm_segment;

/* Shared memory segment for message */
char* shm_message;

/* Shared memory segment for flags */
int* shm_flags;

/* Shared memory segment for PIDS */
pid_t* shm_pids;

/* Number of this reader */
int reader_num = -1;

/*
 * Listens for messages from writer.
 */
int main() {
	/* Set size of shared memory segment */
	shm_size = ( MAX_CHARS * sizeof( char ) ) + ( NUM_READERS * sizeof( int ) ) + ( (NUM_READERS + 1) * sizeof( pid_t ) );

	/* Set up shared memory segment */
	set_shm_segment();

	/* Get reader number */
	get_number();

	/* Store my PID for others */
	shm_pids[reader_num] = getpid();

	/* Set up signal handlers */
	set_signal_handlers();

	/* Read messages from writer */
	read_messages();
}

/*
 * Attaches to a shared memory segment to read messages and flags from.
 */
void set_shm_segment() {
	/* Create shared memory segment ID */
	key_t key = ftok( PATH, PROJ_ID );

	/* Memory segment ID */
	int shm_id;

	/* Address of memory segment */
	char* return_ptr;

	/* Retreive shared memory segment for communication */
	if ( ( shm_id = shmget( key, shm_size, ( S_IRUSR | S_IWUSR ) ) ) < 0 ) {
		perror( "Shared memory allocation failure" );
		exit( 1 );
	}

	/* Attach to the shared memory segment */
	if ( ( return_ptr = (char*)shmat( shm_id, NULL, 0 ) ) == (void*)-1 ) {
		perror( "Shared memory attach failure" );
		exit( 1 );
	}

	/* Set shared memory address */
	shm_segment = return_ptr;

	/* Get address for message */
	shm_message = (char*)( shm_segment + 0 );

	/* Get address for flags */
	shm_flags = (int*)( shm_message + ( MAX_CHARS * sizeof( char ) ) );

	/* Get address for PIDs */
	shm_pids = (pid_t*)( shm_flags + ( NUM_READERS * sizeof( int ) ) );
}

/*
 * Gets the number of this reader from the user.
 */
void get_number() {
	/* Get reader number from user */
	char input[16];
	while ( 1 > reader_num || reader_num > NUM_READERS ) {
		printf( "Enter reader number (1 - %d): ", NUM_READERS );
		fgets( input, 16, stdin );
		reader_num = atoi( input );
	}
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
		for ( int i = 0; i < NUM_READERS + 1; i++ ) {
			if ( i != reader_num ) {
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
	/* Detach from shared memory segment for segments */
	if ( shmdt( shm_segment ) == -1 ) {
		perror( "Shared memory segment segment detach failure\n" );
	}
}

void read_messages() {
	/* Run until shut down */
	while ( 1 ) {

		/* Wait for message */
		while ( shm_flags[reader_num - 1] );

		/* Read message */
		printf( "%s\n", shm_message );

		/* Set flag indicating message read */
		shm_flags[reader_num - 1] = 1;
	}
}
