#include "headers.h"

void communicate_with_ss_info(char *ipaddress, char *port, char *path)
{
    int client_socket = connect_with_ss(ipaddress, port);
    st_request *readerpacket = malloc(sizeof(st_request));
    readerpacket->request_type = RETRIEVE_INFO;
    strcpy(readerpacket->data, path);
    ssize_t bytes_sent = send(client_socket, readerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }

    // Reader Packet sent
    request req = (request)malloc(sizeof(st_request));
    if (recv(client_socket, req, sizeof(st_request), 0) == -1)
    {
        perror("Receiving data failed");
        // continue;
    }
    if(req->request_type==INFO)
    {
        printf("%s\n", req->data);
    }
    else
    {
        printf(RED("Failed to retrieve Data : %s\n"),req->data);
    }
    free(req);
    close(client_socket);
}

void info(char *path)
{
    int client_socket = connect_with_ns();
    st_request *readerpacket = malloc(sizeof(st_request));
    readerpacket->request_type = RETRIEVE_INFO;
    strcpy(readerpacket->data, path);
    ssize_t bytes_sent = send(client_socket, readerpacket, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_request *response = (st_request *) malloc(sizeof(st_request));
    ssize_t bytes_received = recv(client_socket, response, sizeof(st_request), 0);
    char *port;
    char *ipaddress;
    if (bytes_received == -1)
    {
        perror("Receive failed");
    }
    else
    {
        if (response->request_type == FILE_NOT_FOUND)
        {
            printf(RED("File/Directory Not Found \n")); // Error Not Found File
            return;
        }
        else if(response->request_type == TIMEOUT)
        {
            printf(RED("File currently unavailable please try again\n"));
            close(client_socket);
            return;
        }
        else if(response->request_type==INVALID_FILETYPE)
        {
            printf(RED("Invalid file type\n"));
            close(client_socket);
            return;
        }
        else
        {
            printf("%s\n",response->data);
        }
        ipaddress = strtok(response->data, "|");
        port = strtok(NULL, "|");
    }
    close(client_socket);
    communicate_with_ss_info(ipaddress, port, path);
}

