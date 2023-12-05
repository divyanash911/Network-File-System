#include "headers.h"

void create_operation(char *path, char *name,int macro)
{
    int client_socket = connect_with_ns();
    st_request *packet = malloc(sizeof(st_request));
    packet->request_type = macro;
    snprintf(packet->data, sizeof(packet->data), "%s|%s", path, name);
    ssize_t bytes_sent = send(client_socket, packet, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_request *response = (st_request *)malloc(sizeof(st_request));
    ssize_t bytes_received = recv(client_socket, response, sizeof(st_request), 0);
    if (response->request_type==ACK)
    {
        printf(GREEN("Creation of Directory or File succesfull \n"));
    }
    else
    {   
        printf(RED("Creation of Directory or File not succesfull : %s \n"),response->data); // Error Not succesfull
    }
    close(client_socket);
}