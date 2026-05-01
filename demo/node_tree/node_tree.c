#include <stdio.h>
#include <string.h>
#include "node_tree.h"

//#define DEBUG_MODE

node_t system[];
node_t logMgr[];
node_t logConfig[];

void systemConfig(node_t *node, void *args)
{
    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
}

void systemInfo(node_t *node, void *args)
{
    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
}

void logConfigV2(node_t *node, void *args)
{
    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
}

void logInfo(node_t *node, void *args)
{
    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
}

void network(node_t *node, void *args)
{
    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
}

void logInfoV2(node_t *node, void *args)
{
    printf("[%s:%d]\n",__FUNCTION__,__LINE__);
}


/* 根节点数组 */
node_t root_node[] = {
    {"system", NULL, NULL, system, METHOD_GET, "系统调用"},
    {"logInfo", NULL, NULL, logMgr, METHOD_GET, "日志"},
    {"network", network, NULL, NULL, METHOD_GET | METHOD_POST, "网络参数"},
    {"/root_node", NULL, NULL, NULL, METHOD_GET, "hello"},
};

node_t logMgr[] = {
    {"config", NULL, root_node, logConfig, METHOD_GET, NULL},
    {"info", logInfo, root_node, NULL, METHOD_GET, NULL},
    {"/logInfo",NULL, NULL, NULL, METHOD_GET, NULL},
};

node_t system[] = {
    {"config", systemConfig, root_node, NULL, METHOD_GET, NULL},
    {"info", systemInfo, root_node, NULL, METHOD_GET, NULL},
    {"/system", NULL, NULL, NULL, METHOD_GET, NULL},
};

node_t logConfig[] = {
    {"config", logConfigV2, logMgr, NULL, METHOD_GET, NULL},
    {"info", logInfoV2, logMgr, NULL, METHOD_GET, NULL},
    {"/system", NULL, NULL, NULL, METHOD_GET, NULL},
};



process getProcess(node_t *node, char *path, node_t **targetNode)
{
    char pathCopy[128] = {0};
    char *token = NULL;
    node_t *currentLevel = node;
    node_t *foundNode = NULL;

    if (!node || !path)
        return NULL;

    strncpy(pathCopy, path, sizeof(pathCopy) - 1);
    pathCopy[sizeof(pathCopy) - 1] = '\0';

    // 以 “/” 分割路径，如 logInfo/config/config
    token = strtok(pathCopy, "/");
    while (token && currentLevel)
    {
        foundNode = NULL;

        // 遍历当前层节点数组，匹配名字
        while (*(currentLevel->servicename))
        {
            if (strcmp(currentLevel->servicename, token) == 0)
            {
                foundNode = currentLevel;
                break;
            }
            currentLevel++;
        }

        if (!foundNode)
        {
            PROTOCOL_PRINT("no match node: %s\n", token);
            return NULL;
        }

        // 匹配到节点后，看是否是最后一级
        token = strtok(NULL, "/");
        if (!token)
        {
            // 最后一级 → 返回其 process
            if (foundNode->process)
            {
                *targetNode = foundNode;
                return foundNode->process;
            }
            else
            {
                PROTOCOL_PRINT("node has no service: %s\n", foundNode->servicename);
                return NULL;
            }
        }
        else
        {
            // 继续往下一级
            currentLevel = foundNode->nextnode;
        }
    }
    PROTOCOL_PRINT("no match node: %s\n", path);
    return NULL;
}

#if 0
/* 根据url获取node和回调函数、基于一个前提：node有next节点就不能有回调函数
   获取节点需要传二级指针、传一级指针不行
*/
process getProcess(node_t *node, char *path, node_t **targetNode)
{
    char *p1 = NULL;
    char *p2 = NULL;
    char servicename[256] = {0};
    int len = 0;
    node_t *pcurrentnode = NULL;
    node_t *lastnode = NULL;
    /* 标识是否找到匹配的节点 */
    static char byFindService = 1;

    if(NULL == path || NULL == node)
    {
        return NULL;
    }
    p1 = path;

    pcurrentnode = node;

    /* 遍历root_node数组中所有根节点 */
    while(*(pcurrentnode->servicename) != '/')
    {
        /* 获取当前节点 */
        *targetNode = pcurrentnode;
#ifdef DEBUG_MODE
        printf("node->servicename:%s\n",pcurrentnode->servicename);
#endif
        if(byFindService)
        {
#ifdef DEBUG_MODE
            printf("p1:%s p2:%s\n",p1,p2);
#endif
            p2 = strstr(p1, "/");
            if(NULL == p2)
            {
                /* 只剩最后一个，获取service */
                strcpy(servicename, p1);
                goto exit;
            }
            /* 获取service name len */
            len = p2 - p1;
            /* 跳过 "/" */
            p2 += 1;
            //printf("%d\n",len);
            if(len < 0 || len > sizeof(servicename))
            {
                printf("invalid len\n");
                return NULL;
            }
            //printf("%s\n", p1);
            memset(servicename, 0, sizeof(servicename));
            strncpy(servicename, p1, len);
            p1 = p2;
#ifdef DEBUG_MODE
            printf("name:%s\n",servicename);
#endif
        }
exit:
        if(strstr(pcurrentnode->servicename, servicename))
        {
            /* 找到回调函数 */
            if(pcurrentnode->process)
            {
                printf("exit %s\n",pcurrentnode->servicename);
                return pcurrentnode->process;
            }
            else if(pcurrentnode->nextnode)
            {
                /* 找到下一个节点 */
                pcurrentnode = pcurrentnode->nextnode;
                byFindService = 1;
                continue;
            }
            else
            {
                /* 节点的回调函数为空，且没有下一个节点 */
                return NULL;
            }
        }
        else
        {
            byFindService = 0;
        }
        /* 进入下一个节点寻找 */
        pcurrentnode++;
    }
    printf("no match node\n");
    return NULL;
}
#endif

