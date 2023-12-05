//Code for initialising and working with NFS for the project
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <semaphore.h>

//Relevant Macros
#define NS_PORT             2000
#define NS_IP               "0.0.0.0"
#define MAX_DATA_LENGTH     100000
#define MAX_CONNECTIONS     100
#define MAX_PATH_LEN        1024  
#define MAX_PATHS_LOCKED    1000
#define MAX_FILES           50

//Request types
#define ACK                  1
#define REQ                  2
#define RES                  3
#define WRITE_REQ            4
#define READ_REQ             5
#define DELETE_REQ           6
#define CREATE_REQ           7
#define APPEND_REQ           8
#define REGISTRATION_REQUEST 9
#define REGISTRATION_ACK     10
#define STOP_REQ             11
#define READ_REQ_DATA        12
#define ADD_PATHS            13
#define DELETE_PATHS         14
#define COPY_FILE            16
#define PASTE                15
#define COPY_REQUEST         17
#define RETRIEVE_INFO        19
#define LIST                 20
#define PING                 22
#define CREATE_FILE          23
#define CREATE_FOLDER        24
#define DELETE_FILE          25
#define DELETE_FOLDER        26
#define BACKUP_PASTE         27
#define BACKUP_READ_REQ      28
#define BACKUP_WRITE_REQ     29
#define BACKUP_APPEND_REQ    30
#define BACKUP_DELETE_FILE   31
#define BACKUP_DELETE_FOLDER 32
#define BACKUP_CREATE_FILE   33
#define BACKUP_CREATE_FOLDER 34

#define CONSISTENT_WRITE     35
#define COPY_FOLDER 36
#define N_FILE_REQ 37
#define WRITE_APPEND_COMP 40
#define TIMEOUT 39
#define FOLDER_DATA_TO_BE_COPIED 38

// Macros for book keeping
#define SS          -1
#define CLIENT      -2
#define BUFFER_SIZE 1024

// Macros for caching 
#define CACHE_SIZE 20

// ============================= ERROR Codes =============================
// All operation failed error codes start from 600
#define REQ_UNSERVICED       600
#define DELETE_FAILED        601
#define PASTE_FAILED         602
#define COPY_FAILED          603
#define APPEND_FAILED        604
#define CREATE_FAILED        605
#define INFO_RETRIEVAL_FAILED 606
#define READ_FAILED          607
#define WRITE_FAILED         608
// All not allowed error codes will start from 500
#define INVALID_DELETION     501
// All not found error codes will start from 400
#define FILE_NOT_FOUND       404
#define SERVER_NOT_FOUND     403
// Everything ok
#define OK                   200
#define INVALID_FILETYPE     402

// =========================== Color Codes ============================
#define RED_COLOR    "\033[0;31m"
#define GREEN_COLOR  "\033[0;32m"
#define BLUE_COLOR   "\033[0;34m"
#define YELLOW_COLOR "\033[0;33m"
#define CYAN_COLOR   "\033[0;36m"
#define ORANGE_COLOR "\e[38;2;255;85;0m"
#define RESET_COLOR  "\033[0m"

#define RED(str)    RED_COLOR    str RESET_COLOR
#define GREEN(str)  GREEN_COLOR  str RESET_COLOR
#define BLUE(str)   BLUE_COLOR   str RESET_COLOR
#define YELLOW(str) YELLOW_COLOR str RESET_COLOR
#define CYAN(str)   CYAN_COLOR   str RESET_COLOR
#define ORANGE(str) ORANGE_COLOR str RESET_COLOR

//Concurrency locks and conditional variables
extern pthread_mutex_t server_lock;
extern pthread_mutex_t send_buffer_lock;
extern pthread_cond_t send_signal;

extern int server_socket_tcp, client_socket_tcp;
extern struct sockaddr_in server_addr_tcp, client_addr_tcp;
extern socklen_t client_addr_len_tcp;

//Request packet structure
typedef struct st_request
{
    int request_type;
    char data[MAX_DATA_LENGTH];
} st_request;

//Typedef of request and server pointer objects
typedef struct st_request* request;
typedef struct ss_info* ss;

//Storage server info
typedef struct ss_info
{
    int status;
    char ip[20];
    char port[10];
    char client_port[10];
    char paths[1000][100];

    struct trie_node *root;
    struct trie_node* backup_root;
    int path_count;
    int server_socket, client_socket,ping_server_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len;
    pthread_mutex_t lock;
    char backup_paths[1000][100];
    int has_backup;
    int is_backedup;
    int backup_path_count;
    char backup_port[2][10];
    int total_backups;
    int added;
    int synced;
    int ssid;
} ss_info;

//Send packet used for convenience of sending request packets to client/SS
typedef struct send_packet{
    request r;      //main request packet
    int send_to;    //Whether a client or server
    char ip[20];    //IP address where to send the request body
    int status;     
    char port[10];
} send_packet;

 struct proc{
    char data[MAX_DATA_LENGTH];
    int request_type;
    int client_id;
};

typedef struct proc* proc;

struct trie_node
{
    char *key;
    int end;
    int ssid;
    struct trie_node *children[256];    // Total 256 characters possible that can come in any path name
};

typedef send_packet* packet;

typedef struct server_status{

    char port[10];
    int status;

} server_status;

struct path_locked {
    char path[1000];
    struct path_locked *next;
};
typedef struct path_locked paths_lock;

typedef struct linked_list_head_struct* linked_list_head;
typedef struct linked_list_node_struct* linked_list_node;

typedef struct linked_list_head_struct {
    int number_of_nodes;
    struct linked_list_node_struct* first;
    struct linked_list_node_struct* last;
} linked_list_head_struct;

typedef struct linked_list_node_struct {
    char* path;
    struct linked_list_node_struct* next;
} linked_list_node_struct;

typedef struct st_cache {
    int req_type;
    char req_data[MAX_DATA_LENGTH];
    int ss_id;
    char ss_ip[20];
    int ss_port;
} st_cache;

typedef struct st_copy_folder
{
    int request_type;
    int num_paths;
    char paths[MAX_FILES][MAX_PATH_LEN];
} st_copy_folder;

typedef st_cache* cache_array;

//Global variables
extern cache_array cache;
extern int curr_cache_write_index;
extern ss ss_list[100];
extern int server_count;
extern packet send_buffer[100];
extern int send_count;
extern int server_socket_tcp;
extern struct sockaddr_in server_addr_tcp, client_addr_tcp;
extern socklen_t addr_size_tcp;
extern pthread_cond_t send_signal;
extern server_status connections[100];
extern int connection_count;
extern pthread_mutex_t status_lock;
extern pthread_mutex_t path_locked;
extern paths_lock *global_paths_locked;      //Header Node
extern int client_socket_arr[100];
extern sem_t lock;

//Defined functions
char** processstring(char data[],int n);
void init_nfs();
void client_handler(char data[]);
void init_storage(char data[]);
void* process(void* arg);
void* send_handler();
void* receive_handler();
void* server_handler(void* p);
void* backup_thread();
void* sync_backup(void* p);

// Linked list functions
linked_list_head create_linked_list_head();
void free_linked_list(linked_list_head linked_list);
linked_list_node create_linked_list_node(char* path);
void insert_in_linked_list(linked_list_head linked_list, char* path);

// Tries functions
struct trie_node *create_trie_node();
void print_paths(struct trie_node *root);
int insert_path(struct trie_node *root, char *key,int ssid);
int search_path(struct trie_node *root, char *key);
int delete_path(struct trie_node *root, char *key);
linked_list_head return_paths(struct trie_node *root);
void add_paths(linked_list_head ll, struct trie_node *root);

// Book Keeping functions
void handleCtrlZ(int signum);
int insert_log(const int type, const int ss_id, const int ss_or_client_port, const int request_type, const char* request_data, const int status_code);

// Caching functions
void init_cache();
void print_cache();
void delete_cache_index(const int idx);
st_cache* search_in_cache(int req_type, char* req_data);
void insert_in_cache(int req_type, char* req_data, int ss_id, char* ss_ip, int ss_port);

// Path locking functions
void initializer_header_node();
int path_locked_or_not(char *path);
void insert_path_lock(const char *new_path);
void delete_path_lock(const char *path_to_delete);

// Processing functions
void* basic_ops(request req,int client_id);
void* handle_create(request req,int client_id);
void* handle_delete(request req,int client_id);
void* copy_handler(request req,int client_id);
int connect_to_port(char* port);
void replicate_backups(ss server);