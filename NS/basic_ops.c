#include "headers.h"

void *basic_ops(request req, int client_id)
{
    if (strstr(req->data, ".txt") == NULL && req->request_type != RETRIEVE_INFO)
    {
        // printf("here coming\n");
        request r = (request)malloc(sizeof(st_request));
        r->request_type = INVALID_FILETYPE;
        strcpy(r->data, "Incompatible file opened!");
        send(client_socket_arr[client_id], r, sizeof(st_request), 0);
        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed.\n"));
        }
        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]);
        return NULL;
    }

    if (search_in_cache(req->request_type, req->data) != NULL)
    {
        st_cache *c = search_in_cache(req->request_type, req->data);
        printf(GREEN("Client request found in cache\n\n\n"));
        // pthread_mutex_lock(&server_lock);
        int id = c->ss_id;
        // printf("\n");
        request r = (request)malloc(sizeof(st_request));
        if (ss_list[id]->status == 1)
        {
            if (req->request_type == READ_REQ || req->request_type == RETRIEVE_INFO)
            {
                if (path_locked_or_not(req->data) != 1)
                {
                    r->request_type = TIMEOUT;
                    strcpy(r->data, "Timeout was done");
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, TIMEOUT);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                    // pthread_mutex_unlock(&server_lock);
                    client_socket_arr[client_id] = -1;
                    close(client_socket_arr[client_id]);
                    return NULL;
                }
                else
                {

                    r->request_type = RES;
                    snprintf(r->data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->client_port);
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                    // pthread_mutex_unlock(&server_lock);
                    client_socket_arr[client_id] = -1;
                    close(client_socket_arr[client_id]);
                    return NULL;
                }
            }
            else if (req->request_type == WRITE_REQ || req->request_type == APPEND_REQ)
            {
                if (path_locked_or_not(req->data) != 1)
                {

                    r->request_type = TIMEOUT;
                    strcpy(r->data, "Timeout was done");
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, TIMEOUT);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                    // pthread_mutex_unlock(&server_lock);
                    client_socket_arr[client_id] = -1;
                    close(client_socket_arr[client_id]);
                    return NULL;
                }
                else
                {

                    insert_path_lock(req->data); // Adding path to write list
                    r->request_type = RES;
                    snprintf(r->data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->client_port);
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                    printf(GREEN("Given path is added into locked paths : %s\n"), req->data);
                    // pthread_mutex_unlock(&server_lock);
                    client_socket_arr[client_id] = -1;
                    close(client_socket_arr[client_id]);
                    return NULL;
                }
            }

            request r = (request)malloc(sizeof(st_request));
            r->request_type = RES;

            char *data = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
            snprintf(data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->client_port);
            strcpy(r->data, data);
            send(client_socket_arr[client_id], r, sizeof(st_request), 0);
            int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }
        }
        else if (ss_list[id]->is_backedup == 1 && req->request_type == READ_REQ)
        {
        
            ss fs1, fs2;
            int id1 = -1, id2 = -1;
            int count = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, ss_list[id]->backup_port[0]) == 0 || strcmp(ss_list[i]->port, ss_list[id]->backup_port[1]) == 0)
                {
                    if (strcmp(ss_list[i]->port, ss_list[id]->backup_port[0]) == 0)
                    {
                        fs1 = ss_list[i];
                        id1 = i;
                        count++;
                    }
                    else
                    {
                        fs2 = ss_list[i];
                        id2 = i;
                        count++;
                    }

                    if (count == 2)
                        break;
                }
            }
            
            if (id1 == -1 && id2 == -1)
            {   
                request r = (request)malloc(sizeof(st_request));
                r->request_type = SERVER_NOT_FOUND;
                strcpy(r->data, "Server offline!");
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
                
                return NULL;
            }

            if (ss_list[id1]->status == 1)
            {
                printf(GREEN("Backup request at %s\n\n\n"), ss_list[id]->backup_port[0]);
                request r = (request)malloc(sizeof(st_request));
                r->request_type = BACKUP_READ_REQ;
                char *data = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
                snprintf(data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->backup_port[0]);
                strcpy(r->data, data);
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
                return NULL;
            }
            else if (ss_list[id2]->status == 1)
            {
                printf(GREEN("Backup request at %s\n\n\n"), ss_list[id]->backup_port[1]);
                request r = (request)malloc(sizeof(st_request));
                r->request_type = BACKUP_READ_REQ;
                char *data = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
                snprintf(data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->backup_port[1]);
                strcpy(r->data, data);
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
                return NULL;
            }
            else
            {
                // printf("here\n");
                request r = (request)malloc(sizeof(st_request));
                r->request_type = SERVER_NOT_FOUND;
                strcpy(r->data, "Server offline!");
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
                return NULL;
            }
        }
        else
        {

            request r = (request)malloc(sizeof(st_request));
            r->request_type = FILE_NOT_FOUND;
            strcpy(r->data, "File not found");
            send(client_socket_arr[client_id], r, sizeof(st_request), 0);
            int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }
        }

        // pthread_mutex_unlock(&server_lock);
        client_socket_arr[client_id] = -1;
        close(client_socket_arr[client_id]);
        return NULL;
    }

    // pthread_mutex_lock(&server_lock);
    int flag = 0;
    int id = 0;
    request r = (request)malloc(sizeof(st_request));
    char *reference = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    for (int i = 0; i < server_count; i++)
    {
        // pthread_mutex_lock(&ss_list[i]->lock);
        if (search_path(ss_list[i]->root, req->data) >= 0)
        {
            snprintf(reference, MAX_DATA_LENGTH, "%s|%s", ss_list[i]->ip, ss_list[i]->client_port);
            flag = 1;
            id = i;
            // pthread_mutex_unlock(&ss_list[i]->lock);
            break;
        }
        // pthread_mutex_unlock(&ss_list[i]->lock);
    }
    // pthread_mutex_unlock(&server_lock);

    if (flag == 0)
    {

        r->request_type = FILE_NOT_FOUND;
        strcpy(r->data, "File not found");
        printf(RED("No files found , informing client\n\n\n"));
        send(client_socket_arr[client_id], r, sizeof(st_request), 0);
        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
        if (logging == 0)
        {
            printf(RED("Log addition failed.\n"));
        }
    }
    else
    {
        insert_in_cache(req->request_type, req->data, id, ss_list[id]->ip, atoi(ss_list[id]->client_port));
        if (req->request_type == READ_REQ || req->request_type == RETRIEVE_INFO) // Reading a file that is being written not possible
        {
            if (path_locked_or_not(req->data) != 1)
            {
                r->request_type = TIMEOUT;
                strcpy(r->data, "Timeout was done");
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, TIMEOUT);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
                client_socket_arr[client_id] = -1;
                close(client_socket_arr[client_id]);
                return NULL;
            }
        }
        else if (req->request_type == WRITE_REQ || req->request_type == APPEND_REQ) // Writing a file that is being written not possible
        {
            if (path_locked_or_not(req->data) != 1)
            {

                r->request_type = TIMEOUT;
                strcpy(r->data, "Timeout was done");
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, TIMEOUT);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
                client_socket_arr[client_id] = -1;
                close(client_socket_arr[client_id]);
                return NULL;
            }
            insert_path_lock(req->data); // Adding path to write list
            printf(GREEN("Given path is added into locked paths : %s\n"), req->data);
        }

        if (ss_list[id]->status == 1)
        {
            r->request_type = RES;
            strcpy(r->data, reference);
            send(client_socket_arr[client_id], r, sizeof(st_request), 0);
            int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed.\n"));
            }
        }
        else
        {
            // printf("hi\n");
            if (req->request_type == READ_REQ && ss_list[id]->is_backedup == 1)
            {
                // allow

                ss fs1, fs2;
                int id1 = -1, id2 = -1;
                int count = 0;
                for (int i = 0; i < server_count; i++)
                {
                    if (strcmp(ss_list[i]->port, ss_list[id]->backup_port[0]) == 0 || strcmp(ss_list[i]->port, ss_list[id]->backup_port[1]) == 0)
                    {
                        if (strcmp(ss_list[i]->port, ss_list[id]->backup_port[0]) == 0)
                        {
                            fs1 = ss_list[i];
                            id1 = i;
                            count++;
                        }
                        else
                        {
                            fs2 = ss_list[i];
                            id2 = i;
                            count++;
                        }

                        if (count == 2)
                            break;
                    }
                }

                if (id1 == -1 && id2 == -1)
                {
                    return NULL;
                }

                if (ss_list[id1]->status == 1)
                {
                    r->request_type = BACKUP_READ_REQ;
                    snprintf(r->data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->backup_port[0]);
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                }
                else if (ss_list[id2]->status == 1)
                {
                    r->request_type = BACKUP_READ_REQ;
                    snprintf(r->data, MAX_DATA_LENGTH, "%s|%s", ss_list[id]->ip, ss_list[id]->backup_port[1]);
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                }
                else{

                    r->request_type = SERVER_NOT_FOUND;
                    strcpy(r->data, "Server offline!");
                    send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                    int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, OK);
                    if (logging == 0)
                    {
                        printf(RED("Log addition failed.\n"));
                    }
                    return NULL;
                }
            }
            else
            {
                r->request_type = FILE_NOT_FOUND;
                strcpy(r->data, "File not found");
                printf(RED("No files found , informing client\n\n\n"));
                send(client_socket_arr[client_id], r, sizeof(st_request), 0);
                int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
                if (logging == 0)
                {
                    printf(RED("Log addition failed.\n"));
                }
            }
        }
    }
    return NULL;
}
