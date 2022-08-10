
#include<ulib.h>
#include<gthread.h>

/*XXX   Do not declare global or static variables in the test cases*/

/*Thread functions must be declared as static*/
static void *thfunc1(void *arg){
      int ctr;
            u64 *th_priv_ptr = (u64 *) arg;
      char *ptr = (char *)gmalloc(8192, GALLOC_OTRDWR);  // Allocate and place the pointer
                                                        // in thread argument i.e., &th_priv_addr 
            for(ctr=0; ctr < 100; ++ctr){
                   ptr[ctr] = 'a' + (ctr % 26);
            } 
      ptr[ctr] = 0;
      *th_priv_ptr = (u64) ptr;
      sleep(20);    // Need to sleep for other thread to finish
      gfree((void *)ptr);
            return NULL;
}

static void *thfunc2(void *arg){
      char *ptr;
            u64 *th_priv_ptr = (u64 *) arg;
      while(*th_priv_ptr == 0)
         sleep(1);
            ptr = (char *) *th_priv_ptr;
      printf("%s\n", ptr);   // Reading only, should be allowed
            return NULL;
}

static void *thfunc3(void *arg){
          //   sleep(20);
          while(*(u64 *)arg == 0){ sleep(1); }
          u64 *thaddr = (*(u64 *) arg);
          printf(" Address inside other threads - %x\n", thaddr);
            for(int ctr=0; ctr < (*thaddr)/20; ++ctr){
         printf("[pid %d]Arg is %d\n", getpid(), *thaddr);
            } 
            if(*thaddr == 69)
              *thaddr = *thaddr + 100;
            gthread_exit(thaddr);
}

static void *thfunc4(void *arg){
          //   sleep(20);
            u64 *ptr = (u64 *)gmalloc(8192, GALLOC_OTRDONLY);
            *ptr = 69;
            printf("__________ %d %x__________\n", *ptr, ptr);

            *(u64 *)arg = ptr;
            sleep(2);
            *ptr = *ptr + 100;
            sleep(1000);

            *(u64 *)arg = 0;
            gthread_exit(ptr);
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  u64 th_priv_addr = 0;   
  int tid1, tid2;

  int s_count[8];
  int tid[8];
  int ctr;
  void *retval;

  if(gthread_create(&tid1, thfunc1, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid1);

  while(th_priv_addr == 0)
    sleep(1);
  printf("_______\n%s\n_______\n", (char *)*(u64 *)&th_priv_addr);

  if(gthread_create(&tid2, thfunc2, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid2);

  gthread_join(tid1);
  printf("Thread %d returned\n", tid1);
  gthread_join(tid2);
  printf("Thread %d returned\n", tid2);

  th_priv_addr = 0;
  if(gthread_create(&tid1, thfunc4, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  while(!th_priv_addr)sleep(1);
  printf("Created thread: %d private addr = %x \n", tid1, th_priv_addr);

  for(ctr=0; ctr < 2; ++ctr){
        s_count[ctr] = ctr + 1;
        if(gthread_create(&tid[ctr], thfunc3, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
        }
  printf("Created thread: %d\n", tid[ctr]);
   }

  for(ctr=0; ctr<2; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread %d returned is %d\n", tid[ctr], (retval == 0 ? 0 : *((int *)retval)));
   }
  gthread_join(tid1);
  printf("Thread %d returned\n", tid1);

  return 0;
}

/*

GemOS# init
Setting up init process ...
Page table setup done, launching init ...
Created thread: 0
___
abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuv
___
Created thread: 1
abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuv
Thread 0 returned
Thread 1 returned
____ 69 0x180205000____
Created thread: 0 private addr = 0x180205000 
Created thread: 1
Created thread: 2
 Address inside other threads - 0x180205000
[pid 4]Arg is 69
[pid 4]Arg is 69
[pid 4]Arg is 69
Segfault for [4] @0x10000162B for address 0x180205000. Exiting
 Address inside other threads - 0x180205000
[pid 3]Arg is 169
[pid 3]Arg is 169
[pid 3]Arg is 169
[pid 3]Arg is 169
[pid 3]Arg is 169
[pid 3]Arg is 169
[pid 3]Arg is 169
[pid 3]Arg is 169
Thread 1 returned is 169
Thread 2 returned is 0
Thread 0 returned
Cleaned up init process
GemOS shell again!
GemOS# Connection closed by foreign host.

*/

