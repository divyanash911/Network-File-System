#ifndef __THREADS_H__
#define __THREADS_H__

#include <pthread.h>
#include <semaphore.h>

// Concurrency handling mechanisms (locks, condition variables and semaphores)
extern pthread_mutex_t accessible_paths_mutex;      // Used to lock memory access to accessible_paths 2D array
extern pthread_mutex_t backup_paths_mutex;          // Used to lock memory access to backup_paths 2D array
extern pthread_mutex_t threads_arr_mutex;           // Used to lock access to the threads array in which we are assigning each request processing thread

extern pthread_cond_t update_paths_txt_cond_var;    // Whenever the accesible paths array is updated, this condition variable is used to signal the store_filepaths thread to store the updated file path into the text file

// Thread functions
void* serve_request(void* args);
void* start_nfs_port(void* args);
void* start_client_port(void* args);
void* check_and_store_filepaths(void* args);
void* check_and_store_backup_paths(void *args);

#endif