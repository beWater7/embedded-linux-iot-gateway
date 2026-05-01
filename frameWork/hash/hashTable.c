#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash.h"


/*初始化哈希桶*/
void initHashTable(table *t){
    int i;
    if(!t) 
        return;

    for(i = 0; i < BUCKETCOUNT; ++i){
        t->bucket[i].key = NULL;
        t->bucket[i].next = NULL;
        t->bucket[i].value = NULL;
    }
}

/*哈希函数，根据生成哈希值*/
static int hash(const char* key) {

	int hash=0;
	for (; *key; ++key)
	{
		hash=hash*33+*key;
	}
    return hash%BUCKETCOUNT;

    //return *key%BUCKETCOUNT;
}
/*****
hash函数的意义就是生成散列值
将任意长度的输入通过散列算法变换成固定长度的输出，
该输出就是散列值
*****/

/* 插入数据到hash表中
 * 问题是为什么这么做？ malloc分配内存的情况变得更复杂
 */
int hashInsertNode(table *t,char *key,char *value){
    int index, vlen1, vlen2;
    node *e, *ep;

    if(t == NULL || key == NULL || value == NULL) return -1;

    index = hash(key);
	printf("index=%d\n", index);
	
    //如果现在桶为空，直接写入第一个桶节点
    if(!t->bucket[index].key){
        t->bucket[index].key = strdup(key);  //将key拷贝到堆区内存，并返回
        t->bucket[index].value = strdup(value);
    }
    else{
        e = ep = &(t->bucket[index]);
        /*先从已经存在的找*/
        while(e){
            /*如果key值重复，替换相应的值*/
            if(strcmp(e->key, key) == 0){
                vlen1 = strlen(value);
                vlen2 = strlen(e->value);
                if(vlen1 > vlen2){
                    free(e->value);
                    e->value = (char *)malloc(sizeof(vlen1 + 1));
                }
                memcpy(e->value, value, vlen1 + 1);
                return index;/*插入完成*/
            }
            ep = e;
            e = e->next;
        }
        /*没有在当前桶中找到相同key的节点，创建新的节点加入，并加入桶链表*/
        e = (node *)malloc(sizeof(node));
        if(NULL != e){
            e->key = strdup(key);
            e->value = strdup(value);
            e->next = NULL;
            ep->next = e;
        }
    }
    return index;
}

/*打印hash表*/
void hashDisplay(table *t){
    int i;
    node *e;
    if(!t) return;

    for(i = 0; i < BUCKETCOUNT; i++){
        printf("bucket[%d]:", i);
        e = &(t->bucket[i]);
        while(e){
            printf("%s = %s\t\t",e->key, e->value);
            e = e->next;
        }
        printf("\n");
    }
}

/* 清空hash中的所有元素、模拟C++中unorder_map的clear函数*/
void hashClear(table *t)
{
    int i = 0;
    node *cur = NULL;
    node *tmp = NULL;
    if(!t) return;

    for(i = 1; i < BUCKETCOUNT; i++){
        cur = t->bucket[i].next; 
        while(cur){
            tmp = cur;
            cur = cur->next;
            memset(tmp->key, 0, strlen(tmp->key));
            memset(tmp->key, 0, strlen(tmp->key));
        }
        memset(t->bucket[i].key, 0, strlen(t->bucket[i].key));   //释放头
        memset(t->bucket[i].value, 0, strlen(t->bucket[i].value));
    }
}

/* 释放所有内存 */
void hashFree(table *t)
{
    int i = 0;
    node *cur = NULL;
    node *tmp = NULL;
    if(!t) return;

    for(i = 1; i < BUCKETCOUNT; i++){
        cur = t->bucket[i].next; 
        while(cur){
            tmp = cur;
            cur = cur->next;
            free(tmp->key);
            free(tmp->value);
            free(tmp);
        }
        free(t->bucket[i].key);   //释放头
        free(t->bucket[i].value);  
    }
    free(t);
}

/* 寻找键是否在hash table中、模拟C++中unorder_map的count函数 */
int hashCount(table *t, const char *key)
{
    int index = 0;
    node *e = NULL;
    node *ep = NULL;

    if(t == NULL || key == NULL) 
        return -1;

    index = hash(key);

    e = &t->bucket[index];
    while(e)
    {
        if(!strcmp(key, e->key))
        {
            return 1;
        }
        e = e->next;
    }
    return 0;
}

/* 根据键寻找对应的值、模拟C++中unorder_map的find函数*/
char* hashFind(table *t, const char *key)
{
    int index = 0;
    node *e = NULL;
    node *ep = NULL;

    if(t == NULL || key == NULL) 
        return NULL;

    index = hash(key);
    //printf("%s %s\n", t->bucket[index].key, key);
    e = &t->bucket[index];
    /* 多个key可能对应同一个hash值（仅为10）、需要遍历链表 */
    while(e)
    {
        if(!strcmp(key, e->key))
        {
            return e->value;
        }
        e = e->next;
    }
    return NULL;
}


#if 1
int main(int argc, char* argv[])
{
	table *ht;
	ht = (table *)malloc(sizeof(table));
    initHashTable(ht);
	char* key[]={"a","b","c","d","e","f","g","h","i","j",
                "k","l","m","n","o","p","q","r","s","t",
                "u","v","w","x","y","z"};
	char* value[]={"apple","banana","china","doge","egg","fox",
                "goat","hello","ice","jeep","key","love",
                "mirror","net","orange","panda","quarter","rabbit",
                "shark","tea","unix","volley","wolf","x-ray","yard","zoo"};
	for (int i = 0; i < 26; ++i)
	{
		hashInsertNode(ht, key[i], value[i]);
	}
	hashDisplay(ht);
    printf("has %s\n", hashCount(ht, "w")?"w":"null");
    printf("has %s\n", hashCount(ht, "r")?"r":"null");
    
    printf("has %s\n", hashFind(ht, "s"));
    printf("has %s\n", hashFind(ht, "z"));

    hashClear(ht);
    printf("has %s\n", hashCount(ht, "r")?"r":"null"); 
    printf("has %s\n", hashFind(ht, "s"));

    hashFree(ht);
	return 0;
}
#endif


































