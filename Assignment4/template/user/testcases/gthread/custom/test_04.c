/**
 * Check gmalloc, gfree and memory leaks
 */
#include<ulib.h>
#include<gthread.h>

static void* thfunc1(void* arg) {
  int ctr, ret;
  u64* th_priv_ptr = (u64*)arg;
  for(int i=0; i<100; i++){
    char* ptr = (char*)gmalloc(8192, GALLOC_OWNONLY);
    ptr[0]='a';
    ret=gfree((void*)(ptr));
    if(ret!=0){
      printf("gfree failed %d!=0", ret);
      return NULL;
    }
  }
  char* ptr = (char*)gmalloc(8192, GALLOC_OWNONLY);  // Allocate and place the pointer
  ret=gfree((void*)(ptr+1));
  if(ret!=-1){
    printf("gfree failed %d!=-1", ret);
    return NULL;
  }
  for (ctr = 0; ctr < 100; ++ctr) {
    ptr[ctr] = 'a' + (ctr % 26);
  }
  ptr[ctr] = 0;
  *th_priv_ptr = (u64)ptr;
  ret=gfree((void*)(ptr));
  if(ret!=0){
    printf("gfree failed %d!=0", ret);
  }
  return NULL;
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  u64 th_priv_addr = 0;
  int tid1, tid2;
  if (gthread_create(&tid1, thfunc1, (void*)&th_priv_addr) < 0) {
    printf("gthread_create failed\n");
    exit(-1);
  }
  printf("Created thread: %d\n", tid1);

  gthread_join(tid1);
  printf("Thread %d returned\n", tid1);
  return 0;
}
