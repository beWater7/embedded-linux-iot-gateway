/*****KMP 算法实现 字符串匹配问题 *****
          参考B站印度程序员       ****

****  next数组, 通过左指针j, 右指针i(c从1开始)实现构造，
      1、当模式串pattern[i] = pattern[j]时，i++, j++; next[i]为j+1                      
      2、当               ！=           时，需要考虑两种情况
	     (1) j == 0时， 不需要回退 
		 (2) j > 0 时， j = next[j-1]; --i(为了进行下一次的判断，必须保持i的位置，循环会自动+1) 
		 
		 
*** 文本串和模式串的匹配，
      1、不匹配时，模式串的指针回退，且保持k的位置 --k，以便下一次比较 
	  2、匹配时，i++; k++(循环会自动加)
	  3、循环结束时, i指向模式串最后一个元素，说明包含 
			 
***************************************/
	   
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include "stringAlgo.h"

int kmp(char *text, char *pattern, int lenT, int lenP)
{
	/* 构造前缀表 */ 
    if(0 == lenP) return 0;
	
	int i; 
	int j = 0;                //定义左指针j; 右指针i;   
    int next[256] = {0};     //定义前缀表 数组 
    for(i=1; i < lenP; i++)
    {
    	if(pattern[i] == pattern[j])
    	{
		   next[i] = j+1;  
		   j++;
		}
		
		else
		{  
		   if(0 == j)           //不需要回退 
		   {
		      next[i] = j; 
	       }
	       
	       else                //需要回退 
		   {
		     j = next[j-1];  
 		     --i; 	         
		   } 
		}
		
	}
   
   /*显示前缀表*/ 
   for(i = 0; i < lenP; i++)
   {
   	    printf("%d ",next[i]);
    	
   } 
   printf("\n");
 
  
  /** 文本串和模式串的比较 */
  int k = 0;
  i = 0;
  for(; k < lenT && i < lenP; k++)
  {
     if(pattern[i] == text[k])
     {
	    i++;
	 }
	 
	 else 
	 {  
	    if(0 != i) 
		{
		   i = next[i-1];
		   --k;
	    }
	    
	    printf(" i = %d\n",i);
     }
  }
  
  printf("i = %d k = %d\n", i, k);
  if(lenP == i)  return k - i;
  else return -1; 
}


#if 0
int main()
{
  char text[] = "abxabcabcaby";
  char pattern[] = "abcaby";
  printf("index = %d\n" , kmp(text, pattern, strlen(text), strlen(pattern)));   
  
  return 0;
}
#endif
