#include "headers.h"

void list()
{
    int client_socket = connect_with_ns();
    st_request *packet = malloc(sizeof(st_request));
    packet->request_type = LIST;
    strcpy(packet->data,"dummy");
    ssize_t bytes_sent = send(client_socket, packet, sizeof(st_request), 0);
    if (bytes_sent == -1)
    {
        perror("Send failed");
    }
    st_request *response = (st_request *)malloc(sizeof(st_request));
    ssize_t bytes_received = recv(client_socket, response, sizeof(st_request), 0);
    if(response->request_type==RES)
    {
        char *token = strtok(response->data, "|");
        while (token != NULL) {
            printf("%s\n", token);
            token = strtok(NULL, "|");
        }
    }
    else
    {
        printf(RED("List Request Not successful \n"));
    }

}