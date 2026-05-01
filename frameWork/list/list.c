#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep的头文件
#include "list.h"
#define HEAD node.next
#define TAIL node.prev

/* 自己实现的msleep函数 */
void msleep(unsigned int ms)
{
    usleep(ms * 1000);
}



void lstInit(LIST *pList)
{
     pList->HEAD = NULL;
     pList->TAIL = NULL;
     pList->count = 0; 
}

void lstNodeInit(NODE *pNode)
{
    pNode->next = NULL;
    pNode->prev = NULL;
}



void lstInsertTail(LIST *pList, NODE *prev, NODE *pNode)
{
    if(NULL == prev)
    {
        pList->HEAD = pNode;
        pList->TAIL = pNode;
    }
    else
    {
        prev->next = pNode;
    }

    pNode->prev = prev;
    pNode->next = NULL;
    pList->TAIL = pNode;

    pList->count++;
}


void lstInsertHead(LIST *pList, NODE *next, NODE *pNode)
{
    if(NULL == next)
    {
        pList->HEAD = pNode;
        pList->TAIL = pNode;
    }
    else
    {
        pNode->next = next;
    }
    
    pNode->prev = NULL;
    if(NULL == next)
    {
        pNode->next = NULL;
    }
    else
    {
        next->prev = pNode;
        pList->HEAD = pNode;
    }
    pList->count++;
}

/**
 * @fn list_add_tail
 * @brief<使用尾插将new插入到链表中，最后插入的节点将成为链表第一个节点>
 */
void lstAddTail(LIST *pList, NODE *pNode)
{
    return lstInsertTail(pList, pList->TAIL, pNode);
}


/*********************************************
                                             +
 第一个节点插入后的情况：                      +
    head--->1--->head                        +
                                             +
 头插的情况：插入到head 和 head->next(1)之间   +
         2(new)                              +
    head--->1--->head                        +
            |                                +
            |                                +
    head--->2--->1--->head                   +
                                             +
尾插的情况：插入到head->prev(1) 和 head 之间   +
              2(new)                         +
    head--->1--->head                        +
            |                                +
            |                                +                                            
    head--->1--->2--->head                   +
                                             +
*********************************************/

/**
 * @fn list_add
 * @brief<使用头插法将new插入到链表中，最后插入的节点将成为链表第一个节点>
 */
void lstAddHead(LIST *pList, NODE *pNode)
{
    return lstInsertHead(pList, pList->HEAD, pNode);                                               
}

void lstDel(LIST *pList, NODE *pNode)
{
    NODE *tmp;
    if(0 == pList->count || NULL == pNode)
    {
        printf("[%s:%d]invalid para!\n",__FUNCTION__,__LINE__);
        return;
    }

    if(1 == pList->count)
    {
        free(pNode);
        pList->HEAD = NULL;
        pList->TAIL = NULL;
    }
    else if( pList->HEAD == pNode)
    {
        tmp = pList->HEAD->next;
        tmp->prev = NULL;
        free(pNode);
        pList->HEAD = tmp;
    }
    else if(pList->TAIL == pNode)
    {
        tmp = pList->TAIL->prev;
        tmp->next = NULL;
        free(pNode);
        pList->TAIL = tmp;
    }
    else
    {
        pNode->prev->next = pNode->next;
        pNode->next->prev = pNode->prev;
        pNode->next = NULL;
        pNode->prev = NULL;
        free(pNode);
    }
    pList->count--;
}

NODE * lstFirst(LIST *pList)
{
    return pList->HEAD;
}

NODE * lstLast(LIST *pList)
{
    return pList->TAIL;
}

int lstLength(LIST *pList)
{
    return pList->count;
}

NODE *lstDth(LIST *pList, int index)
{
    if((index < 1) || pList->count < index)
    {
        printf("[%s:%d]invalid para!\n",__FUNCTION__,__LINE__);
        return NULL;
    }
    int i,j;
    NODE *node, *tmp;
    if(index <= (pList->count >> 1))
    {
        node = pList->HEAD;
        while(--index)
        {
            node = node->next;
        }
    }
    else
    {
        j = pList->count - index;
        node = pList->TAIL;
        while(j--)              // 如果此处用--j会少执行一次
        {
            //printf("111\n");
            node = node->prev;
        }
    }
    return node;
}


void lstFree(LIST *pList)
{
    NODE *tmp, *node;
    node = pList->HEAD;
    while(node)
    {
        tmp = node->next;
        free(node);
        node = tmp;
    }
    pList->HEAD = NULL;
    pList->TAIL = NULL;
    pList->count = 0;
}


NODE *lstNext(NODE *pNode)
{
    if(pNode)
        return pNode->next;
    return NULL;
}

NODE *lstPrev(NODE *pNode)
{
    if(pNode)
        return pNode->prev;
    return NULL;
}

int lstMerge(LIST *pList, LIST *pList2)
{
    if(NULL == pList || NULL == pList2)
    {
        return -1;
    }
    /* 合并两个链表 */
    pList->TAIL->next = pList2->HEAD;
    pList2->HEAD->prev = pList->TAIL;

    /* 更新链表尾部指针和长度 */
    pList->TAIL = pList2->TAIL;
    pList->count += lstLength(pList2);

    /**/
    lstInit(pList2);
    return 0;
}

/* 按节点合并链表, 目前可能有内存泄漏的风险 
 * 避免泄漏的方法：将plist2多余的节点free后形成新的链表再进行merge
 */
int lstMergeN(LIST *pList, NODE *startNode, NODE *endNode, LIST *pList2)
{
    static int subListflag = 0; // 记录是否子串开始的标志位，用来记录链表长度什么时候应该增长
    if(NULL == pList || NULL == pList2 
      || NULL == startNode || NULL == endNode)
    {
        printf("[%s:%d]invalid para!\n",__FUNCTION__,__LINE__);
        return -1;
    }

    /* 如果startNode 和 endNode 顺序反了，会发生意料之外的事，此处应避免
     * 但使用地址比较的方法并不绝对准确，
     */
    if(startNode - endNode < 0)
    {
        printf("[%s:%d]invalid para!\n",__FUNCTION__,__LINE__);
        return -1;
    }
#if 0
    NODE *node = lstFirst(pList2);
    NODE *nextNode = NULL;
    while(node)
    {
        nextNode = lstNext(node);
        /*连接plist和plist2->HEAD*/
        if(startNode == node)
        {
            printf("66666\n");
            /* 断开前一个节点*/
            if(node->prev)
                node->prev->next = NULL;
            /*建立和第一个链表的连接*/
            node->prev = pList->TAIL;
            pList->TAIL->next = node;
            subListflag = 1; 
        }
        if(subListflag)
            pList->count++;
        /*去掉plist2的多余的尾巴*/
        if(endNode == node)
        {
            printf("77777\n");
            /*断开和后一个节点的联系*/
            node->next->prev = NULL;
            node->next = NULL;
            pList->TAIL = node;
            break;
        }
        node = nextNode;
    }
    /**/
    //lstInit(pList2);
#else
    NODE *node = lstFirst(pList2);
    NODE *nextNode = NULL;
    NODE *prevNode = NULL;
    /*从HEAD开始遍历释放头部多余的节点*/
    while(node)        
    {
        nextNode = lstNext(node);
        if(startNode != node)
        {
            free(node);
            pList2->count--;
        }
        else
        {
            node->prev = NULL;
            pList2->HEAD = node;
            break;
        }
        node = nextNode;
    }
    node = lstLast(pList2);
    /*从TAIL开始遍历释放尾部多余的节点*/
    while(node)        
    {
        prevNode = lstPrev(node);
        if(endNode != node)
        {
            free(node);
            pList2->count--;
        }
        else
        {
            node->next = NULL;
            pList2->TAIL = node;
            break;
        }
        node = prevNode;
    }
    /* 两个完整链表合并*/
    lstMerge(pList,pList2);
#endif
    return 0;
}
