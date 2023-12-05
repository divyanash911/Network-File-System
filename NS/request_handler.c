#include "headers.h"

// Code to process the request according to request type

void *process(void *arg)
{
    proc n = (proc)arg;
    int client_id = n->client_id;
    request req = (request)malloc(sizeof(st_request));
    req->request_type = n->request_type;
    strcpy(req->data, n->data);

    if (req->request_type == REGISTRATION_REQUEST)
    {

        init_storage(req->data);
        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]); // Code to add a new storage server in naming server list
        return NULL;
    }
    else if (req->request_type == WRITE_REQ || req->request_type == READ_REQ || req->request_type == RETRIEVE_INFO || req->request_type == APPEND_REQ)
    {
        printf(BLUE("New request received from client number! %d\n\n\n"), client_id);
        basic_ops(req, client_id);
        if (client_socket_arr[client_id] > 0)
        {
            client_socket_arr[client_id] = -1;
            close(client_socket_arr[client_id]);
        }
        return NULL;
    }
    else if (req->request_type == DELETE_FOLDER || req->request_type == DELETE_FILE)
    {
        handle_delete(req, client_id);
        printf(BLUE("New delete request from client %d\n\n\n"), client_id);
        if (client_socket_arr[client_id] > 0)
        {
            client_socket_arr[client_id] = -1;
            close(client_socket_arr[client_id]);
        }
        return NULL;
    }
    else if (req->request_type == CREATE_FOLDER || req->request_type == CREATE_FILE)
    {
        printf(BLUE("New create request from client %d\n\n\n"), client_id);
        handle_create(req, client_id);
        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]);
    }

    else if (req->request_type == COPY_FILE || req->request_type == COPY_FOLDER)
    {
        printf("Copy request received from client %d\n\n\n", client_id);
        copy_handler(req, client_id);
        if (client_socket_arr[client_id] > 0)
        {
            client_socket_arr[client_id] = -1;
            close(client_socket_arr[client_id]);
        }
        return NULL;
    }
    else if (req->request_type == ADD_PATHS)
    {
        char *ss_id = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        char **path = (char **)malloc(sizeof(char *) * MAX_CONNECTIONS);
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            path[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        }
        int ind = 0;
        char *token = strtok(req->data, "|");
        while (token != NULL)
        {
            if (ind == 0)
            {
                strcpy(ss_id, token);
            }
            else
            {
                strcpy(path[ind - 1], token);
            }
            ind++;
            token = strtok(NULL, "|");
        }
        int count = 0;
        ss found_server;

        int id = -1;
        for (int i = 0; i < server_count; i++)
        {
            if (ss_list[i]->ssid == atoi(ss_id))
            {
                found_server = ss_list[i];
                id = i;
                break;
            }
        }

        pthread_mutex_lock(&found_server->lock);
        for (int i = 0; i < ind - 1; i++)
        {
            strcpy(found_server->paths[found_server->path_count + i], path[i]);
            if (search_path(found_server->root, path[i]) == -1)
            {
                if (insert_path(found_server->root, path[i], atoi(ss_id)) == 1)
                {
                    count++;
                }

                if (found_server->is_backedup == 1)
                {
                    if (strstr(path[i], ".txt") != NULL)
                    {
                        request r = (request)malloc(sizeof(st_request));
                        r->request_type = COPY_FILE;
                        strcpy(r->data, path[i]);

                        int sock_one = connect_to_port(found_server->port);
                        if (sock_one == -1)
                        {
                            printf(RED("Server %s is down\n"), found_server->port);
                            continue;
                        }

                        if (send(sock_one, r, sizeof(st_request), 0) < 0)
                        {
                            printf(RED("Error in sending request to server %s\n"), found_server->port);
                        }

                        int logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
                        if (logging == 0)
                        {
                            printf(RED("Log addition failed\n"));
                        }

                        if (recv(sock_one, r, sizeof(st_request), 0) < 0)
                        {
                            printf(RED("Error in receiving request from server %s\n"), found_server->port);
                        }

                        if (close(sock_one) < 0)
                        {
                            printf(RED("Error in closing socket %s\n"), found_server->port);
                        }

                        r->request_type = BACKUP_PASTE;

                        int sock_two = connect_to_port(found_server->backup_port[0]);

                        if (sock_two < 0)
                        {
                            printf(RED("Server %s is down\n"), found_server->backup_port[0]);
                            continue;
                        }

                        if (send(sock_two, r, sizeof(st_request), 0) < 0)
                        {
                            printf(RED("Error in sending request to server %s\n"), found_server->backup_port[0]);
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
                            printf(RED("Log addition failed\n"));
                        }

                        if (close(sock_two) < 0)
                        {
                            printf(RED("Error in closing socket %s\n"), found_server->backup_port[0]);
                        }

                        int sock_three = connect_to_port(found_server->backup_port[1]);

                        if (sock_three < 0)
                        {
                            printf(RED("Server %s is down\n"), found_server->backup_port[1]);
                            continue;
                        }

                        if (send(sock_three, r, sizeof(st_request), 0) < 0)
                        {
                            printf(RED("Error in sending request to server %s\n"), found_server->backup_port[1]);
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
                            printf(RED("Log addition failed\n"));
                        }

                        if (close(sock_three) < 0)
                        {
                            printf(RED("Error in closing socket %s\n"), found_server->backup_port[1]);
                        }
                    }

                    else
                    {
                        request r = (request)malloc(sizeof(st_request));
                        r->request_type = BACKUP_CREATE_FOLDER;
                        strcpy(r->data, path[i]);

                        int sock_one = connect_to_port(found_server->backup_port[0]);

                        if (sock_one < 0)
                        {
                            printf(RED("Server %s is down\n"), found_server->backup_port[0]);
                            continue;
                        }

                        if (send(sock_one, r, sizeof(st_request), 0) < 0)
                        {
                            printf(RED("Error in sending request to server %s\n"), found_server->backup_port[0]);
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
                        int logging = insert_log(SS, ssid, atoi(found_server->backup_port[0]), r->request_type, r->data, OK);
                        if (logging == 0)
                        {
                            printf(RED("Log addition failed\n"));
                        }
                        if (close(sock_one) < 0)
                        {
                            printf(RED("Error in closing socket %s\n"), found_server->backup_port[0]);
                        }

                        int sock_two = connect_to_port(found_server->backup_port[1]);

                        if (sock_two < 0)
                        {
                            printf(RED("Server %s is down\n"), found_server->backup_port[1]);
                            continue;
                        }

                        if (send(sock_two, r, sizeof(st_request), 0) < 0)
                        {
                            printf(RED("Error in sending request to server %s\n"), found_server->backup_port[1]);
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
                            printf(RED("Log addition failed\n"));
                        }

                        if (close(sock_two) < 0)
                        {
                            printf(RED("Error in closing socket %s\n"), found_server->backup_port[1]);
                        }
                    }
                }
            }
        }

        found_server->added = 1;
        found_server->path_count = found_server->path_count + ind - 1;
        found_server->synced = 0;
        pthread_mutex_unlock(&found_server->lock);
        if (count > 0)
            printf(BLUE("Added %d new files/directories from server  %s\n\n\n"), count, found_server->port);

        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]);

        return NULL;
    }

    else if (req->request_type == DELETE_PATHS)
    {
        char *ss_id = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        char **path = (char **)malloc(sizeof(char *) * MAX_CONNECTIONS);
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            path[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        }
        int tkn_cnt = 0;
        char *token = strtok(req->data, "|");
        while (token != NULL)
        {
            if (tkn_cnt == 0)
            {
                strcpy(ss_id, token);
            }
            else
            {
                strcpy(path[tkn_cnt - 1], token);
            }
            tkn_cnt++;
            token = strtok(NULL, "|");
        }
        int count = 0;
        ss found_server = ss_list[atoi(ss_id) - 1];
        pthread_mutex_lock(&found_server->lock);
        for (int i = 0; i < tkn_cnt - 1; i++)
        {
            if (search_path(found_server->root, path[i]) >= 0)
            {
                if (delete_path(found_server->root, path[i]) == 1)
                {
                    count++;
                }

                if (found_server->is_backedup == 1)
                {

                    request r = (request)malloc(sizeof(st_request));

                    if (strstr(path[i], ".txt") != NULL)
                    {
                        r->request_type = BACKUP_DELETE_FILE;
                    }
                    else
                        r->request_type = BACKUP_DELETE_FOLDER;

                    strcpy(r->data, path[i]);

                    int sock_one = connect_to_port(found_server->backup_port[0]);
                    send(sock_one, r, sizeof(st_request), 0);
                    int ssid = 0;
                    for (int i = 0; i < server_count; i++)
                    {
                        if (strcmp(ss_list[i]->port, found_server->backup_port[0]) == 0)
                        {
                            ssid = ss_list[i]->ssid;
                            break;
                        }
                    }
                    int logging = insert_log(SS, ssid, atoi(found_server->backup_port[0]), r->request_type, r->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed\n"));
                    }
                    close(sock_one);

                    int sock_two = connect_to_port(found_server->backup_port[1]);
                    send(sock_two, r, sizeof(st_request), 0);
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
                        printf(RED("Log addition failed\n"));
                    }
                    close(sock_two);
                }

                found_server->path_count--;
            }
        }
        found_server->synced = 0;
        pthread_mutex_unlock(&found_server->lock);
        if (count > 0)
            printf(BLUE("Deleted %d files/directories from server number %d\n\n\n"), count, atoi(ss_id));
        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]);
        return NULL;
    }
    // Yet to work on depending on type of requests
    else if (req->request_type == LIST)
    {
        printf(BLUE("List request received from client %d\n\n\n"), client_id);
        char *list = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        strcpy(list, "");
        for (int i = 0; i < server_count; i++)
        {
            if (ss_list[i]->status == 1 || ss_list[i]->is_backedup == 1)
            {
                linked_list_head head = return_paths(ss_list[i]->root);
                linked_list_node trav = head->first;
                while (trav != NULL)
                {
                    char *temp = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
                    strcpy(temp, trav->path);
                    strcat(temp, "|");
                    strcat(list, temp);
                    trav = trav->next;
                }
            }
        }
        request r = (request)malloc(sizeof(st_request));
        r->request_type = RES;
        strcpy(r->data, list);
        send(client_socket_arr[client_id], r, sizeof(st_request), 0);
        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed\n"));
        }
        return NULL;
    }
    else if (req->request_type == WRITE_APPEND_COMP)
    {
        delete_path_lock(req->data);
        printf(GREEN("Given path is deleted from locked paths : %s\n"), req->data);
    }

    else if (req->request_type == CONSISTENT_WRITE)
    {
        char *token = strtok(req->data, "|");
        char *token1 = strtok(NULL, "|");
        ss found_server;
        for (int i = 0; i < server_count; i++)
        {
            if (search_path(ss_list[i]->root, token) >= 0)
            {
                found_server = ss_list[i];
                break;
            }
        }

        if (found_server->is_backedup == 1)
        {
            request r = (request)malloc(sizeof(st_request));
            r->request_type = BACKUP_WRITE_REQ;

            snprintf(r->data, sizeof(r->data), "%s|%s", token, token1);

            int sock_one = connect_to_port(found_server->backup_port[0]);
            send(sock_one, r, sizeof(st_request), 0);
            int ssid = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, found_server->backup_port[0]) == 0)
                {
                    ssid = ss_list[i]->ssid;
                    break;
                }
            }
            int logging = insert_log(SS, ssid, atoi(found_server->backup_port[0]), r->request_type, r->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }
            close(sock_one);

            int sock_two = connect_to_port(found_server->backup_port[1]);
            send(sock_two, r, sizeof(st_request), 0);
            int ssid2 = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, found_server->backup_port[1]) == 0)
                {
                    ssid2 = ss_list[i]->ssid;
                    break;
                }
            }
            logging = insert_log(SS, ssid, atoi(found_server->backup_port[1]), r->request_type, r->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }
            close(sock_two);
        }
        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]);
        return NULL;
    }

    client_socket_arr[client_id] = -1;
    close(client_socket_arr[client_id]);
    return NULL;
}

