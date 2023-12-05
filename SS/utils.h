#ifndef __UTILS_H__
#define __UTILS_H__

typedef struct linked_list_head_struct {
    int number_of_nodes;
    struct linked_list_node_struct* first;
    struct linked_list_node_struct* last;
} linked_list_head_struct;

typedef struct linked_list_head_struct* linked_list_head;

typedef struct linked_list_node_struct {
    char* path;
    struct linked_list_node_struct* next;
} linked_list_node_struct;

typedef struct linked_list_node_struct* linked_list_node;

linked_list_head create_linked_list_head();
linked_list_node create_linked_list_node(char* path);
void free_linked_list(linked_list_head linked_list);
void insert_in_linked_list(linked_list_head linked_list, char* path);

// General utility functions
int register_ss(void);
int is_file(char *string);
int send_update_paths_request(const int request_type, const char* paths_string);
void create_folder(char* path);
void free_tokens(char** tokens);
void find_not_accessible_paths();
void update_path(char* path, char* next_dir);
void send_msg_to_nfs(char* msg, int req_type);
void seek(char* path_to_base_dir, linked_list_head paths);
void send_ack(const int status_code, const int sock_fd, const char* msg);
char* get_folder_name(char* path);
char* remove_extension(char* file_name);
char* create_abs_path(char* relative_path);
char* replace_storage_by_backup(char* path);
char* update_path_rel(char* abs_path, char* curr_path);
char** tokenize(const char* str, const char ch);
char** get_all_files_folders(const char* abs_path);

#endif