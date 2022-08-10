/**
 * # Threads > 4 (MAX_THREADS) but waiting for them to exit.
 * Expected Verdict: All threads should be created/joined/exited successfully
 */
#include<ulib.h>
#include<gthread.h>
static void* thfunc1(void* arg) {
  int* ptr = (int*)arg;
  int cnt = *(int*)arg;
  printf("[pid %d]Arg is %d\n", getpid(), *ptr);
  for (int ctr = 0; ctr < cnt; ++ctr) {
    *ptr += 100;
  }
  return ptr;
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5) {
  int num_of_threads = 7;
  int s_count[num_of_threads];
  int tid[num_of_threads];
  int ctr;
  void* retval;
  for (ctr = 0; ctr < num_of_threads;) {
    int i;
    for (i = 0; i < MAX_THREADS && i + ctr < num_of_threads; i++) {
      int ctr_ = ctr + i;
      s_count[ctr_] = ctr_ + 1;
      if (gthread_create(&tid[ctr_], thfunc1, &s_count[ctr_]) < 0) {
        printf("gthread_create failed\n");
        exit(-1);
      }
      printf("Created thread: %d\n", tid[ctr_]);
    }
    for (i = 0; i < MAX_THREADS && i + ctr < num_of_threads; i++) {
      int ctr_ = ctr + i;
      retval = gthread_join(tid[ctr_]);
      printf("Thread %d returned is %d\n", tid[ctr_], *((int*)retval));
    }
    ctr += i;
  }
  return 0;
}
