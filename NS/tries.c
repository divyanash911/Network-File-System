#include "headers.h"

// Creates a new trie node
struct trie_node *create_trie_node()
{
    // Allocating memory to the node
    struct trie_node *node = (struct trie_node *) malloc(sizeof(struct trie_node));
    if (node == NULL)
    {
        fprintf(stderr, RED("malloc : could not allocate memory to trie node : %s\n"), strerror(errno));
        return NULL;
    }
    node->key = NULL;   // Initially there is no string stored in the node (strings are stored only in end nodes)
    node->end = 0;      // Initially the node is not an end node
    node->ssid = -1;    // Initially no ssid is assigned to node
    // 256 childrens 1 for each of the ASCII characters
    for (int i = 0; i < 256; i++)
    {
        node->children[i] = NULL;
    }
    return node;
}

// A function to insert a string into the trie, Returns 1 if successfull else 0
int insert_path(struct trie_node *root, char *key,int ssid)
{
    // Starting from the root node
    struct trie_node *current = root;
    for (int i = 0; key[i] != '\0'; i++)
    {
        // Index is just the ASCII value of that character
        int index = (int)key[i];

        // If that node does not exist already then just create one
        if (current->children[index] == NULL)
        {
            current->children[index] = create_trie_node();
            if (current->children[index] == NULL)
            {
                fprintf(stderr, RED("create_trie_node : could not create node.\n"));
                return 0;
            }
        }
        // Move to that node
        current = current->children[index];
    }
    // We have reached the end of the string that is to be stored so we are at the final node so just store the string as the key and mark this node as one of the end nodes
    current->key = key;
    current->end = 1;
    current->ssid = ssid;
    return 1;
}

// A function to search for a string in the trie, Returns 1 if path is found else 0
int search_path(struct trie_node *root, char *key)
{
    // Starting from the root node
    struct trie_node *current = root;
    for (int i = 0; key[i] != '\0'; i++)
    {
        int index = (int)key[i];

        if (current->children[index] == NULL)
        {
            // If the character we are looking for is not present as the child then the string is not present in the trie
            return -1;
        }
        current = current->children[index];
    }

    if (current->end == 1)
    {
        return current->ssid;
    }
    return -1;
}

// A function to delete a string from the trie, Returns 1 if successfull else 0
int delete_path(struct trie_node *root, char *key)
{
    // Lazy deletion
    struct trie_node *current = root;
    for (int i = 0; key[i] != '\0'; i++)
    {
        int index = (int) key[i];

        if (current->children[index] == NULL)
        {
            return 0;
        }
        current = current->children[index];
    }
    
    current->key = NULL;
    current->end = 0;
    current->ssid = -1;
    return 1;
}

// Prints all the paths present in the trie
void print_paths(struct trie_node *root)
{
    if (root == NULL)
    {
        return;
    }

    if (root->end == 1)
    {
        printf("%s %d\n", root->key,root->ssid);
    }

    for (int i = 0; i < 256; i++)
    {
        if (root->children[i] != NULL)
        {
            print_paths(root->children[i]);
        }
    }
}

// Returns all the paths in the form of a linked list
linked_list_head return_paths(struct trie_node *root)
{
    linked_list_head ll = create_linked_list_head();
    add_paths(ll,root);
    return ll;
}

// Adds paths one by one recursively to the linked list
void add_paths(linked_list_head ll, struct trie_node *root)
{
    if (root == NULL)
    {
        return;
    }

    if (root->end == 1)
    {
        insert_in_linked_list(ll, root->key);
    }

    for (int i = 0; i < 256; i++)
    {
        if (root->children[i] != NULL)
        {
            add_paths(ll, root->children[i]);
        }
    }
}

// Linked list functions
linked_list_head create_linked_list_head() {
    linked_list_head linked_list = (linked_list_head) malloc(sizeof(linked_list_head_struct));
    linked_list->number_of_nodes = 0;
    linked_list->first = NULL;
    linked_list->last = NULL;
    return linked_list;
}

linked_list_node create_linked_list_node(char* path) {
    linked_list_node N = (linked_list_node) malloc(sizeof(linked_list_node_struct));
    N->next = NULL;
    N->path = (char*) calloc(MAX_PATH_LEN, sizeof(char));
    strcpy(N->path, path);
    return N;
}

void insert_in_linked_list(linked_list_head linked_list, char* path) {
    linked_list_node N = create_linked_list_node(path);
    if (linked_list->number_of_nodes == 0) {
        linked_list->first = N;
        linked_list->last = N;
        linked_list->number_of_nodes++;
    } else if (linked_list->number_of_nodes == 1) {
        linked_list->last = N;
        linked_list->first->next = N;
        linked_list->number_of_nodes++;
    } else {
        linked_list->last->next = N;
        linked_list->last = N;
        linked_list->number_of_nodes++;
    }
}

void free_linked_list(linked_list_head linked_list) {
    linked_list_node trav = linked_list->first;
    while (trav != NULL) {
        free(trav->path);
        linked_list_node temp = trav->next;
        free(trav);
        trav = temp;
    }
    free(linked_list);
}

