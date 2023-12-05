#include "headers.h"

int connect_with_ss(char *ipaddress, char *port)
{
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(port));
    server_address.sin_addr.s_addr = inet_addr(ipaddress);

    while(1){
    int x=connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    if(x == -1)
    {
        perror("Connection failed");
        return 0;
    }
    else if(x==0) break;
    
    }
    return client_socket;
}

int connect_with_ns()
{
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(NS_PORT);
    server_address.sin_addr.s_addr = inet_addr(NS_IP);
    while (1)
    {
        if (connect(client_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
        {
            perror("Connection failed");
            continue;
        }
        else
        {
            break;
        }
    }
    return client_socket;
}