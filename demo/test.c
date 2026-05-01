#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "list.h"

struct stu{
    NODE node;
    int score;
};

#if 1
int main()
{
    int i = 0;   
    LIST list;
    lstInit(&list);
    struct stu *st;
    for(i = 0; i < 10; i++)
    {
        st = (struct stu *)malloc(sizeof(struct stu));
        st->score = i;
        //lstAddTail(&list,&st->node);
        lstAddHead(&list, &st->node);
    }
    printf("length:%d\n", lstLength(&list));
    struct stu *pos;
    NODE *node;
    pos = (struct stu *)lstFirst(&list);
    printf("pos->score:%d\n",pos->score);
    struct stu *next;
    while(pos)
    {
        next = (struct stu *)lstNext(&pos->node); 
        printf("pos->score:%d\n",pos->score);
        pos = next;
    }

  #if 1 
    pos = (struct stu *)lstFirst(&list);
    while(pos)
    {
        if(pos->score == 9)
        {
            printf("000\n");
            lstDel(&list, &pos->node);
            break;
        }
        pos = (struct stu *)lstNext(&pos->node); // 有空指针风险

    }
#endif


    pos = (struct stu *)lstFirst(&list);
    while(pos)
    {
        next = (struct stu *)lstNext(&pos->node); 
        printf("pos->score:%d\n",pos->score);
        pos = next;
    }


    printf("length:%d\n", lstLength(&list));
    pos = (struct stu *)lstLast(&list);
    printf("pos->score:%d\n",pos->score);

    pos = (struct stu *)lstDth(&list, 5); // 1表示获取第一个元素
    printf("pos->score:%d\n",pos?pos->score:-1);

    LIST list2;
    lstInit(&list2);
    for(i = 17; i < 28; i++)
    {
        st = (struct stu *)malloc(sizeof(struct stu));
        st->score = i;
        //lstAddTail(&list,&st->node);
        lstAddHead(&list2, &st->node);
    }

#if 0
    /*链表合并测试*/
    lstMerge(&list, &list2);
    /*打印测试结果*/
    pos = (struct stu *)lstFirst(&list);
    while(pos)
    {
        next = (struct stu *)lstNext(&pos->node); 
        printf("pos->score:%d\n",pos->score);
        pos = next;
    }
#else
    /*链表按节点合并测试 */
    struct stu *pos2;
    pos = (struct stu *)lstFirst(&list2);
    while(pos)
    {
        if(pos->score == 26)
        {
            break;
        }
        pos = (struct stu *)lstNext(&pos->node);
    }

    pos2 = pos;
    while(pos2)
    {
        if(pos2->score == 21)
        {
            break;
        }
        pos2 = (struct stu *)lstNext(&pos2->node);
    }

    lstMergeN(&list, &pos->node, &pos2->node, &list2);
    printf("list->count:%d\n",lstLength(&list));
    pos = (struct stu *)lstLast(&list);
    while(pos)
    {
        next = (struct stu *)lstPrev(&pos->node); 
        printf("pos->score:%d\n",pos->score);
        pos = next;
    }

#endif
    lstFree(&list);
    return 0;
}
#endif
