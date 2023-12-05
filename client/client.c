#include "headers.h"

int main()
{
    printf(BLUE("For input related doubts type MAN\n"));
    char input[3000];
    while (1)
    {
        printf(YELLOW("\n------- Enter operation you want to do -------\n"));
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';
        char *operation = strtok(input, " ");
        if (strcmp("READ", operation) == 0)
        {
            char *path = strtok(NULL, " ");
            reading_operation(path); // Reading Function Gets Called
        }
        else if (strcmp("WRITE", operation) == 0)
        {
            char *path = strtok(NULL, " ");
            writing_append_operation(path, 1); // Writing Function Gets Called
        }
        else if (strcmp("APPEND", operation) == 0)
        {
            char *path = strtok(NULL, " ");
            writing_append_operation(path, 0); // Append Function Gets Called
        }
        else if (strcmp("CREATE", operation) == 0)
        {
            char *type = strtok(NULL, " ");
            char *path = strtok(NULL, " ");
            char *name = strtok(NULL, " ");
            if (strcmp(type, "FOLDER") == 0)
            {
                create_operation(path, name, CREATE_FOLDER);
            }
            else if (strcmp(type, "FILE") == 0)
            {
                create_operation(path, name, CREATE_FILE);
            }
        }
        else if (strcmp("DELETE", operation) == 0)
        {
            char *type = strtok(NULL, " ");
            char *path = strtok(NULL, " ");
            if (strcmp(type, "FOLDER") == 0)
            {
                delete_operation(path, DELETE_FOLDER);
            }
            else if (strcmp(type, "FILE") == 0)
            {
                delete_operation(path, DELETE_FILE);
            }
        }
        else if (strcmp("COPY", operation) == 0)
        {
            char *type = strtok(NULL, " ");
            char *path1 = strtok(NULL, " ");
            char *path2 = strtok(NULL, " ");
            printf("%s %s\n", path1, path2);

            if (strcmp(type, "FOLDER") == 0)
            {
                copy_operation(COPY_FOLDER, path1, path2);
            }
            else
                copy_operation(COPY_FILE, path1, path2);
        }
        else if (strcmp("INFO", operation) == 0)
        {
            char *path = strtok(NULL, " ");
            info(path);
        }
        else if (strcmp("LIST", operation) == 0)
        {
            list();
        }
        else if (strcmp("MAN", operation) == 0)
        {
            man();
        }
        else if (strcmp("EXIT", operation) == 0)
        {
            break;
        }
        else
        {
            printf(RED("Invalid Operation do again\n"));
        }
    }

    return 0;
}

