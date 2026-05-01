#ifndef __NODE_TREE_H__
#define __NODE_TREE_H__

#define METHOD_GET  1
#define METHOD_PUT  2
#define METHOD_POST 3

#define PROTOCOL_PRINT printf

typedef struct node_t
{
    char servicename[256];
    void (*process)(struct node_t *node, void *args);
    struct node_t *prenode;
    struct node_t *nextnode;
    unsigned char method;
    char *description;
}node_t;

typedef void (*process)(struct node_t *node, void *args);

extern node_t root_node[];
extern process getProcess(node_t *node, char *path, node_t **targetNode);
#endif