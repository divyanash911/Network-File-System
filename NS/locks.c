#include "headers.h"

paths_lock *global_paths_locked;

void initializer_header_node()
{
    global_paths_locked = malloc(sizeof(paths_lock));
    global_paths_locked->next = NULL;
}

int path_locked_or_not(char *path)
{
    sem_wait(&lock);
    paths_lock *current = global_paths_locked->next;

    while (current != NULL)
    {
        if (strcmp(current->path, path) == 0)
        {
            sem_post(&lock);
            return 0;
        }
        current = current->next;
    }
    sem_post(&lock);
    return 1;
}

void insert_path_lock(const char *new_path)
{
    sem_wait(&lock);
    paths_lock *new_node = malloc(sizeof(paths_lock));
    strncpy(new_node->path, new_path, sizeof(new_node->path) - 1);
    new_node->path[sizeof(new_node->path) - 1] = '\0';
    new_node->next = global_paths_locked->next;
    global_paths_locked->next = new_node;
    sem_post(&lock);
}

void delete_path_lock(const char *path_to_delete)
{
    sem_wait(&lock);
    paths_lock *current = global_paths_locked;
    paths_lock *previous = NULL;

    while (current != NULL && strcmp(current->path, path_to_delete) != 0)
    {
        previous = current;
        current = current->next;
    }

    if (current != NULL)
    {
        // Unlink the node to be deleted
        if (previous != NULL)
        {
            previous->next = current->next;
        }
        else
        {
            global_paths_locked->next = current->next;
        }

        // Free the memory of the deleted node
        free(current);
    }
    sem_post(&lock);
}

