#include "headers.h"

// This file handles creation and deletion of files in the storage servers and also ensures redundancy (consistency with backup)

void *handle_create(request req, int client_id)
{
    int flag = 0;
    request r = (request)malloc(sizeof(st_request));
    memset(r->data, 0, MAX_DATA_LENGTH);
    char *reference = (char *)calloc(MAX_DATA_LENGTH, sizeof(char));
    char *path = (char *)calloc(MAX_DATA_LENGTH, sizeof(char));

    char *search_path_string = strtok(req->data, "|");
    char *name = strtok(NULL, "|");

    ss found_server;
    pthread_mutex_lock(&server_lock);
    for (int i = 0; i < server_count; i++)
    {
        pthread_mutex_lock(&ss_list[i]->lock);
        if (search_path(ss_list[i]->root, search_path_string) >= 0)
        {
            snprintf(reference, MAX_DATA_LENGTH, "%s|%s", ss_list[i]->ip, ss_list[i]->client_port);
            flag = 1;
            found_server = ss_list[i];
            pthread_mutex_unlock(&ss_list[i]->lock);
            break;
        }
        pthread_mutex_unlock(&ss_list[i]->lock);
    }
    pthread_mutex_unlock(&server_lock);

    if (flag == 0 || found_server->status == 0)
    {
        if (flag == 0)
        {
            r->request_type = FILE_NOT_FOUND;
        }
        else
        {
            r->request_type = SERVER_NOT_FOUND;
        }
        strcpy(r->data, "File/Directory not found");

        if (send(client_socket_arr[client_id], r, sizeof(st_request), 0) < 0)
        {
            printf(RED("Error in sending data to client \n"));
        }
        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
        if (logging == 0)
        {
            printf(RED("Log addition failed.\n"));
        }
    }
    else
    {
        r->request_type = req->request_type;
        snprintf(r->data, sizeof(r->data), "%s/%s", search_path_string, name);

        int s_fd = connect_to_port(found_server->port);

        if (s_fd < 0)
        {
            printf(RED("Error in connecting to server\n"));
        }

        if (send(s_fd, r, sizeof(st_request), 0) < 0)
        {
            printf(RED("Error in sending data to server \n"));
        }

        int logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed.\n"));
        }

        if (recv(s_fd, r, sizeof(st_request), 0) < 0)
        {
            printf(RED("Error in receiving data from server \n"));
        }

        logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed.\n"));
        }

        if (close(s_fd) < 0)
        {
            printf(RED("Error in closing connection with server \n"));
        }

        if (r->request_type != ACK)
        {
            printf(RED("Error in creating file/folder with code : %d\n\n\n"), r->request_type);
            send(client_socket_arr[client_id], r, sizeof(st_request), 0);
            return NULL;
        }

        if (found_server->is_backedup == 1)
        {
            if (req->request_type == CREATE_FOLDER)
            {
                r->request_type = BACKUP_CREATE_FOLDER;
            }
            else
            {
                r->request_type = BACKUP_CREATE_FILE;
            }
            snprintf(r->data, sizeof(r->data), "%s/%s", search_path_string, name);
            int s_fd = connect_to_port(found_server->backup_port[0]);

            if (send(s_fd, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending data to server \n"));
            }

            int ssid = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, found_server->backup_port[0]) == 0)
                {
                    ssid = ss_list[i]->ssid;
                    break;
                }
            }

            logging = insert_log(SS, ssid, atoi(found_server->backup_port[0]), r->request_type, r->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }

            if (close(s_fd) < 0)
            {
                printf(RED("Error in closing connection with server \n"));
            }

            if (req->request_type == CREATE_FOLDER)
            {
                r->request_type = BACKUP_CREATE_FOLDER;
            }
            else
            {
                r->request_type = BACKUP_CREATE_FILE;
            }
            snprintf(r->data, sizeof(r->data), "%s/%s", search_path_string, name);
            s_fd = connect_to_port(found_server->backup_port[1]);

            if (s_fd < 0)
            {
                printf(RED("Error in connecting to server\n"));
            }

            if (send(s_fd, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending data to server\n"));
            }

            int ssid2 = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, found_server->backup_port[1]) == 0)
                {
                    ssid2 = ss_list[i]->ssid;
                    break;
                }
            }
            logging = insert_log(SS, ssid2, atoi(found_server->backup_port[1]), r->request_type, r->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }

            if (close(s_fd) < 0)
            {
                printf(RED("Error in closing connection with server\n"));
            }
        }

        r->request_type = ACK;
        strcpy(r->data, "Operation succesful!\n");

        if (send(client_socket_arr[client_id], r, sizeof(st_request), 0) < 0)
        {
            printf(RED("Error in sending data to client \n"));
        }

        int logging1 = insert_log(CLIENT, 0, NS_PORT, r->request_type, r->data, OK);
        if (logging1 == 0)
        {
            printf(RED("Log addition failed.\n"));
        }
    }
    return NULL;
}

void *handle_delete(request req, int client_id)
{
    pthread_mutex_lock(&server_lock);
    int flag = 0;
    request r = (request)malloc(sizeof(st_request));
    memset(r->data, 0, MAX_DATA_LENGTH);
    char *reference = (char *)calloc(MAX_DATA_LENGTH, sizeof(char));
    char *path = (char *)calloc(MAX_DATA_LENGTH, sizeof(char));
    ss found_server;
    for (int i = 0; i < server_count; i++)
    {
        pthread_mutex_lock(&ss_list[i]->lock);
        if (search_path(ss_list[i]->root, req->data) >= 0)
        {
            snprintf(reference, MAX_DATA_LENGTH, "%s|%s", ss_list[i]->ip, ss_list[i]->client_port);
            flag = 1;
            found_server = ss_list[i];
            pthread_mutex_unlock(&ss_list[i]->lock);
            break;
        }
        pthread_mutex_unlock(&ss_list[i]->lock);
    }
    pthread_mutex_unlock(&server_lock);

    if (flag == 0)
    {
        r->request_type = FILE_NOT_FOUND;
        strcpy(r->data, "File/Directory not found");

        if (send(client_socket_arr[client_id], r, sizeof(st_request), 0) < 0)
        {
            printf(RED("Error in sending data to client \n"));
        }

        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
        if (logging == 0)
        {
            printf(RED("Log addition failed.\n"));
        }
    }
    else
    {
        if (found_server->status == 1)
        {
            r->request_type = req->request_type;
            strcpy(r->data, req->data);

            int s_fd = connect_to_port(found_server->port);

            if (s_fd < 0)
            {
                printf(RED("Error in connecting to server\n"));
            }

            if (send(s_fd, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending data to server \n"));
            }

            int logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }

            if (recv(s_fd, r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in receiving data from server \n"));
            }

            logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }

            if (close(s_fd) < 0)
            {
                printf(RED("Error in closing connection with server \n"));
            }

            if (r->request_type != ACK)
            {
                printf(RED("Error in creating file/folder with code : %d\n\n\n"), r->request_type);
                return NULL;
            }

            if (found_server->is_backedup == 1)
            {
                int id1 = -1, id2 = -1;

                for (int i = 0; i < server_count; i++)
                {
                    if (strcmp(ss_list[i]->port, found_server->backup_port[0]) == 0)
                    {
                        id1 = i;
                    }
                    if (strcmp(ss_list[i]->port, found_server->backup_port[1]) == 0)
                    {
                        id2 = i;
                    }
                    if (id1 != -1 && id2 != -1)
                        break;
                }

                if (req->request_type == DELETE_FOLDER)
                {
                    r->request_type = BACKUP_DELETE_FOLDER;
                }
                else
                {
                    r->request_type = BACKUP_DELETE_FILE;
                }

                if (ss_list[id1]->status == 1)
                {
                    strcpy(r->data, req->data);
                    s_fd = connect_to_port(found_server->backup_port[0]);

                    if (s_fd < 0)
                    {
                        printf(RED("Error in connecting to server\n"));
                    }

                    if (send(s_fd, r, sizeof(st_request), 0) < 0)
                    {
                        printf(RED("Error in sending data to server \n"));
                    }

                    int ssid = 0;
                    for (int i = 0; i < server_count; i++)
                    {
                        if (strcmp(ss_list[i]->port, found_server->backup_port[0]) == 0)
                        {
                            ssid = ss_list[i]->ssid;
                            break;
                        }
                    }
                    logging = insert_log(SS, ssid, atoi(found_server->backup_port[0]), r->request_type, r->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }

                    if (close(s_fd) < 0)
                    {
                        printf(RED("Error in closing connection with server \n"));
                    }
                }

                if (ss_list[id2]->status == 1)
                {
                    if (req->request_type == DELETE_FOLDER)
                    {
                        r->request_type = BACKUP_DELETE_FOLDER;
                    }
                    else
                    {
                        r->request_type = BACKUP_DELETE_FILE;
                    }
                    strcpy(r->data, req->data);
                    s_fd = connect_to_port(found_server->backup_port[1]);

                    if (s_fd < 0)
                    {
                        printf(RED("Error in connecting to server\n"));
                    }

                    if (send(s_fd, r, sizeof(st_request), 0) < 0)
                    {
                        printf(RED("Error in sending data to server\n"));
                    }

                    int ssid2 = 0;
                    for (int i = 0; i < server_count; i++)
                    {
                        if (strcmp(ss_list[i]->port, found_server->backup_port[1]) == 0)
                        {
                            ssid2 = ss_list[i]->ssid;
                            break;
                        }
                    }
                    logging = insert_log(SS, ssid2, atoi(found_server->backup_port[1]), r->request_type, r->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }

                    if (close(s_fd) < 0)
                    {
                        printf(RED("Error in closing connection with server \n"));
                    }
                }
            }

            r->request_type = ACK;
            strcpy(r->data, "Operation succesful!\n");

            if (send(client_socket_arr[client_id], r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending data to client \n"));
            }

            int logging2 = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
            if (logging2 == 0)
            {
                printf(RED("Log addition failed.\n"));
            }

            if (close(s_fd) < 0)
            {
                printf(RED("Error in closing connection with server \n"));
            }
        }
        else
        {
            r->request_type = SERVER_NOT_FOUND;
            strcpy(r->data, "File not found");

            if (send(client_socket_arr[client_id], r, sizeof(st_request), 0) < 0)
            {
                printf(RED("Error in sending data to client \n"));
            }

            int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }
        }
    }
    return NULL;
}

