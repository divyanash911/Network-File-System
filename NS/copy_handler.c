#include "headers.h"

void *copy_handler(request req, int client_id)
{

    char *source = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
    char *desti = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);

    char *token = strtok(req->data, "|");

    strcpy(source, token);
    token = strtok(NULL, "|");
    strcpy(desti, token);

    if (req->request_type == COPY_FOLDER)
    {

        request r = (request)malloc(sizeof(st_request));
        r->request_type = COPY_FOLDER;
        strcpy(r->data, source);
        ss found_server = NULL;
        ss dest_server = NULL;
        pthread_mutex_lock(&server_lock);

        for (int i = 0; i < server_count; i++)
        {
            if (search_path(ss_list[i]->root, source) >= 0)
            {
                found_server = ss_list[i];
                printf("found source at %d\n", i);
            }
            if (search_path(ss_list[i]->root, desti) >= 0)
            {
                dest_server = ss_list[i];
                printf("found dest at %d\n", i);
            }

            if (found_server != NULL && dest_server != NULL)
            {
                break;
            }
        }
        pthread_mutex_unlock(&server_lock);

        if (found_server == NULL || dest_server == NULL || found_server->status == 0 || dest_server->status == 0)
        {
            r->request_type = FILE_NOT_FOUND;
            strcpy(r->data, "File not found");
            send(client_socket_arr[client_id], r, sizeof(st_request), 0);
            int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }
            client_socket_arr[client_id] = -1;
            close(client_socket_arr[client_id]);
            return NULL;
        }

        else
        {

            printf(YELLOW("Copying folder %s to %s\n"), found_server->port, dest_server->port);
            st_copy_folder *get_r = (st_copy_folder *)malloc(sizeof(st_copy_folder));
            int sock_fd = connect_to_port(found_server->port);
            send(sock_fd, r, sizeof(st_request), 0);
            int logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }
            recv(sock_fd, get_r, sizeof(st_copy_folder), 0);
            logging = insert_log(SS, found_server->ssid, atoi(found_server->port), req->request_type, req->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }

            close(sock_fd);
            request r = (request)malloc(sizeof(st_request));
            printf("%d\n", get_r->num_paths);
            for (int i = 0; i < get_r->num_paths; i++)
            {
                memset(r->data, 0, MAX_DATA_LENGTH);
                r->request_type = PASTE;

                strcat(r->data, desti);
                strcat(r->data, get_r->paths[i]);
                int sock_fd1 = connect_to_port(dest_server->port);
                send(sock_fd1, r, sizeof(st_request), 0);
                int logging = insert_log(SS, dest_server->ssid, atoi(dest_server->port), req->request_type, req->data, OK);
                if (logging == 0)
                {
                    printf(RED("Log addition failed\n"));
                }
                close(sock_fd1);
            }
            r->request_type = ACK;
            strcpy(r->data, "Copying succesful!\n");
            send(client_socket_arr[client_id], r, sizeof(st_request), 0);
            int logging1 = insert_log(CLIENT, 0, NS_PORT, r->request_type, r->data, OK);
            if (logging1 == 0)
            {
                printf(RED("Log addition failed\n"));
            }
        }

        return NULL;
    }
    ss source_no, dest_no;
    int flag = 0;

    pthread_mutex_lock(&server_lock);

    for (int i = 0; i < server_count; i++)
    {
        pthread_mutex_lock(&ss_list[i]->lock);

        if (search_path(ss_list[i]->root, source) >= 0)
        {
            source_no = ss_list[i];
            flag++;
        }
        if (search_path(ss_list[i]->root, desti) >= 0)
        {
            dest_no = ss_list[i];
            flag++;
        }

        if (flag == 2)
        {
            pthread_mutex_unlock(&ss_list[i]->lock);
            break;
        }
        pthread_mutex_unlock(&ss_list[i]->lock);
    }
    pthread_mutex_unlock(&server_lock);
    if (flag < 2 || source_no->status == 0 || dest_no->status == 0)
    {
        request r = (request)malloc(sizeof(st_request));
        r->request_type = FILE_NOT_FOUND;
        strcpy(r->data, "File not found");
        send(client_socket_arr[client_id], r, sizeof(st_request), 0);
        int logging = insert_log(CLIENT, 0, NS_PORT, req->request_type, req->data, FILE_NOT_FOUND);
        if (logging == 0)
        {
            printf(RED("Log addition failed\n"));
        }
    }
    else
    {
        request get_r = (request)malloc(sizeof(st_request));
        get_r->request_type = COPY_FILE;
        strcpy(get_r->data, source);

        int s_fd = connect_to_port(source_no->port);

        send(s_fd, get_r, sizeof(st_request), 0);

        int logging = insert_log(SS, source_no->ssid, atoi(source_no->port), get_r->request_type, get_r->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed\n"));
        }
        recv(s_fd, get_r, sizeof(st_request), 0);
        logging = insert_log(SS, source_no->ssid, atoi(source_no->port), get_r->request_type, get_r->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed\n"));
        }

        if (get_r->request_type != 21)
        {
            printf(RED("Error in copying file with code : %d\n\n\n"), get_r->request_type);
            return NULL;
        }

        close(s_fd);

        char *token = strtok(get_r->data, "|");
        char *token1 = strtok(NULL, "|");
        char *file_name = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        strcpy(file_name, token1);
        char **paths = (char **)malloc(sizeof(char *) * MAX_CONNECTIONS);
        for (int i = 0; i < MAX_CONNECTIONS; i++)
        {
            paths[i] = (char *)malloc(sizeof(char) * MAX_DATA_LENGTH);
        }
        char *token_path = strtok(token, "/");
        int ind = 0;
        while (token_path != NULL)
        {
            strcpy(paths[ind], token_path);
            ind++;
            token_path = strtok(NULL, "/");
        }

        char *new_dest = (char *)calloc(MAX_DATA_LENGTH, sizeof(char));
        snprintf(new_dest, MAX_DATA_LENGTH, "%s/%s", desti, paths[ind - 1]);

        get_r->request_type = PASTE;
        memset(get_r->data, 0, MAX_DATA_LENGTH);
        strcat(get_r->data, new_dest);
        strcat(get_r->data, "|");
        strcat(get_r->data, file_name);
        int s_fd1 = connect_to_port(dest_no->port);

        send(s_fd1, get_r, sizeof(st_request), 0);
        logging = insert_log(SS, dest_no->ssid, atoi(dest_no->port), get_r->request_type, get_r->data, OK);
        if (logging == 0)
        {
            printf(RED("Log addition failed\n"));
        }
        close(s_fd1);

        if (dest_no->is_backedup == 1)
        {
            get_r->request_type = BACKUP_PASTE;

            int s_fd1 = connect_to_port(dest_no->backup_port[0]);

            send(s_fd1, get_r, sizeof(st_request), 0);
            int ssid = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, dest_no->backup_port[0]) == 0)
                {
                    ssid = ss_list[i]->ssid;
                    break;
                }
            }
            logging = insert_log(SS, ssid, atoi(dest_no->backup_port[0]), get_r->request_type, get_r->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }
            close(s_fd1);

            get_r->request_type = BACKUP_PASTE;
            int s_fd2 = connect_to_port(dest_no->backup_port[1]);
            send(s_fd2, get_r, sizeof(st_request), 0);
            int ssid2 = 0;
            for (int i = 0; i < server_count; i++)
            {
                if (strcmp(ss_list[i]->port, dest_no->backup_port[1]) == 0)
                {
                    ssid2 = ss_list[i]->ssid;
                    break;
                }
            }
            logging = insert_log(SS, ssid2, atoi(dest_no->backup_port[1]), get_r->request_type, get_r->data, OK);
            if (logging == 0)
            {
                printf(RED("Log addition failed\n"));
            }
            close(s_fd2);
        }

        request r = (request)malloc(sizeof(st_request));
        r->request_type = ACK;
        strcpy(r->data, "Copying succesful!\n");
        printf(BLUE("Copying succesful!\n\n\n"));
        send(client_socket_arr[client_id], r, sizeof(st_request), 0);
        int logging1 = insert_log(CLIENT, 0, NS_PORT, r->request_type, r->data, OK);
        if (logging1 == 0)
        {
            printf(RED("Log addition failed\n"));
        }
    }
    return NULL;
}

