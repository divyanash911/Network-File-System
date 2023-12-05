#ifndef __HEADERS_H__
#define __HEADERS_H__

#define LINUX

// =========================== Header files ===========================
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// ==================== User defined header files  ====================


// ========================== Useful Macros ==========================
#define MAX_DATA_LENGTH     100000      // Maximum number of characters data being sent can have (query/file data)
#define MAX_NO_OF_REQ       10          // At max it can handle 10 pending requests, if the request buffer is full then all the other incoming requests will be rejected
#define MAX_FILES           10          // Maximum number of files that can be stored in the storage server
#define MAX_PATH_LEN        1024        // Maximum length the relative path of a file can have
#define NFS_SERVER_PORT_NO  2000        // Port on which NFS server listens
#define MY_NFS_PORT_NO      3000        // Port number used to communicate with NFS server
#define MY_CLIENT_PORT_NO   4000        // Port number used to communicate with client
#define MY_IP               "0.0.0.0"   // Ip address of this storage server
#define NS_PORT 2000                    // NS PORT
#define NS_IP "0.0.0.0"               // NS IP

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


// =========================== Request Types ==========================
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
#define COPY_TO              15
#define COPY_FROM            16
#define COPY_FILE            16
// #define FILE_NOT_FOUND       18
#define RETRIEVE_INFO        19
#define INFO                 20
#define LIST                 20
#define PING                 21
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
#define WRITE_APPEND_COMP    40
#define TIMEOUT              39
#define COPY_FOLDER 36
// ============================= Statuses =============================
#define NOT_REGISTERED 0
#define REGISTERED     1


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
#define INVALID_FILETYPE     402
#define SERVER_NOT_FOUND     403
// Everything ok
#define OK                   200


// ============================ Structures ============================
// All the network communication happens in this structure form
typedef struct st_request 
{
    int  request_type;              // Request type would determine whether it is an acknowledgement, query, response, data etc.
    char data[MAX_DATA_LENGTH];     // All the communication happens in the form of strings
} st_request;

typedef struct st_request* request;

// Used to pass data to request serving threads
// typedef struct st_thread_data
// {
//     int thread_idx;
//     int client_sock_fd;
// } st_thread_data;

//typedef struct st_thread_data* thread_data;

// ========================= Global variables =========================




void create_operation(char *path, char *name,int macro);
int connect_with_ss(char *ipaddress, char *port);
void communicate_with_ss(char *ipaddress, char *port, char *path);
int connect_with_ns();
void reading_operation(char *path);
void communicate_with_ss_write(char *ipaddress, char *port, char *path,int f);
void writing_append_operation(char *path,int f);
void communicate_with_ss_info(char *ipaddress, char *port, char *path);
void info(char *path);
void delete_operation(char *path,int macro);
void copy_operation(int req_type,char *path1, char *path2);
void list();
void man();

#endif