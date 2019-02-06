
#include <stdio.h>
#include <string.h>
 
int main()
{
	char szTest[1000] = {0};
	int len = 0;
 
	FILE *fp = fopen("device.txt", "r");
	if(NULL == fp)
	{
		printf("failed to open dos.txt\n");
		return 1;
	}
 
	while(!feof(fp))
	{
		memset(szTest, 0, sizeof(szTest));
		fgets(szTest, sizeof(szTest) - 1, fp); // 包含了换行符
		printf("%s", szTest);
	}
 
	fclose(fp);
 
	printf("\n");
 
	return 0;

}