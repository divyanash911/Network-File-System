#include "headers.h"

cache_array cache = NULL;
int curr_cache_write_index;
ss ss_list[100];         // List of all storage servers
int server_count = 0;    // Number of storage servers
packet send_buffer[100]; // Buffer to store packets to be sent
int send_count = 0;      // Number of packets in buffer

// Helper function to split string into tokens (n tokens)
char **processstring(char data[], int n)
{
    char **tokens = (char **)malloc(sizeof(char *) * n);
    for (int i = 0; i < n; i++)
    {
        tokens[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    }

    int i = 0;
    char *token = strtok(data, "|");
    int j = 0;
    while (token != NULL && j < n)
    {
        j++;
        strcpy(tokens[i], token);
        token = strtok(NULL, "|");
        i++;
    }

    return tokens;
}

int connect_to_port(char *port)
{
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(port));
    server_address.sin_addr.s_addr = INADDR_ANY;

    while (1)
    {
        int x = connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
        if (x == -1)
        {
            printf(RED("Connection to %s failed\n"), port);
            return 0;
        }
        else if (x == 0)
            break;
    }
    return client_socket;
}

// Code to initialise nfs
void init_nfs()
{
    // nothing as of now but any global pointers declared will be malloced here
    init_cache();
    sem_init(&lock, 0, 1);
    for (int i = 0; i < 100; i++)
    {
        client_socket_arr[i] = -1;
    }
    initializer_header_node();
    return;
}

// Client requests handled here
void client_handler(char data[])
{
    // yet to work on based on client request format
    char **tokens = (char **)malloc(sizeof(char *) * 3);
    tokens = processstring(data, 3);
    char *path = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    strcpy(path, tokens[0]);
}

void replicate_backups(ss server)
{
    int id_array[MAX_CONNECTIONS];
    int ind = 0;
    for (int i = 0; i < server_count; i++)
    {

        if (ss_list[i]->is_backedup == 1 && (strcmp(ss_list[i]->backup_port[0], server->port) == 0 || strcmp(ss_list[i]->backup_port[1], server->port) == 0))
        {
            id_array[ind] = i;
            ind++;
        }
    }

    char **new_paths = (char **)malloc(sizeof(char *) * MAX_CONNECTIONS);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        new_paths[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    }
    int total_new_paths = 0;
    for (int i = 0; i < ind; i++)
    {
        linked_list_head head = return_paths(ss_list[id_array[i]]->root);
        linked_list_node trav = head->first;
        while (trav != NULL)
        {
            strcpy(new_paths[total_new_paths], trav->path);
            total_new_paths++;
            trav = trav->next;
        }
    }

    for (int i = 0; i < total_new_paths; i++)
    {
        ss found_server = NULL;
        for (int j = 0; j < server_count; j++)
        {
            if (search_path(ss_list[j]->root, new_paths[i]) >= 0)
            {
                found_server = ss_list[j];
                break;
            }
        }

        request r = (request)malloc(sizeof(st_request));

        if (strstr(new_paths[i], ".txt") != NULL)
        {
            if(found_server->status ==1){

            
            printf("Copying file %s\n\n", new_paths[i]);
            r->request_type = COPY_FILE;
            strcpy(r->data, new_paths[i]);
            printf("Connecting to %s\n", found_server->port);
            int sock = connect_to_port(found_server->port);

            if (send(sock, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending\n"));
            }

            if (recv(sock, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in receiving\n"));
            }

            close(sock);
            r->request_type = BACKUP_PASTE;

            sock = connect_to_port(server->port);
            if (sock <= 0)
            {
                printf(RED("Error in connecting to %s\n"), server->port);
            }

            if (send(sock, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending\n"));
            }
            close(sock);
        }
        }
        else
        {
            if(server->status ==1){
            printf("Copying folder %s\n\n", new_paths[i]);
            r->request_type = BACKUP_CREATE_FOLDER;
            strcpy(r->data, new_paths[i]);

            int sock = connect_to_port(server->port);
            if (sock <= 0)
            {
                printf(RED("Error in connecting to %s\n"), server->port);
            }

            if (send(sock, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending\n"));
            }
            close(sock);}
        }
        
    }

    // delete new files
    char **del_paths = (char **)malloc(sizeof(char *) * MAX_CONNECTIONS);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        del_paths[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    }

    linked_list_head ll = return_paths(server->backup_root);

    linked_list_node trav = ll->first;
    int del_ind = 0;
    while (trav != NULL)
    {
        strcpy(del_paths[del_ind], trav->path);
        del_ind++;
        trav = trav->next;
    }

    char **to_rem = (char **)malloc(sizeof(char *) * MAX_CONNECTIONS);
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
        to_rem[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    }
    int to_rem_ind = 0;
    for (int i = 0; i < del_ind; i++)
    {
        int flag = 1;

        for (int j = 0; j < total_new_paths; j++)
        {
            if (strcmp(del_paths[i], new_paths[j]) == 0)
            {
                flag = 0;
                break;
            }
        }
        if (flag == 1)
        {
            strcpy(to_rem[to_rem_ind], del_paths[i]);
            to_rem_ind++;
        }
    }
    request r = (request)malloc(sizeof(st_request));
    for (int i = 0; i < to_rem_ind; i++)
    {

        if (strstr(to_rem[i], ".txt") != NULL)
        {
            memset(r->data, 0, sizeof(MAX_DATA_LENGTH));
            r->request_type = BACKUP_DELETE_FILE;
            strcpy(r->data, to_rem[i]);

            int sock = connect_to_port(server->port);

            if (sock < 0)
            {
                printf(RED("Error in connecting to %s\n"), server->port);
            }

            if (send(sock, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending\n"));
            }
            close(sock);
        }
        else
        {
            memset(r->data, 0, sizeof(MAX_DATA_LENGTH));
            r->request_type = BACKUP_DELETE_FOLDER;
            strcpy(r->data, to_rem[i]);

            int sock = connect_to_port(server->port);

            if (sock < 0)
            {
                printf(RED("Error in connecting to %s\n"), server->port);
            }

            if (send(sock, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending\n"));
            }

            close(sock);
        }
    }

    return;
}

void init_storage(char data[])
{
    // tokenise the string and create a new server object with extracted attributes
    char **tokens = processstring(data, 4);
    ss new_ss = (ss)malloc(sizeof(ss_info));
    strcpy(new_ss->ip, tokens[1]);
    strcpy(new_ss->port, tokens[3]);
    strcpy(new_ss->client_port, tokens[2]);
    new_ss->path_count = 0;

    new_ss->synced = 0;
    pthread_mutex_init(&new_ss->lock, NULL);

    // pthread_mutex_lock(&server_lock);

    int check_flag = 0;
    int id = -1;
    for (int i = 0; i < server_count; i++)
    {
        if (strcmp(ss_list[i]->port, new_ss->port) == 0)
        {
            check_flag = 1;
            id = i;
            break;
        }
    }
    if (check_flag == 1)
    {
        printf(GREEN("%s is back online!\n\n\n"), new_ss->port);
        new_ss->status = 1;
        ss_list[id]->status = 1;

        if (ss_list[id]->total_backups > 0)
        {
            replicate_backups(ss_list[id]);
        }

        if (ss_list[id]->is_backedup == 1)
        {
            sync_backup((void *)ss_list[id]);
        }

        ss_list[id]->path_count = 0;
    }
    else
    {
        printf(GREEN("Connected to server %s\n\n\n"), new_ss->port);

        new_ss->backup_path_count = 0;
        new_ss->root = create_trie_node();
        new_ss->backup_root = create_trie_node();
        new_ss->is_backedup = 0;
        new_ss->has_backup = 0;
        new_ss->status = 1;
        new_ss->total_backups = 0;
        new_ss->ssid = atoi(tokens[0]);
        ss_list[server_count] = new_ss;
        id = server_count;
        server_count++;
    }
    // pthread_mutex_unlock(&server_lock);

    sleep(2);
    pthread_t server_thread;
    pthread_create(&server_thread, NULL, &server_handler, (void *)ss_list[id]);

    return;
}

int main()
{
    // Handling Ctrl + z (SIGTSTP) signal to print the log information on the screen without interrupting the working of NS
    struct sigaction sa;
    sa.sa_handler = &handleCtrlZ;  // Ctrl + Z (Windows/Linux/Mac)
    sa.sa_flags = SA_RESTART;      // Automatically restart the system call
    sigaction(SIGTSTP, &sa, NULL); // Ctrl + Z sends SIGTSTP signal (Signal Stop) - Prints the log onto the screen

    init_nfs(); // initialises ns server

    // declaring thread variables
    pthread_t receive_thread;
    pthread_t backup_thread_idx;

    // TCP socket to check for new requests

    // constructing threads for listening to TCP sockets
    pthread_create(&receive_thread, NULL, &receive_handler, NULL);
    pthread_create(&backup_thread_idx, NULL, &backup_thread, NULL);

    // joining threads
    pthread_join(receive_thread, NULL);
    pthread_join(backup_thread_idx, NULL);

    while (1) {}

    return 0;
}
