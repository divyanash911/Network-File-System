#include "headers.h"

// Signal handler function
void handleCtrlZ(int signum)
{
    printf("Ctrl+Z signal received (SIGTSTP)\n");
    // Print all the logs

    FILE* fptr = fopen("logs.txt", "r");
    if (fptr == NULL)
    {
        fprintf(stderr, RED("fopen : %s\n"), strerror(errno));
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE] = {0};

    // Read and print the file in chunks of BUFFER_SIZE bytes
    printf(YELLOW_COLOR);
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, BUFFER_SIZE, fptr)) > 0) {
        // Process the buffer, e.g., print its content
        fwrite(buffer, 1, bytesRead, stdout);
    }
    printf(RESET_COLOR);

    fclose(fptr);
}

// First argument should be whether the communication was with storage server or client (SS or CLIENT)
// Second argument is storage server id with which the communication is happening, if communication is with a client and not a storage server then by default pass value 0 there
// Third argument should be the port number of client or storage server that is involved in the communication whose log is being inserted
// Fourth argument is the request type
// Fifth argument is the request data
// Sixth argumnet is the status code of the request processed (Yet to implement like error codes like path found or path not found or successfully completed or not successfully completed etc)
// This function returns 1 if log is successfully logged or else 0
int insert_log(const int type, const int ss_id, const int ss_or_client_port, const int request_type, const char* request_data, const int status_code)
{
    FILE* fptr = fopen("logs.txt" , "a");
    if (fptr == NULL)
    {
        fprintf(stderr, RED("fopen : %s\n"), strerror(errno));
        return 0;
    }

    if (type == SS)
    {
        fprintf(fptr, "Communicating with Storage Server : %d\n", ss_id);
        fprintf(fptr, "NFS Port number                   : %d\n", NS_PORT);
        fprintf(fptr, "Storage Server Port number        : %d\n", ss_or_client_port);
        fprintf(fptr, "Request type                      : %d\n", request_type);
        fprintf(fptr, "Request data                      : %s\n", request_data);
        fprintf(fptr, "Status                            : %d\n", status_code);
        fprintf(fptr, "\n");
    }
    else
    {
        fprintf(fptr, "Communicating with Client\n");
        fprintf(fptr, "NFS Port number                   : %d\n", NS_PORT);
        fprintf(fptr, "Client Port number                : %d\n", ss_or_client_port);
        fprintf(fptr, "Request type                      : %d\n", request_type);
        fprintf(fptr, "Request data                      : %s\n", request_data);
        fprintf(fptr, "Status                            : %d\n", status_code);
        fprintf(fptr, "\n");
    }

    fclose(fptr);

    return 1;
}

