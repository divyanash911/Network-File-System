#include "headers.h"

// Initiating global variables
pthread_mutex_t accessible_paths_mutex;
pthread_mutex_t threads_arr_mutex;
pthread_mutex_t backup_paths_mutex;

int        num_of_not_accessible_paths_stored = 0;
int        num_of_backup_paths_stored   = 0;              // Stores the number of all the backup paths
int        num_of_paths_stored          = 0;              // Initially no paths are stored
int        nfs_registrations_status     = NOT_REGISTERED; // Stores the status whether our server has been registered with NFS or not
int        client_server_socket_fd;                       // TCP Socket file descriptor to receive client requests
int        nfs_server_socket_fd;                          // TCP Socket file descriptor to receive NFS requests
int*       thread_slot_empty_arr;                         // 1 = thread is running, 0 = thread slot is free and can be used to create a new thread
char*      PWD                          = NULL;           // Stores the path to PWD
char**     not_accessible_paths         = NULL;           // Stores the RELATIVE PATH of all the files that are not accessible when the ss is initialized
char**     accessible_paths             = NULL;           // Stores the RELATIVE PATH (relative to the directory in which the storage server c file resides) of all the files that are accessible by clients on this storage server
char**     backup_paths                 = NULL;           // Stores the relative path of backup files
struct     sockaddr_in ss_address_nfs;                    // IPv4 address struct for ss and nfs TCP communication (requests)
struct     sockaddr_in ss_address_client;                 // IPv4 address struct for ss and client TCP communication (requests)
socklen_t  addr_size;                                     // IPv4 address struct for ss and nfs USP communication (register)
pthread_t* requests_serving_threads_arr;                  // Holds the threads when a request is being served in some thread

int main(int argc, char *argv[])
{
    // Initializing mutexes, condition variables and semaphores
    /*========== MUTEX ==========*/
    if (pthread_mutex_init(&accessible_paths_mutex, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_mutex_init : Unable to initialise accessible_paths_mutex : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&threads_arr_mutex, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_mutex_init : Unable to initialise threads_arr_mutex : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_init(&backup_paths_mutex, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_mutex_init : Unable to initialise backup_paths_mutex : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    PWD = (char*) calloc(MAX_PATH_LEN, sizeof(char));
    if (PWD == NULL)
    {
        fprintf(stderr, RED("calloc : cannot allocate memory to pwd char array : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (getcwd(PWD, MAX_PATH_LEN) == NULL)
    {
        fprintf(stderr, RED("pwd : error in getting pwd : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    accessible_paths = (char**) malloc(MAX_FILES * sizeof(char*));
    if (accessible_paths == NULL)
    {
        fprintf(stderr, RED("malloc : cannot allocate memory to accessible paths : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    not_accessible_paths = (char**) malloc(MAX_FILES * sizeof(char*));
    if (not_accessible_paths == NULL)
    {
        fprintf(stderr, RED("malloc : cannot allocate memory to not accessible paths : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    backup_paths = (char**) malloc(MAX_FILES * sizeof(char*));
    if (backup_paths == NULL)
    {
        fprintf(stderr, RED("malloc : cannot allocate memory to backup paths : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_FILES; i++)
    {
        accessible_paths[i] = (char*) calloc(MAX_PATH_LEN, sizeof(char));
        if (accessible_paths[i] == NULL)
        {
            fprintf(stderr, RED("calloc : cannot allocate memory to accessible_paths[i] : %s\n"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        backup_paths[i] = (char*) calloc(MAX_PATH_LEN, sizeof(char));
        if (backup_paths[i] == NULL)
        {
            fprintf(stderr, RED("calloc : cannot allocate memory to backup_paths[i] : %s\n"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        not_accessible_paths[i] = (char*) calloc(MAX_PATH_LEN, sizeof(char));
        if (not_accessible_paths[i] == NULL)
        {
            fprintf(stderr, RED("calloc : cannot allocate memory to not_accessible_paths[i] : %s\n"), strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    
    char new_paths[MAX_DATA_LENGTH - 1000] = {0};
    for (int i = 1; i < argc; i++)
    {
        strcpy(accessible_paths[num_of_paths_stored++], argv[i]);
        strcat(new_paths, argv[i]);
        strcat(new_paths, "|");
    }

    find_not_accessible_paths();

    // Allocating memory
    requests_serving_threads_arr = (pthread_t*) malloc(MAX_PENDING * sizeof(pthread_t));
    if (requests_serving_threads_arr == NULL)
    {
        fprintf(stderr, RED("malloc : cannot allocate memory to requests thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    thread_slot_empty_arr = (int*) calloc(MAX_PENDING, sizeof(int));    // 0 indicates slot is empty and 1 indicates slot is busy
    if (thread_slot_empty_arr == NULL)
    {
        fprintf(stderr, RED("malloc : cannot allocate memory to thread_slot_empty_arr : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Register my SS with NFS
    if (register_ss() != 0)
    {
        fprintf(stderr, RED("Could not register SS.\n"));
        exit(EXIT_FAILURE);
    }
    // First start the NFS and Client TCP servers to listen to their requests
    pthread_t nfs_thread, client_thread;
    if (pthread_create(&nfs_thread, NULL, &start_nfs_port, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_create : Unable to create nfs_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&client_thread, NULL, &start_client_port, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_create : Unable to create client_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }


    if (strlen(new_paths) > 0)
    {
        // Removing the last | from the concatenation of paths
        new_paths[strlen(new_paths) - 1] = '\0';
        if (send_update_paths_request(ADD_PATHS, new_paths) != 0)
        {
            fprintf(stderr, RED("Could not send add paths request.\n"));
        }
    }

    // Creating the thread that would keep updating the paths.txt file with the current state of the accessible paths array regularly after some time interval
    pthread_t check_and_store_filepaths_thread;
    pthread_t check_and_store_backup_paths_thread;
    if (pthread_create(&check_and_store_filepaths_thread, NULL, &check_and_store_filepaths, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_create : Unable to create check_and_store_filepaths_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&check_and_store_backup_paths_thread, NULL, &check_and_store_backup_paths, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_create : Unable to create check_and_store_backup_paths_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Waiting for threads to complete
    if (pthread_join(check_and_store_backup_paths_thread, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_join : Could not join thread check_and_store_backup_paths_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_join(check_and_store_filepaths_thread, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_join : Could not join thread check_and_store_filepaths_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_join(nfs_thread, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_join : Could not join thread nfs_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_join(client_thread, NULL) != 0)
    {
        fprintf(stderr, RED("pthread_join : Could not join thread client_thread : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Destroying mutexes, condition variables and semaphores
    /*========== MUTEX ==========*/
    if (pthread_mutex_destroy(&accessible_paths_mutex) != 0)
    {
        fprintf(stderr, RED("pthread_mutex_destroy : Unable to destroy accessible_paths_mutex : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_destroy(&threads_arr_mutex) != 0)
    {
        fprintf(stderr, RED("pthread_mutex_destroy : Unable to destroy threads_arr_mutex : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_destroy(&backup_paths_mutex) != 0)
    {
        fprintf(stderr, RED("pthread_mutex_destroy : Unable to destroy backup_paths_mutex : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Freeing Memory
    free(requests_serving_threads_arr);
    free(thread_slot_empty_arr);

    return 0;
}

