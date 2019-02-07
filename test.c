#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
main(){
   // 基于当前系统的当前日期/时间
   time_t now = time(0);
   
   

    struct tm *ltm;
  
    ltm = localtime(&now);
   
   // 把 now 转换为字符串形式
   char* dt = ctime(&now);
	int a=1900+ltm->tm_year;
	int b=1 + ltm->tm_mon;
	int c=ltm->tm_mday;
	int d=ltm->tm_hour;
	int e=ltm->tm_min;
	int f=ltm->tm_sec;
   
   printf("[%d.%d.%d]%d:%d:%d \n",(1900+ltm->tm_year),(1 + ltm->tm_mon),ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
   
   return 0;
}