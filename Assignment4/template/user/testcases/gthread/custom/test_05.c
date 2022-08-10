/**
 * Check private memory access
 */
#include<ulib.h>
#include<gthread.h>

typedef struct thread_argument {
  u64* th_priv_ptr;
  u64 access_flag;
} thread_argument;

static void* thfunc1(void* arg) {
  thread_argument* thread_arg = (thread_argument*)arg;
  int ctr;
  u64* th_priv_ptr = thread_arg->th_priv_ptr;
  u64 access_flag = thread_arg->access_flag;
  char* ptr = (char*)gmalloc(8192, access_flag);
  for (ctr = 0; ctr < 26; ++ctr) {
    ptr[ctr] = 'a' + (ctr % 26);
  }
  ptr[ctr] = 0;
  *th_priv_ptr = (u64)ptr;
  sleep(10);    // Need to sleep for other thread to finish
  gfree((void*)ptr);
  return NULL;
}

static void* thfunc2(void* arg) {
  char* ptr;
  u64* th_priv_ptr = (u64*)arg;
  while (*th_priv_ptr == 0)
    sleep(1);
  ptr = (char*)*th_priv_ptr;
  printf("%s\n", ptr);
  ptr[0]='a';
  return NULL;
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  u64 all_flags[] = { GALLOC_OWNONLY, GALLOC_OTRDONLY, GALLOC_OTRDWR };
  for(int i=0; i<3; i++){
    u64 th_priv_addr = 0;
    thread_argument thread_arg={&th_priv_addr, all_flags[i]};
    int tid1, tid2;
    if (gthread_create(&tid1, thfunc1, (void*)&thread_arg) < 0) {
      printf("gthread_create failed\n");
      exit(-1);
    }
    printf("Created thread: %d\n", tid1);

    if (gthread_create(&tid2, thfunc2, (void*)&th_priv_addr) < 0) {
      printf("gthread_create failed\n");
      exit(-1);
    }
    printf("Created thread: %d\n", tid2);
    thfunc2((void*)&th_priv_addr);
    gthread_join(tid1);
    printf("Thread %d returned\n", tid1);
    gthread_join(tid2);
    printf("Thread %d returned\n", tid2);
  }
  return 0;
}
