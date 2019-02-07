#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
main(){
   // 基于当前系统的当前日期/时间
   time_t now = time(0);
   char buffer[100];
   char pin[100];
   

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
	sprintf(buffer, "[%d.%d.%d]%d:%d:%d",(1900+ltm->tm_year),(1 + ltm->tm_mon),ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
	printf("%s\n",buffer);
	
   
//   printf("[%d.%d.%d]%d:%d:%d \n",(1900+ltm->tm_year),(1 + ltm->tm_mon),ltm->tm_mday,ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
   
   return 0;
}