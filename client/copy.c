#include "headers.h"

void copy_operation(int req_type,char *path1, char *path2)
{
    if(strstr(path2,".txt")!=NULL)
    {
        printf(RED("File cannot be a destination path \n"));
        return ;
    }
    int client_socket = connect_with_ns();
    st_request *packet = malloc(sizeof(st_request));
    packet->request_type = req_type;
    snprintf(packet->data, sizeof(packet->data), "%s|%s", path1, path2);
    ssize_t bytes_sent = send(client_socket, packet, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_request *response = (st_request *)malloc(sizeof(st_request));
    ssize_t bytes_received = recv(client_socket, response, sizeof(st_request), 0);
    if (response->request_type== ACK)            //Check if condition(ACK AND STUFF)
    {
        printf(GREEN("Copy of Directory or File succesfull \n"));
    }
    else
    {
        printf(RED("Copy of Directory or File not succesfull : %s \n"),response->data); // Error Not succesfull
    }
    close(client_socket);
}