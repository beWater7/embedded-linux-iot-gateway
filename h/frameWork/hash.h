#ifndef _HASH_H_
#define _HASH_H_

/* 
    hash的结构如下：
    1   []-[]-[]-[]-[]  
    2   []-[]-[]-[]-[]
    3   []-[]-[]-[]-[]
    4   []-[]-[]-[]-[]
    
    链表的头节点放在数组bucket中, node该放在哪个链表中有key映射得到的hash值决定
    hash值离散情况越好，链表越短，当每个链表只有一个元素时 hash 的bucket类似于数组
    此时查找效率最高，因为不需要遍历链表
 */


#define BUCKETCOUNT 10 // 相当于10条链表

/*链表节点*/
struct Node{
	char* key;
	char* value;
	struct Node *next;
};

typedef struct Node node;

/*顺序哈希桶*/
struct hashTable{
    node bucket[BUCKETCOUNT];/*先默认定义10个桶*/
};

typedef struct hashTable table;


void initHashTable(table *t);
//struct Node* lookup(const char* key);
int hashInsertNode(table *t,char *key,char *value);
void hashDisplay(table *t);
void hashFree(table *t);
int hashCount(table *t, const char *key);
char* hashFind(table *t, const char *key);
void hashClear(table *t);

#endif