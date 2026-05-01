#ifndef __LIST_H__
#define __LIST_H__

#if defined (__cplusplus) //在c++编译器中依然按照C语言的方式进行编译
extern "C" {
#endif

typedef struct node{
    struct node *next;
    struct node *prev;
}NODE;

typedef struct list_head{
    NODE node;
    int count;
}LIST;


/* 获取结构体成员和结构体头之间的偏移量 */
#define offsetof(type, member) (unsigned long)&((type*)0)->member

/* 计算结构体首地址，typeof可以获取变量的类型 */
#define container_of(ptr, type, member) ({  \
    const typeof( ((type *)0)->member) *__mptr = (ptr); \
    (type *)((char *)__mptr - offsetof(type, member));})

#if 0
#define list_entry container_of
#define INIT_LIST_HEAD init_list_head   // 初始化
#define LIST_POISON1 NULL
#define LIST_POISON2 NULL

/**
 * @brief<遍历容器结构中的链表>
 * @prefetch(pos->next): 对数据进行手工预取，减少读取延迟，提高性能可删除
 * @pos: 临时变量，当前位置，用以遍历链表
 * @head: 被遍历的链表，结构体person1(容器结构)中的list
 */
#define list_for_each(pos, head) \
    for(pos = (head)->next; /*prefetch(pos->next),*/pos!=(head); pos = pos->next)

/**
 * @brief<遍历容器结构中的链表，同时获取容器结构的地址保存在pos中>
 * @pos: 容器结构类型指针
 * @head: 初始化的那个头指针
 * @member: 容器结构中的链表名
 */
#define list_for_each_entry(pos, head, member)  \
    for(pos = list_entry((head)->next, typeof(*pos), member);  \
    &pos->member != (head);  \
    pos = list_entry(pos->member.next, typeof(*pos), member))


/**
 * @brief<使用中间变量n做缓存，提升了安全性>
 * @pos: 容器结构类型指针
 * @n: 中间变量，用以缓存需要删除节点的下一个节点
 * @head: 初始化的那个头指针
 * @member: 容器结构中的链表名
 */
#define list_for_each_entry_safe(pos, n, head, member)  \
    for(pos = list_entry((head)->next, typeof(*pos), member),  \
    n = list_entry(pos->member.next, typeof(*pos), member);  \
    &pos->member != (head);  \
    pos = n, n = list_entry(n->member.next, typeof(*n), member))
#endif

void msleep(unsigned int ms);

void lstInit(LIST *pList);

void lstNodeInit(NODE *pNode);
/**
 * @fn list_add
 * @brief<使用头插法将new插入到链表中，最后插入的节点将成为链表第一个节点>
 */
void lstAdd(LIST *pList, NODE *pNode);

/**
 * @fn list_add_tail
 * @brief<使用尾插将new插入到链表中，最后插入的节点将成为链表第一个节点>
 */
void lstAddTail(LIST *pList, NODE *pNode);

void lstAddHead(LIST *pList, NODE *pNode);

void lstDel(LIST *pList, NODE *pNode);

NODE *lstFirst(LIST *pList);

NODE *lstLast(LIST *pList);

NODE *lstNext(NODE *pNode);

NODE *lstPrev(NODE *pNode);

int lstLength(LIST *pList);

NODE *lstDth(LIST *pList, int index);

void lstFree(LIST *pList);

int lstMerge(LIST *pList, LIST *pList2);

int lstMergeN(LIST *pList, NODE *startNode, NODE *endNode, LIST *pList2);



#if defined (__cplusplus)
}
#endif
#endif