#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#ifdef __unix__
# include <unistd.h>
#elif defined _WIN32
# include <windows.h>
#define sleep(x) Sleep(1000 * (x))
#endif
#include <stdio.h>
#pragma comment(lib,"x86/pthreadVC2.lib")

// 子執行緒函數
void* child(void* data) {
  char *str = (char*) data; // 取得輸入資料
  for(int i = 0;i < 5;++i) {
    printf("%s\n", str); // 每秒輸出文字
    sleep(3);
  }
  pthread_exit(NULL); // 離開子執行緒
}

// 主程式
int main() {
  pthread_t t; // 宣告 pthread 變數
  pthread_create(&t, NULL, child, "Child"); // 建立子執行緒

  // 主執行緒工作
  for(int i = 0;i < 5;++i) {
    printf("Master\n"); // 每秒輸出文字
    sleep(2);
  }

  pthread_join(t, NULL); // 等待子執行緒執行完成
  return 0;
}