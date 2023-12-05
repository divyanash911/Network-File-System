#include "headers.h"

// Periodically keeps checking for all the file paths and if some new file path is created/deleted then immediately signals the NFS to add/delete that path
void *check_and_store_filepaths(void *args)
{
    while (1)
    {
        pthread_mutex_lock(&accessible_paths_mutex);

        char base_dir_path[MAX_PATH_LEN] = {0};
        sprintf(base_dir_path, "%s", PWD);

        // Linked list to store the paths found as we don't know in advance how many paths will be found
        linked_list_head paths = create_linked_list_head();

        // Searching the SS_test_dir recursively to obtain the absolute paths of all the files
        seek(base_dir_path, paths);

        // Print the number of paths found for debugging
    

        // Number of paths found
        int num_paths_found = paths->number_of_nodes;

        // Storing relative paths of the files found in the found_paths array
        char **found_paths = (char **)malloc(num_paths_found * sizeof(char *));

        linked_list_node n = paths->first;
        int idx = 0;
        while (n != NULL)
        {
            found_paths[idx] = (char *)calloc(MAX_PATH_LEN, sizeof(char));
            strcpy(found_paths[idx], ".");
            strcat(found_paths[idx++], &n->path[strlen(PWD)]);
            n = n->next;
        }

        // Have copied all the found paths in the array so now we can free the linked list
        free_linked_list(paths);

        for(int k=0 ; k< num_paths_found; k++){
            if(strstr(found_paths[k],"/backup")!=NULL){
                free(found_paths[k]);
                found_paths[k]=NULL;
            }
        }
        // Now go and match each paths in not_accessible_paths and found_paths
        for (int k = 0; k < num_of_not_accessible_paths_stored; k++)
        {
            char *curr_not_accessible_path = not_accessible_paths[k];
            for (int j = 0; j < num_paths_found; j++)
            {
                char *curr_found_path = found_paths[j];
                if (curr_found_path != NULL)
                {
                    if (strcmp(curr_found_path, curr_not_accessible_path) == 0)
                    {
                        free(found_paths[j]);
                        found_paths[j] = NULL;
                        break;
                    }
                }
            }
        }

        char deleted_paths[MAX_DATA_LENGTH - 1000] = {0};
        for (int i = 0; i < num_of_paths_stored; i++)
        {
            char* curr_acc_path = accessible_paths[i];
            int flag = 0;
            for (int j = 0; j < num_paths_found; j++)
            {
                char* curr_found_path = found_paths[j];
                if (curr_found_path != NULL)
                {
                    if (strcmp(curr_acc_path, curr_found_path) == 0)
                    {
                        flag = 1;
                        break;
                    }
                }
            }
            if (flag == 0)
            {
                strcat(deleted_paths, curr_acc_path);
                strcat(deleted_paths, "|");
            }
        }
        // Removing the last | from the concatenation of paths
        if (strlen(deleted_paths) > 0)
        {
            deleted_paths[strlen(deleted_paths) - 1] = '\0';
            if (send_update_paths_request(DELETE_PATHS, deleted_paths) != 0)
            {
                fprintf(stderr, RED("Could not send delete paths request.\n"));
                goto Free;
            }
        }

        char new_paths[MAX_DATA_LENGTH - 1000] = {0};
        for (int i = 0; i < num_paths_found; i++)
        {
            char* curr_found_path = found_paths[i];
            if (curr_found_path != NULL)
            {
                int flag = 0;
                for (int j = 0; j < num_of_paths_stored; j++)
                {
                    char* curr_acc_path = accessible_paths[j];
                    if (curr_acc_path != NULL)
                    {
                        if (strcmp(curr_acc_path, curr_found_path) == 0)
                        {
                            flag = 1;
                            break;
                        }
                    }
                }
                if (flag == 0)
                {
                    strcat(new_paths, curr_found_path);
                    strcat(new_paths, "|");
                }
            }
        }
        // Removing the last | from the concatenation of paths
        if (strlen(new_paths) > 0)
        {
            new_paths[strlen(new_paths) - 1] = '\0';
            if (send_update_paths_request(ADD_PATHS, new_paths) != 0)
            {
                fprintf(stderr, RED("Could not send add paths request.\n"));
                goto Free;
            }
        }

        // Copying all the found paths in accessible_paths
        int acc_idx = 0;
        for (int i = 0; i < num_paths_found; i++)
        {
            char* curr_found_path = found_paths[i];
            if (curr_found_path != NULL)
            {
                memset(accessible_paths[i], 0, MAX_PATH_LEN);
                strcpy(accessible_paths[acc_idx], found_paths[i]);
                acc_idx++;
            }
        }

        int n_new_num_found_paths = 0;
        for (int i = 0; i < num_paths_found; i++)
        {
            if (found_paths[i] != NULL)
            {
                n_new_num_found_paths++;
            }
        }
        num_of_paths_stored = n_new_num_found_paths;

        // Freeing all the memory allocated
    Free:
        for (int i = 0; i < num_paths_found; i++)
        {
            if (found_paths[i] != NULL)
            {
                free(found_paths[i]);
                found_paths[i] = NULL;
            }
        }
        free(found_paths);

        pthread_mutex_unlock(&accessible_paths_mutex);

        // Keep checking every 5 seconds
        sleep(5);
    }

    return NULL;
}

// Periodically keeps checking for all the file paths and if some new file path is created/deleted then immediately signals the NFS to add/delete that path
void *check_and_store_backup_paths(void *args)
{
    while (1)
    {
        pthread_mutex_lock(&backup_paths_mutex);

        char base_dir_path[MAX_PATH_LEN] = {0};
        sprintf(base_dir_path, "%s/backup", PWD);

        // Linked list to store the paths found as we don't know in advance how many paths will be found
        linked_list_head paths = create_linked_list_head();

        // Searching the SS_test_dir recursively to obtain the absolute paths of all the files
        seek(base_dir_path, paths);

        // Print the number of paths found for debugging
        

        // Number of paths found
        int num_backup_paths_found = paths->number_of_nodes;

        linked_list_node n = paths->first;
        int idx = 0;
        while (n != NULL)
        {
            memset(backup_paths[idx], 0, MAX_PATH_LEN);
            strcpy(backup_paths[idx], ".");
            strcat(backup_paths[idx], &n->path[strlen(PWD)]);
            n = n->next;
        }
        num_of_backup_paths_stored = num_backup_paths_found;
        // Have copied all the found paths in the array so now we can free the linked list
        free_linked_list(paths);

        pthread_mutex_unlock(&backup_paths_mutex);

        // Keep checking every 5 seconds
        sleep(5);
    }

    return NULL;
}

// Processes the request allocated to it in the allocated thread and then returns
void *serve_request(void *args)
{
    st_thread_data meta_data = *((thread_data)args);
    int sock_fd = meta_data.client_sock_fd;  // Socket id for communicating with the node which has sent the request
    int thread_index = meta_data.thread_idx; // Index of the thread on which this is running

    // Freeing arguments as all the information is extracted
    free(args);

    // Receiving and serving the request
    st_request recvd_request;
    memset(recvd_request.data, 0, MAX_DATA_LENGTH);

    // Receiving the request
    int recvd_msg_size;
    if ((recvd_msg_size = recv(sock_fd, &recvd_request, sizeof(st_request), 0)) <= 0)
    {
        fprintf(stderr, RED("recv  : %s\n"), strerror(errno));
        send_ack(REQ_UNSERVICED, sock_fd, strerror(errno));
        goto End2;
    }

    char **request_tkns = tokenize(recvd_request.data, '|');
    /*
        READ data format : <path>
        WRITE data format : <path>|<content to write> (Keep sending write data in this format and when all the data to be written is sent send the stop request)
        APPEND data format : same as write
    */

    // Selecting the type of request sent
    // Except for reads and backup alter requests I am sending ack for each request
    if (recvd_request.request_type == READ_REQ || recvd_request.request_type == BACKUP_READ_REQ)
    {
        char* path_to_read;
        if (recvd_request.request_type == READ_REQ)
        {
            printf(YELLOW("Read request received.\n"));
            // Read the data in the file specified (Assuming that all the data can be read within the size of the request data buffer)
            path_to_read = request_tkns[0];
        }
        else
        {
            printf(YELLOW("Backup Read request received.\n"));
            // Read the data in the file specified (Assuming that all the data can be read within the size of the request data buffer)
            path_to_read = replace_storage_by_backup(request_tkns[0]);
        }

        FILE* fptr = fopen(path_to_read, "r");
        if (fptr == NULL)
        {
            fprintf(stderr, RED("fopen : could not open file to read : %s\n"), strerror(errno));
            if (recvd_request.request_type == READ_REQ)
            {
                send_ack(READ_FAILED, sock_fd, strerror(errno));
            }
            else
            {
                free(path_to_read);
            }
            goto End;
        }

        st_request send_read_data;
        send_read_data.request_type = READ_REQ_DATA;
        memset(send_read_data.data, 0, MAX_DATA_LENGTH);

        int n_bytes_read;

        while ((n_bytes_read = fread(send_read_data.data, sizeof(char), MAX_DATA_LENGTH - 1, fptr)) > 0)
        {
            int sent_msg_size;
            if ((sent_msg_size = send(sock_fd, (request)&send_read_data, sizeof(st_request), 0)) <= 0)
            {
                fprintf(stderr, RED("send : could not send the read data : %s\n"), strerror(errno));
                if (recvd_request.request_type == READ_REQ)
                {
                    send_ack(READ_FAILED, sock_fd, strerror(errno));
                }
                else
                {
                    free(path_to_read);
                }
                goto End;
            }

            memset(send_read_data.data, 0, MAX_DATA_LENGTH);
        }
        sleep(1);
        send_ack(ACK, sock_fd, NULL);

        if (recvd_request.request_type == BACKUP_READ_REQ)
        {
            free(path_to_read);
        }
    }
    else if (recvd_request.request_type == WRITE_REQ || recvd_request.request_type == BACKUP_WRITE_REQ)
    {
        char* path_to_write;
        if (recvd_request.request_type == WRITE_REQ)
        {
            printf(YELLOW("Write request received.\n"));
            path_to_write = request_tkns[0];
        }
        else
        {
            printf(YELLOW("Backup Write request received.\n"));
            path_to_write = replace_storage_by_backup(request_tkns[0]);
        }
        
        char *data_to_write = request_tkns[1];

        // Opening file in write mode and writing onto the file (Assuming that the path is always correct and the file already exits)
        FILE *fptr = fopen(path_to_write, "w");
        if (fptr == NULL)
        {
            fprintf(stderr, RED("fopen : could not open file to write : %s\n"), strerror(errno));
            if (recvd_request.request_type == WRITE_REQ)
            {
                send_ack(WRITE_FAILED, sock_fd, strerror(errno));
            }
            else
            {
                free(path_to_write);
            }
            goto End;
        }
        fprintf(fptr, "%s", data_to_write);
        fclose(fptr);

        if (recvd_request.request_type == BACKUP_WRITE_REQ)
        {
            free(path_to_write);
            // No need to send ack as backup is done asynchronously
        }
        else
        {
            FILE* fptr2 = fopen(path_to_write, "r");
            char buffer[MAX_DATA_LENGTH - MAX_PATH_LEN - 1] = {0};
            fread(buffer, sizeof(char), MAX_DATA_LENGTH - MAX_PATH_LEN - 2, fptr2);
            fclose(fptr2);

            char final_str[MAX_DATA_LENGTH] = {0};
            strcpy(final_str, path_to_write);
            strcat(final_str, "|");
            strcat(final_str, buffer);

            send_msg_to_nfs(final_str, CONSISTENT_WRITE);

            send_ack(ACK, sock_fd, NULL);
        }
    }
    else if (recvd_request.request_type == APPEND_REQ || recvd_request.request_type == BACKUP_APPEND_REQ)
    {
        char* path_to_write;
        if (recvd_request.request_type == APPEND_REQ)
        {
            printf(YELLOW("Append request received.\n"));
            path_to_write = request_tkns[0];
        }
        else
        {
            printf(YELLOW("Backup Append request received.\n"));
            path_to_write = replace_storage_by_backup(request_tkns[0]);
        }
        
        char *data_to_write = request_tkns[1];

        FILE *fptr = fopen(path_to_write, "a");
        if (fptr == NULL)
        {
            fprintf(stderr, RED("fopen : could not open file to append : %s\n"), strerror(errno));
            // Keep sending acknowledgement till not sent successfully only if it is not the backup request (since backup is asynchronous)
            if (recvd_request.request_type == APPEND_REQ)
            {
                send_ack(APPEND_FAILED, sock_fd, strerror(errno));
            }
            else
            {
                free(path_to_write);
            }
            goto End;
        }
        fprintf(fptr, "%s", data_to_write);
        fclose(fptr);

        if (recvd_request.request_type == BACKUP_APPEND_REQ)
        {
            free(path_to_write);
            // No need to send ack as backup is done asynchronously
        }
        else
        {
            FILE* fptr2 = fopen(path_to_write, "r");
            char buffer[MAX_DATA_LENGTH - MAX_PATH_LEN - 1] = {0};
            fread(buffer, sizeof(char), MAX_DATA_LENGTH - MAX_PATH_LEN - 2, fptr2);
            fclose(fptr2);

            char final_str[MAX_DATA_LENGTH] = {0};
            strcpy(final_str, path_to_write);
            strcat(final_str, "|");
            strcat(final_str, buffer);

            send_msg_to_nfs(final_str, CONSISTENT_WRITE);

            send_ack(ACK, sock_fd, NULL);
        }
    }
    else if (recvd_request.request_type == RETRIEVE_INFO)
    {
        printf(YELLOW("Retrieve Information request received.\n"));
        char* path = recvd_request.data;

        // Path of the file whose information has to be retrieved
        struct stat file_stat;

        st_request send_info;
        send_info.request_type = INFO;
        memset(send_info.data, 0, MAX_DATA_LENGTH);

        strcpy(send_info.data, PINK_COLOR);
        strcat(send_info.data, "Permission : ");

        // Retrieving file permissions
        if (!stat(path, &file_stat))
        {
            strcat(send_info.data, (S_ISDIR(file_stat.st_mode)) ? "d" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IRUSR) ? "r" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IWUSR) ? "w" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IXUSR) ? "x" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IRGRP) ? "r" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IWGRP) ? "w" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IXGRP) ? "x" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IROTH) ? "r" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IWOTH) ? "w" : "-");
            strcat(send_info.data, (file_stat.st_mode & S_IXOTH) ? "x" : "-");
        }
        else
        {
            fprintf(stderr, RED("info : could not get the stats for info of the file : %s\n"), strerror(errno));
            send_ack(INFO_RETRIEVAL_FAILED, sock_fd, strerror(errno));
            goto End;
        }
        strcat(send_info.data, " ");
        char size_str[10] = {0};

        // Storing file size with a space
        strcat(send_info.data, " Size : ");
        #if defined(LINUX)
            sprintf(size_str, "%ld", file_stat.st_blocks);
        #else
            sprintf(size_str, "%lld", file_stat.st_blocks);
        #endif
        strcat(send_info.data, size_str);
        strcat(send_info.data, " ");

        strcat(send_info.data, "Last Modified : ");

        // Storing last modification time
        time_t modificationTime = file_stat.st_mtime;

        // Convert time to a formatted string and print it
        char *time_string;
        time_string = ctime(&modificationTime);
        char month[4];
        char date[3];
        char hour[3];
        char mins[3];
        char year[5];
        for (int i = 4; i < 7; i++)
        {
            month[i - 4] = time_string[i];
        }
        month[3] = '\0';

        for (int i = 8; i < 10; i++)
        {
            date[i - 8] = time_string[i];
        }
        date[2] = '\0';

        for (int i = 11; i < 13; i++)
        {
            hour[i - 11] = time_string[i];
        }
        hour[2] = '\0';

        for (int i = 14; i < 16; i++)
        {
            mins[i - 14] = time_string[i];
        }
        mins[2] = '\0';

        for (int i = 20; i < 24; i++)
        {
            year[i - 20] = time_string[i];
        }
        year[4] = '\0';

        char timestamp[100] = {0};
        sprintf(timestamp, "Date : %s %s  Time : %s:%s ", month, date, hour, mins);
        strcat(send_info.data, timestamp);
        strcat(send_info.data, RESET_COLOR);

        int sent_msg_size;
        if ((sent_msg_size = send(sock_fd, (request)&send_info, sizeof(st_request), 0)) <= 0)
        {
            fprintf(stderr, RED("send : could not send meta info : %s\n"), strerror(errno));
            send_ack(INFO_RETRIEVAL_FAILED, sock_fd, strerror(errno));
        }
    }
    else if (recvd_request.request_type == COPY_FILE)
    {
        printf(YELLOW("Copy request received.\n"));
        char* path = recvd_request.data;

        st_request copy_data_request;
        copy_data_request.request_type = DATA_TO_BE_COPIED;
        memset(copy_data_request.data, 0, MAX_DATA_LENGTH);

        FILE *fptr = fopen(path, "r");
        if (fptr == NULL)
        {
            fprintf(stderr, RED("fopen : could not open file to read data to be copied : %s\n"), strerror(errno));
            send_ack(FILE_NOT_FOUND, sock_fd, NULL);
            goto End;
        }
        char buffer[MAX_DATA_LENGTH - 1026] = {0};
        int bytes_read = fread(buffer, sizeof(char), MAX_DATA_LENGTH - 1027, fptr);

        // <path>|<data in file>
        strcpy(copy_data_request.data, path);
        strcat(copy_data_request.data, "|");
        strcat(copy_data_request.data, buffer);

        int sent_msg_size;
        if ((sent_msg_size = send(sock_fd, (request)&copy_data_request, sizeof(st_request), 0)) <= 0)
        {
            fprintf(stderr, RED("send : could not send the data to be copied : %s\n"), strerror(errno));
            send_ack(COPY_FAILED, sock_fd, NULL);
            goto End;
        }

        send_ack(ACK, sock_fd, NULL);
    }
    else if (recvd_request.request_type == COPY_FOLDER)
    {
        // Relative path to the folder to be copied
        char *path = recvd_request.data;
        
        // Absolute path
        char abs_path[MAX_PATH_LEN] = {0};

        // Creating the absolute path to the folder
        strcpy(abs_path, PWD);
        strcat(abs_path, path + 1);
        
        char** files_folders = get_all_files_folders(abs_path);

        int n_files_folders = 0;
        while (files_folders[n_files_folders] != NULL)
        {
            n_files_folders++;
        }
        char* src_folder_name = get_folder_name(path);

        st_copy_folder paths_sending_req;
        paths_sending_req.request_type = FOLDER_DATA_TO_BE_COPIED;
        paths_sending_req.num_paths = n_files_folders;

        for (int i = 0; i < n_files_folders; i++)
        {
            char* curr_path = files_folders[i];
            char* updated_path = update_path_rel(abs_path, curr_path);

            char final_str_to_send[MAX_DATA_LENGTH] = {0};
            strcat(final_str_to_send, "/");
            strcat(final_str_to_send, src_folder_name);
            strcat(final_str_to_send, "/");
            strcat(final_str_to_send, curr_path + 2);

            if (is_file(updated_path))
            {
                // Path is of a file
                strcat(final_str_to_send, "|");

                FILE* fptr = fopen(updated_path, "r");
                char buffer[MAX_DATA_LENGTH - MAX_PATH_LEN - MAX_PATH_LEN - 3] = {0};
                fread(buffer, sizeof(char), MAX_DATA_LENGTH - MAX_PATH_LEN - MAX_PATH_LEN - 4, fptr);
                fclose(fptr);
                strcat(final_str_to_send, buffer);
            }
            else
            {
                // Path is of a folder (don't need to attach content at end)
            }
            memset(paths_sending_req.paths[i], 0, MAX_PATH_LEN);
            strcpy(paths_sending_req.paths[i], final_str_to_send);

            free(updated_path);
        }

        int sent_msg_size;
        if ((sent_msg_size = send(sock_fd, (st_copy_folder*) &paths_sending_req, sizeof(st_copy_folder), 0)) <= 0)
        {
            fprintf(stderr, RED("send : could not sent acknowledgement for Request type %d : %s\n"), FOLDER_DATA_TO_BE_COPIED, strerror(errno));
        }

        // Freeing memory
        free(src_folder_name);
        for (int i = 0; i < n_files_folders; i++)
        {
            free(files_folders[i]);
        }
        free(files_folders);
    }
    else if (recvd_request.request_type == PASTE || recvd_request.request_type == BACKUP_PASTE)
    {
        char* file_path;
        if (recvd_request.request_type == PASTE)
        {
            printf(YELLOW("Paste request received.\n"));
            printf("Pasting file : %s\n", recvd_request.data);

            file_path = request_tkns[0];
        }
        else
        {
            printf(YELLOW("Backup Paste request received.\n"));
            printf("Pasting file : %s\n", recvd_request.data);

            file_path = replace_storage_by_backup(request_tkns[0]);
        }

        char *file_content = request_tkns[1];

        // Creating intermediate directories if not already present
        // First tokenising the file_path on "/"

        char **dirs = tokenize(file_path, '/');

        // Calculating the number of intermediate dirs
        int n_tkns = 0;
        while (dirs[n_tkns] != NULL)
        {
            n_tkns++;
        }

        int n_dirs;
        int file_type;

        int result = is_file(dirs[n_tkns - 1]);
        if (result == 1)
        {
            // This is a paste file request
            // Final number of dirs is 1 less than the number of tokens as the last one is the file
            n_dirs = n_tkns - 1;
            file_type = FILE_T;
        }
        else
        {
            // This is a paste folder request
            n_dirs = n_tkns;
            file_type = FOLDER_T;
        }

        // Now creating all the intermediate dirs one by one
        for (int i = 0; i < n_dirs; i++)
        {
            if (mkdir(dirs[i], 0777) == 0)
            {
                // If the directory did not exist already then it got created
            }
            else if (errno == EEXIST)
            {
                // If the directory already exists then do nothing and just move into it
            }
            else
            {
                // Error creating the directory
                fprintf(stderr, RED("mkdir : could not create directory : %s\n"), strerror(errno));
                chdir(PWD);
                if (recvd_request.request_type == BACKUP_PASTE)
                {
                    free(file_path);printf(YELLOW("Ping request received.\n"));
                }
                else
                {
                    send_ack(PASTE_FAILED, sock_fd, strerror(errno));
                }
                goto End;
            }
            // Moving into that directory to create the next directory in hierarchy
            chdir(dirs[i]);
        }

        // Moving out back to the pwd
        chdir(PWD);

        if (file_type == FILE_T)
        {
            // Now opening the file in write mode, if it does not exist it would be created otherwise the old data would be overwritten
            FILE *fptr = fopen(file_path, "w");
            if (fptr == NULL)
            {
                fprintf(stderr, RED("fopen : could not open file for pasting : %s\n"), strerror(errno));
                send_ack(PASTE_FAILED, sock_fd, strerror(errno));
                goto End;
            }

            fprintf(fptr, "%s", file_content);

            fclose(fptr);
        }

        if (recvd_request.request_type == BACKUP_PASTE)
        {
            free(file_path);
            // No need to send ack as backup is done asynchronously
        }
        else
        {
            send_ack(ACK, sock_fd, NULL);
        }
    }
    else if (recvd_request.request_type == PING)
    {
        // Received a ping request from NFS to check if my SS is still responding or not so sending back the PING to say that I am active and listening
        st_request ping_request;
        send_ack(ACK, sock_fd, NULL);
    }
    else if (recvd_request.request_type == DELETE_FILE)
    {
        printf(YELLOW("Delete file request received.\n"));
        char* file_path = recvd_request.data;

        // First checking if filepath exists or not
        if (access(file_path, F_OK) != -1)
        {
            // File path exists
            if (remove(file_path) != 0)
            {
                // If there was some error in deleting the file
                fprintf(stderr, RED("remove : could not delete file : %s\n"), strerror(errno));
                send_ack(DELETE_FAILED, sock_fd, strerror(errno));
                goto End;
            }
        }
        else
        {
            // File path does not exist
            fprintf(stderr, RED("remove : file does not exist\n"));
            send_ack(DELETE_FAILED, sock_fd, strerror(errno));
            goto End;
        }

        // Keep sending acknowledgement till not sent successfully
        send_ack(ACK, sock_fd, NULL);
    }
    else if (recvd_request.request_type == BACKUP_DELETE_FILE)
    {
        printf(YELLOW("Backup Delete file request received.\n"));
        char* file_path = replace_storage_by_backup(recvd_request.data);

        // First checking if filepath exists or not
        if (access(file_path, F_OK) != -1)
        {
            // File path exists
            if (remove(file_path) != 0)
            {
                // If there was some error in deleting the file
                fprintf(stderr, RED("remove : could not delete backup file : %s\n"), strerror(errno));
                free(file_path);
                goto End;
            }
        }
        else
        {
            // File path does not exist
            // Do nothing and continue;
        }

        // No need to send ack as backup is done asynchronously
        free(file_path);
    }
    else if (recvd_request.request_type == DELETE_FOLDER)
    {
        printf(YELLOW("Delete folder request received.\n"));
        char* dir_path = recvd_request.data;

        char* absolute_path = create_abs_path(dir_path);
        if (absolute_path == NULL)
        {
            fprintf(stderr, RED("rmdir : Can't remove the specified folder.\n"));
            send_ack(INVALID_DELETION, sock_fd, strerror(errno));
            goto End;
        }

        int pid = fork();
        if (pid == 0)
        {
            // Child process
            char* command = "rm";
            char* args[] = {"rm", "-r", absolute_path, NULL};

            execvp(command, args);
            // execvp failed
            fprintf(stderr, RED("execvp : failed to remove directories\n"));
            send_ack(DELETE_FAILED, sock_fd, strerror(errno));
            goto End;
        }
        else if (pid > 0)
        {
            // Parent process (waiting for child to finish)
            wait(NULL);
        }
        else
        {
            // Fork failed so display error and send failed ack
            fprintf(stderr, RED("fork: could not fork for folder deletion\n"));
            send_ack(DELETE_FAILED, sock_fd, strerror(errno));
            goto End;
        }

        send_ack(ACK, sock_fd, NULL);
    }
    else if (recvd_request.request_type == BACKUP_DELETE_FOLDER)
    {
        printf(YELLOW("Backup Delete folder request received.\n"));
        char* dir_path = replace_storage_by_backup(recvd_request.data);

        char* absolute_path = create_abs_path(dir_path);
        if (absolute_path == NULL)
        {
            fprintf(stderr, RED("rmdir : Can't remove the specified folder : %s\n"), strerror(errno));
            free(dir_path);
            goto End;
        }

        int pid = fork();
        if (pid == 0)
        {
            // Child process
            char* command = "rm";
            char* args[] = {"rm", "-r", absolute_path, NULL};

            execvp(command, args);
            // execvp failed
            fprintf(stderr, RED("execvp : failed to remove directories : %s\n"), strerror(errno));
            free(dir_path);
            goto End;
        }
        else if (pid > 0)
        {
            // Parent process
            wait(NULL);
        }
        else
        {
            // Fork failed so display error and send failed ack
            fprintf(stderr, RED("fork: could not fork for folder deletion : %s\n"), strerror(errno));
            free(dir_path);
            goto End;
        }

        // No need to send ack as backup is done asynchronously
        free(dir_path);
    }
    else if (recvd_request.request_type == CREATE_FILE)
    {
        printf(YELLOW("Create file request received.\n"));
        char* file_path = recvd_request.data;

        FILE *fptr = fopen(file_path, "w");
        if (fptr == NULL)
        {
            fprintf(stderr, RED("fopen : path DNE : %s\n"), strerror(errno));
            send_ack(CREATE_FAILED, sock_fd, strerror(errno));
            goto End;
        }
        fclose(fptr);

        send_ack(ACK, sock_fd, NULL);
    }
    else if (recvd_request.request_type == BACKUP_CREATE_FILE)
    {
        printf(YELLOW("Backup Create file request received.\n"));
        char* file_path = replace_storage_by_backup(recvd_request.data);

        // First calculating the path to the folder where the file is located it may so happen that the folder may not exist right now but we need to backup so first finding the folder path so that I can create the folder first before creating the file
        char folder_path[MAX_PATH_LEN] = {0};

        char **dirs = tokenize(file_path, '/');

        // Calculating the number of intermediate dirs
        int n_tkns = 0;
        while (dirs[n_tkns] != NULL)
        {
            n_tkns++;
        }
        // Final number of dirs is 1 less than the number of tokens as the last one is the file
        int n_dirs = n_tkns - 1;

        for (int i = 0; i < n_dirs; i++)
        {
            strcat(folder_path, dirs[i]);
            strcat(folder_path, "/");
        }
        free_tokens(dirs);
        folder_path[strlen(folder_path) - 1] = '\0';

        create_folder(folder_path);

        FILE *fptr = fopen(file_path, "w");
        if (fptr == NULL)
        {
            fprintf(stderr, RED("fopen : could not open file to create backup file : %s\n"), strerror(errno));
            free(file_path);
            goto End;
        }
        fclose(fptr);

        free(file_path);
    }
    else if (recvd_request.request_type == CREATE_FOLDER)
    {
        printf(YELLOW("Create folder request received.\n"));
        char* file_path = request_tkns[0];

        create_folder(file_path);

        send_ack(ACK, sock_fd, NULL);
    }
    else if (recvd_request.request_type == BACKUP_CREATE_FOLDER)
    {
        printf(YELLOW("Backup Create folder request received.\n"));
        char* file_path = replace_storage_by_backup(request_tkns[0]);

        create_folder(file_path);

        // No need to send ack as backup is done asynchronously

        free(file_path);
    }

End:
    // Freeing tokens created at the start from request data
    free_tokens(request_tkns);

End2:
    // Closing client socket as all the communication is done
    if (close(sock_fd) < 0)
    {
        fprintf(stderr, RED("close : failed to close the client socket!\n"));
        exit(EXIT_FAILURE);
    }

    // Freeing thread slot
    pthread_mutex_lock(&threads_arr_mutex);
    thread_slot_empty_arr[thread_index] = 0;
    pthread_mutex_unlock(&threads_arr_mutex);

    return NULL;
}

// This function starts and binds to the port which listens for communication with NFS
void *start_nfs_port(void *args)
{
    memset(&ss_address_nfs, 0, sizeof(ss_address_nfs));

    // This socket id is never used for sending/receiving data, used by server just to get new sockets (clients)
    nfs_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (nfs_server_socket_fd < 0)
    {
        // Some error occured while creating socket
        fprintf(stderr, RED("socket : could not start nfs socket : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    ss_address_nfs.sin_family = AF_INET;
    ss_address_nfs.sin_port = htons(MY_NFS_PORT_NO); // My port on which I am listening to NFS communication
    ss_address_nfs.sin_addr.s_addr = htonl(INADDR_ANY);

    // Binding to the port
    if (bind(nfs_server_socket_fd, (struct sockaddr *)&ss_address_nfs, sizeof(ss_address_nfs)) == -1)
    {
        fprintf(stderr, RED("bind : could not bind nfs socket : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf(GREEN("Started listening to NFS requests.\n"));

    // Listening for incoming requests for communication
    if (listen(nfs_server_socket_fd, MAX_PENDING) == -1)
    {
        fprintf(stderr, RED("listen : could not listen on nfs socket : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Keep accepting incoming requests
        // Accepting NFS requests
        struct sockaddr_in nfs_addr;
        int addr_size = sizeof(struct sockaddr_in);
        int nfs_sock_fd = accept(nfs_server_socket_fd, (struct sockaddr *)&nfs_addr, (socklen_t *)&addr_size);
        if (nfs_sock_fd == -1)
        {
            fprintf(stderr, RED("accept : could not accept connection on nfs socket : %s\n"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&threads_arr_mutex);
        for (int i = 0; i < MAX_PENDING; i++)
        {
            if (thread_slot_empty_arr[i] == 0)
            {
                thread_slot_empty_arr[i] = 1;
                thread_data args = (thread_data)malloc(sizeof(st_thread_data));
                args->client_sock_fd = nfs_sock_fd;
                args->thread_idx = i;

                // Create a new thread on the same position
                pthread_create(&requests_serving_threads_arr[i], NULL, &serve_request, args);
                break;
            }
        }
        pthread_mutex_unlock(&threads_arr_mutex);
    }

    if (close(nfs_server_socket_fd) < 0)
    {
        fprintf(stderr, RED("close : failed to close the nfs socket!\n"));
        exit(EXIT_FAILURE);
    }

    return NULL;
}

// This function binds to the port listening for client requests and starts listening for requests made by clients (SS acts as TCP server)
void *start_client_port(void *args)
{
    memset(&ss_address_client, 0, sizeof(ss_address_client));

    // This socket id is never used for sending/receiving data, used by server just to get new sockets (clients)
    client_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_server_socket_fd < 0)
    {
        // Some error occured while creating socket
        fprintf(stderr, RED("socket : could not create client socket : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    ss_address_client.sin_family = AF_INET;
    ss_address_client.sin_port = htons(MY_CLIENT_PORT_NO); // My port on which I am listening to Client communication
    ss_address_client.sin_addr.s_addr = htonl(INADDR_ANY);

    // Binding to the port
    if (bind(client_server_socket_fd, (struct sockaddr *)&ss_address_client, sizeof(ss_address_client)) == -1)
    {
        // Error while binding to the port
        fprintf(stderr, RED("bind : could not bind to client socket : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf(GREEN("Started listening to Client requests.\n"));

    // Listening for incoming requests for communication
    if (listen(client_server_socket_fd, MAX_PENDING) == -1)
    {
        fprintf(stderr, RED("listen : could not listen on client socket : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        // Keep accepting incoming requests
        // Accepting client requests
        struct sockaddr_in client_addr;
        int addr_size = sizeof(struct sockaddr_in);

        // This socket is used for communication with the particular client
        int client_socket_fd = accept(client_server_socket_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addr_size);
        if (client_socket_fd == -1)
        {
            fprintf(stderr, RED("accept : could not accept requests on client socket : %s\n"), strerror(errno));
            exit(EXIT_FAILURE);
        }

        // If the threads array is full then the request won't be served and just ignored => No ack will be sent for that request so the NFS should request the same again if ack is not received in some time
        pthread_mutex_lock(&threads_arr_mutex);
        for (int i = 0; i < MAX_PENDING; i++)
        {
            if (thread_slot_empty_arr[i] == 0)
            {
                // This thread slot will now get busy so setting it's value to 1 (busy)
                thread_slot_empty_arr[i] = 1;
                // Storing the data that is to be passed to the thread in a struct
                thread_data args = (thread_data)malloc(sizeof(st_thread_data));
                args->client_sock_fd = client_socket_fd;
                args->thread_idx = i;

                // If this thread is still running then first wait for it to complete, it is about to complete as it has already made it's slot empty to 0
                pthread_join(requests_serving_threads_arr[i], NULL);

                // Create a new thread on the same position
                pthread_create(&requests_serving_threads_arr[i], NULL, &serve_request, args);
                break;
            }
        }
        pthread_mutex_unlock(&threads_arr_mutex);
    }

    // Close the server
    if (close(client_server_socket_fd) < 0)
    {
        // Error while closing the socket
        fprintf(stderr, RED("close : failed to close the client socket!\n"));
        exit(EXIT_FAILURE);
    }

    return NULL;
}
