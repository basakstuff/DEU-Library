#include <stdio.h>
#include <stdlib.h> 
#include <pthread.h> 
#include <semaphore.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h> 

#define ROOM_LEAVE_TIME_RANDOM 20
#define ROOM_LEAVE_TIME_MIN 20

#define STUDENT_SPAWN_TIME_RANDOM 5
#define STUDENT_SPAWN_TIME_MIN 2

// Room parameters struct
typedef struct {
	// Global student mutex handle
	sem_t* student_mutex;
	// Mutex for accessing line strings specific to this room
	sem_t* buffer_mutex;
	// Line strings of the room
	char line_buffer[5][64];
} t_room_thread_arg;
 
// Room thread worker
void* room_thread(void* arg) { 
	// The argument is of the struct type above
	t_room_thread_arg* room_arg = (t_room_thread_arg*) arg;
	// Room keeps count of students it has
	int student_count = 0;
	// Time at which "New student!" message disappears
	time_t new_alert_expire = 0;
	// Time at which all the students will leave
	time_t busy_room_expire = 0;
	
	// Work indefinitely
	while (1) {
		// If the room is full, start the leave timer
		if (student_count == 4) {
			busy_room_expire = time(NULL) + rand() % ROOM_LEAVE_TIME_RANDOM + ROOM_LEAVE_TIME_MIN;
			
			while (busy_room_expire > time(NULL)) {
				sprintf(room_arg->line_buffer[4], "%d seconds to leave", busy_room_expire - time(NULL));
				usleep(1000000);
			}
			
			// Empty the room
			student_count = 0;
		}
		
		// Check if there are any students in queue
		if (sem_trywait(room_arg->student_mutex) == 0) {
			// Student exists in the queue
			// And this room has accepted it
			student_count++;
			new_alert_expire = time(NULL) + 3;
		}
		
		// Acquire the mutec before accessing the strings
		sem_wait(room_arg->buffer_mutex);
		
		// Write as many "Student" lines as there are students
		for (int j = 0; j < 4; j++) {
			if (j < student_count) {
				strcpy(room_arg->line_buffer[j], "Student");
			}
			
			else {
				strcpy(room_arg->line_buffer[j], "___");
			}
		}
		
		// Alert until it expires
		if (time(NULL) < new_alert_expire) {
			strcpy(room_arg->line_buffer[4], "New student!");
		}
		
		else {
			strcpy(room_arg->line_buffer[4], "");
		}

		sem_post(room_arg->buffer_mutex); 
		
		// Wait some random time so that
		// new students are accepted by random rooms
		usleep(rand() % 30000);
	}
} 

int main() { 
	// Seed the randomizer
	srand(time(NULL));

	// A semaphore specifying the number of students in the queue
	sem_t* student_mutex = malloc(sizeof(sem_t)); 
    sem_init(student_mutex, 0, 0); 
	// Time at which a new student is pushed
	time_t wait_for_push_expire = 0;
	
	// Room threads and their parameter structs
    pthread_t rooms[10]; 
	t_room_thread_arg room_args[10];
	
	// Initialize all the rooms
	for (int i = 0; i < 10; i++) {
		// All the rooms point to the same global student mutex
		room_args[i].student_mutex = student_mutex;
		
		// Rooms have their own mutex for their strings
		room_args[i].buffer_mutex = malloc(sizeof(sem_t)); 
		sem_init(room_args[i].buffer_mutex, 0, 1); 
		
		// Clear the strings initially
		for (int j = 0; j < 5; j++) {
			strcpy(room_args[i].line_buffer[j], "");
		}
		
		// Run the thread
		pthread_create(rooms + i, NULL, room_thread, room_args + i); 
	}
	
	// Work indefinitely
	while (1) {
		// Print the status of the rooms
		// Running over 5 indices, while
		// printing two rooms at the same time
		// side by side
		for (int i = 0; i < 5; i++) {
			printf("Room: %d             |", i + 1);
			printf("Room: %d\n", i + 6);
			
			for (int j = 0; j < 5; j++) {
				// Left room strings 
				sem_wait(room_args[i].buffer_mutex);
				printf("%-20s|", room_args[i].line_buffer[j]);
				sem_post(room_args[i].buffer_mutex);
				
				// Right room strings 
				sem_wait(room_args[i + 5].buffer_mutex);
				printf("%s\n", room_args[i + 5].line_buffer[j]);
				sem_post(room_args[i + 5].buffer_mutex);
			}
			
			printf("--------------------|");
			printf("--------------------\n");
		}
		
		// Prompt for students in the queue if there are any
		int queue_size;
		sem_getvalue(student_mutex, &queue_size);
		
		for (int i = 0; i < queue_size; i++) {
			printf("Student (in queue)\n");
		}
		
		// If it is time to push a new student, increase the student semaphore
		if (wait_for_push_expire < time(NULL)) {
			sem_post(student_mutex);
			// New time to push the next student is randomly selected
			wait_for_push_expire = time(NULL) + rand() % STUDENT_SPAWN_TIME_RANDOM + STUDENT_SPAWN_TIME_MIN;
			printf("New student arrived\n", wait_for_push_expire - time(NULL));
		}
		
		else {
			printf("%d seconds before new student\n", wait_for_push_expire - time(NULL));
		}
		
		// Flush the buffer so that what is written
		// is shown on console immidiately
		fflush(stdout);
		
		// Wait for a while before writing to the console again
		// so that the console doesn't move too quickly
		usleep(200000);
	}
	
    return 0; 
} 