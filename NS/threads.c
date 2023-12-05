#include "headers.h"

pthread_mutex_t server_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t send_buffer_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t send_signal = PTHREAD_COND_INITIALIZER;
pthread_mutex_t status_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t path_locked = PTHREAD_MUTEX_INITIALIZER;

sem_t lock;

int server_socket_tcp, client_socket_tcp;
struct sockaddr_in server_addr_tcp, client_addr_tcp;
socklen_t client_addr_len_tcp = sizeof(client_addr_tcp);

server_status connections[100];
int connection_count = 0;
int ack = 0;

int client_socket_arr[100];

void *receive_handler()
{
    printf(YELLOW("-------------------------Naming Server started-------------------------------\n\n\n"));

    server_socket_tcp = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_tcp == -1)
    {
        perror(RED("Socket creation failed"));
    }

    server_addr_tcp.sin_family = AF_INET;
    server_addr_tcp.sin_port = htons(NS_PORT);
    server_addr_tcp.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_tcp, (struct sockaddr *)&server_addr_tcp, sizeof(server_addr_tcp)) == -1)
    {
        perror(RED("Binding to port failed"));
    }

    if (listen(server_socket_tcp, MAX_CONNECTIONS) == -1)
    {
        perror("Listening on port failed");
    }

    while (1)
    {
        int id = -1;
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            if (client_socket_arr[i] == -1)
            {
                id = i;
                break;
            }
        }
        client_socket_arr[id] = accept(server_socket_tcp, (struct sockaddr *)&client_addr_len_tcp, &client_addr_len_tcp);
        if (client_socket_arr[id] == -1)
        {
            perror("Accepting connection failed");
        }
        request req = (request)malloc(sizeof(st_request));
        int x = recv(client_socket_arr[id], req, sizeof(st_request), 0);
        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
        if (logging == 0)
        {
            printf(RED("Logging not added\n"));
        }
        if (req->request_type < 41 && req->request_type > 3)
        {
            printf("New request of type %d from client %d\n\n\n", req->request_type, id);
            pthread_t new_proc;
            proc n = (proc)malloc(sizeof(struct proc));
            n->request_type = req->request_type;
            strcpy(n->data, req->data);
            n->client_id = id;

            pthread_create(&new_proc, NULL, &process, (void *)n);
        }

        free(req);
    }

    close(server_socket_tcp);

    return NULL;
}

void *server_handler(void *p)
{
    ss pack = (ss)p;

    request r = (request)malloc(sizeof(st_request));

    while (1)
    {
        if (pack->status == 1)
        {
            int sock = connect_to_port(pack->port);

            if (sock == -1)
            {
                printf(RED("Server %s disconnected!\n\n\n"), pack->port);
                pack->status = 0;
                return NULL;
            }

            pack->status = 1;
            r->request_type = PING;
            strcpy(r->data, "");

            int x = send(sock, r, sizeof(st_request), MSG_NOSIGNAL);
            if (x < 0)
            {
                pack->status = 0;
                pack->synced = 0;
                printf(RED("Server %s disconnected with send error!\n\n\n"), pack->port);
                pthread_mutex_lock(&server_lock);
                for (int i = 0; i < server_count; i++)
                {
                    if (strcmp(ss_list[i]->port, pack->port) == 0)
                    {
                        ss_list[i]->status = 0;
                    }
                }
                pthread_mutex_unlock(&server_lock);
                return NULL;
            }
            recv(sock, r, sizeof(st_request), 0);

            if (r->request_type != ACK)
            {

                pack->synced = 0;
                printf(RED("Server %s disconnected!\n\n\n"), pack->port);
                pthread_mutex_lock(&server_lock);
                for (int i = 0; i < server_count; i++)
                {
                    if (strcmp(ss_list[i]->port, pack->port) == 0)
                    {
                        ss_list[i]->status = 0;
                    }
                }
                pthread_mutex_unlock(&server_lock);
                return NULL;
            }
            close(sock);
            sleep(5);
        }
    }

    return NULL;
}

void *sync_backup(void *arg)
{
    while (1)
    {
        ss pack = (ss)arg;
        pthread_mutex_lock(&pack->lock);
        if (pack->is_backedup == 1 && pack->synced == 0 && pack->added == 1 && pack->status == 1)
        {
            char **paths = (char **)malloc(sizeof(char *) * 100);
            char **add = (char **)malloc(sizeof(char *) * 100);
            char **old_paths = (char **)malloc(sizeof(char *) * 100);
            for (int i = 0; i < 100; i++)
            {
                paths[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
                add[i] = (char *)malloc(sizeof(char *) * MAX_DATA_LENGTH);
                old_paths[i] = (char *)malloc(sizeof(char *) * MAX_DATA_LENGTH);
            }

            ss id, id2;
            int idx_1 = -1, idx_2 = -1;

            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, pack->backup_port[0]) == 0)
                {
                    id = ss_list[i];
                    idx_1 = i;
                }
                else if (strcmp(ss_list[i]->port, pack->backup_port[1]) == 0)
                {
                    id2 = ss_list[i];
                    idx_2 = i;
                }

                if (idx_1 != -1 && idx_2 != -1)
                    break;
            }

            if(ss_list[idx_1]->status ==0 || ss_list[idx_2]->status ==0)return NULL;

            int cnt = 0;
            linked_list_head ll = return_paths(pack->root);
            linked_list_node trav = ll->first;
            while (trav != NULL)
            {
                strcpy(paths[cnt], trav->path);
                cnt++;
                trav = trav->next;
            }

            int ind = 0;

            for (int i = 0; i < cnt; i++)
            {

                if (search_path(id->backup_root, paths[i]))
                {
                    continue;
                }
                else
                {
                    strcpy(add[ind++], paths[i]);
                }
            }
            pthread_mutex_unlock(&pack->lock);
            if (ind != 0)
                printf(BLUE("Syncing %s\n\n\n"), pack->port);
            else
                printf("%s all synced up\n\n\n", pack->port);

            for (int i = 0; i < cnt; i++)
            {
                int id1 = -1, id2 = -1;
                for (int j = 0; j < server_count; j++)
                {
                    if (strcmp(ss_list[j]->port, pack->backup_port[0]) == 0)
                    {
                        id1 = j;
                    }
                    if (strcmp(ss_list[j]->port, pack->backup_port[1]) == 0)
                    {
                        id2 = j;
                    }
                    if (id1 != -1 && id2 != -1)
                        break;
                }

                request r = (request)malloc(sizeof(st_request));

                if (strstr(paths[i], ".txt") != NULL)
                {
                    r->request_type = COPY_FILE;
                    strcpy(r->data, paths[i]);

                    int sock = connect_to_port(pack->port);
                    send(sock, r, sizeof(st_request), 0);
                    recv(sock, r, sizeof(st_request), 0);
                    close(sock);

                    r->request_type = BACKUP_PASTE;

                    if (ss_list[id1]->status == 1)
                    {
                        int sock = connect_to_port(pack->backup_port[0]);

                        send(sock, r, sizeof(st_request), 0);
                        close(sock);
                    }

                    if (ss_list[id2]->status == 1)
                    {
                        int sock = connect_to_port(pack->backup_port[1]);
                        send(sock, r, sizeof(st_request), 0);
                        close(sock);
                    }
                }
                else
                {
                    r->request_type = BACKUP_CREATE_FOLDER;
                    strcpy(r->data, paths[i]);

                    if (ss_list[id1]->status == 1)
                    {
                        int sock = connect_to_port(pack->backup_port[0]);
                        send(sock, r, sizeof(st_request), 0);
                        close(sock);
                    }

                    if (ss_list[id2]->status == 1)
                    {
                        int sock = connect_to_port(pack->backup_port[1]);
                        send(sock, r, sizeof(st_request), 0);
                        close(sock);
                    }
                }
            }

            pack->synced = 1;

            char **paths1 = (char **)malloc(sizeof(char *) * 100);
            for (int i = 0; i < 100; i++)
            {
                paths1[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
            }

            linked_list_head ll1 = return_paths(id->backup_root);
            int cnt1 = 0;
            trav = ll1->first;
            while (trav != NULL)
            {
                strcpy(paths1[cnt1], trav->path);
                cnt1++;
                trav = trav->next;
            }

            char **paths2 = (char **)malloc(sizeof(char *) * 100);
            for (int i = 0; i < 100; i++)
            {
                paths2[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
            }
            linked_list_head ll2 = return_paths(id2->backup_root);
            int cnt2 = 0;
            trav = ll2->first;
            while (trav != NULL)
            {
                strcpy(paths2[cnt2], trav->path);
                cnt2++;
                trav = trav->next;
            }

            char **to_rem = (char **)malloc(sizeof(char *) * 100);
            for (int i = 0; i < 100; i++)
            {
                to_rem[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
            }
            int ind1 = 0;
            for (int i = 0; i < cnt1; i++)
            {
                int flag = 1;
                for (int j = 0; j < cnt; j++)
                {
                    if (strcmp(paths1[i], paths[j]) == 0)
                    {
                        flag = 0;
                        break;
                    }
                }
                if (flag == 1 && search_path(id->backup_root, paths1[i]) == pack->ssid)
                {
                    strcpy(to_rem[ind1++], paths1[i]);
                }
            }

            for (int i = 0; i < ind1; i++)
            {
                request r = (request)malloc(sizeof(st_request));

                if (strstr(to_rem[i], ".txt") != NULL)
                {
                    r->request_type = BACKUP_DELETE_FILE;
                }
                else
                {
                    r->request_type = BACKUP_DELETE_FOLDER;
                }

                strcpy(r->data, to_rem[i]);

                if (id->status == 1)
                {
                    int sock = connect_to_port(pack->backup_port[0]);
                    send(sock, r, sizeof(st_request), 0);
                    close(sock);
                }
            }

            char **to_rem1 = (char **)malloc(sizeof(char *) * 100);
            for (int i = 0; i < 100; i++)
            {
                to_rem1[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
            }

            int ind2 = 0;
            for (int i = 0; i < cnt2; i++)
            {
                int flag = 1;
                for (int j = 0; j < cnt; j++)
                {
                    if (strcmp(paths2[i], paths[j]) == 0)
                    {
                        flag = 0;
                        break;
                    }
                }
                if (flag == 1 && search_path(id2->backup_root, paths2[i]) == pack->ssid)
                {
                    strcpy(to_rem1[ind2++], paths2[i]);
                }
            }

            for (int i = 0; i < ind2; i++)
            {
                request r = (request)malloc(sizeof(st_request));

                if (strstr(to_rem1[i], ".txt") != NULL)
                {
                    r->request_type = BACKUP_DELETE_FILE;
                }
                else
                {
                    r->request_type = BACKUP_DELETE_FOLDER;
                }
                strcpy(r->data, to_rem1[i]);
                if (id2->status == 1)
                {
                    int sock = connect_to_port(pack->backup_port[1]);
                    send(sock, r, sizeof(st_request), 0);
                    close(sock);
                }
            }
        }
        else
        {
            pthread_mutex_unlock(&pack->lock);
        }
    }
}

void *backup_thread()
{
    while (1)
    {
        for (int i = 0; i < server_count; i++)
        {
            if (ss_list[i]->is_backedup == 0 && ss_list[i]->status == 1 && ss_list[i]->added == 1)
            {
                int flag = 0;
                int id1 = -1, id2 = -1;
                int min = 1000000, second_min = 1000000;

                pthread_mutex_lock(&server_lock);
                for (int j = 0; j < server_count; j++)
                {
                    if (i != j && ss_list[j]->status == 1)
                    {
                        if (flag == 0)
                        {
                            id1 = j;
                            flag = 1;
                        }
                        else if (flag == 1)
                        {
                            id2 = j;
                            flag = 0;
                        }
                    }
                }
                pthread_mutex_unlock(&server_lock);

                // copy paths in these two servers
                if (id1 != -1 && id2 != -1 && ss_list[id1]->status == 1 && ss_list[id2]->status == 1 && id1 != i && id2 != i)
                {
                    printf(ORANGE("Backing up server %s in servers %s %s\n\n\n"), ss_list[i]->port, ss_list[id1]->port, ss_list[id2]->port);

                    strcpy(ss_list[i]->backup_port[0], ss_list[id1]->port);
                    strcpy(ss_list[i]->backup_port[1], ss_list[id2]->port);
                    ss_list[id1]->total_backups++;
                    ss_list[id2]->total_backups++;

                    linked_list_head ll = return_paths(ss_list[i]->root);

                    char **paths = (char **)malloc(sizeof(char *) * 100);
                    for (int i = 0; i < 100; i++)
                    {
                        paths[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
                    }

                    int cnt = 0;
                    linked_list_node trav = ll->first;
                    while (trav != NULL)
                    {
                        strcpy(paths[cnt], trav->path);
                        cnt++;
                        trav = trav->next;
                    }

                    for (int j = 0; j < cnt; j++)
                    {
                        insert_path(ss_list[id1]->backup_root, paths[j], ss_list[i]->ssid);
                        insert_path(ss_list[id2]->backup_root, paths[j], ss_list[i]->ssid);

                        if (strstr(paths[j], ".txt") != NULL)
                        {
                            request r = (request)malloc(sizeof(st_request));
                            request put_r = (request)malloc(sizeof(st_request));
                            struct sockaddr_in addr;
                            int sock = socket(AF_INET, SOCK_STREAM, 0);
                            if (sock == -1)
                            {
                                perror("Socket creation failed");
                            }
                            memset(&addr, '\0', sizeof(addr));
                            addr.sin_family = AF_INET;
                            addr.sin_port = htons(atoi(ss_list[i]->port));
                            addr.sin_addr.s_addr = INADDR_ANY;

                            int y = -1;

                            while (y != 0)
                            {
                                y = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
                            }

                            r->request_type = COPY_FILE;
                            strcpy(r->data, paths[j]);

                            int x = send(sock, r, sizeof(st_request), 0);
                            recv(sock, r, sizeof(st_request), 0);

                            close(sock);

                            put_r->request_type = BACKUP_PASTE;
                            strcpy(put_r->data, r->data);

                            struct sockaddr_in addr1;

                            int sock_one = socket(AF_INET, SOCK_STREAM, 0);
                            if (sock_one == -1)
                            {
                                perror("Socket creation failed");
                            }
                            memset(&addr1, '\0', sizeof(addr1));
                            addr1.sin_family = AF_INET;
                            addr1.sin_port = htons(atoi(ss_list[id1]->port));
                            addr1.sin_addr.s_addr = INADDR_ANY;

                            y = -1;

                            while (y != 0)
                            {
                                y = connect(sock_one, (struct sockaddr *)&addr1, sizeof(addr1));
                            }
                            send(sock_one, put_r, sizeof(st_request), 0);
                            close(sock_one);

                            struct sockaddr_in addr2;

                            int sock_two = socket(AF_INET, SOCK_STREAM, 0);
                            if (sock_two == -1)
                            {
                                perror("Socket creation failed");
                            }
                            memset(&addr2, '\0', sizeof(addr2));
                            addr2.sin_family = AF_INET;
                            addr2.sin_port = htons(atoi(ss_list[id2]->port));
                            addr2.sin_addr.s_addr = INADDR_ANY;

                            y = -1;
                            while (y != 0)
                            {
                                y = connect(sock_two, (struct sockaddr *)&addr2, sizeof(addr2));
                            }

                            x = send(sock_two, put_r, sizeof(st_request), 0);
                            close(sock_two);
                        }
                        
                        else{

                            request r = (request)malloc(sizeof(st_request));
                            r->request_type = BACKUP_CREATE_FOLDER;
                            strcpy(r->data,paths[j]);

                            if(ss_list[id1]->status == 1){
                            int sock1=connect_to_port(ss_list[id1]->port);
                            send(sock1,r,sizeof(st_request),0);
                            close(sock1);
                            }
                            if(ss_list[id2]->status == 1){
                            int sock1=connect_to_port(ss_list[id2]->port);
                            send(sock1,r,sizeof(st_request),0);
                            close(sock1);
                            }

                        }

                    }

                    


                    ss_list[i]->is_backedup = 1;
                    pthread_t backup_thread_idx;
                }
            }
        }
    }
}

