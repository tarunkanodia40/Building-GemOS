#include<ulib.h>
#include<gthread.h>

/*XXX   Do not declare global or static variables in the test cases*/

/*Thread functions must be declared as static*/
static void *thfunc1(void *arg){
	    int ctr;
            u64 *th_priv_ptr = (u64 *) arg;
	    char *ptr = (char *)gmalloc(8192, GALLOC_OWNONLY);  // Allocate and place the pointer
                                                        // in thread argument i.e., &th_priv_addr 
            for(ctr=0; ctr < 100; ++ctr){
                   ptr[ctr] = 'a' + (ctr % 26);
            } 
	    ptr[ctr] = 0;
	    *th_priv_ptr = (u64) ptr;
	    sleep(100);    // Need to sleep for other thread to finish
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


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
  u64 th_priv_addr = 0; 	
  int tid1, tid2, tid3, tid4, tid5, tid6;
  if(gthread_create(&tid1, thfunc1, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid1);

  if(gthread_create(&tid2, thfunc2, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid2);

  if(gthread_create(&tid3, thfunc2, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid3);

  if(gthread_create(&tid4, thfunc2, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid4);

  gthread_join(tid2);
  printf("Thread %d returned\n", tid2);
  gthread_join(tid3);
  printf("Thread %d returned\n", tid3);
  gthread_join(tid4);
  printf("Thread %d returned\n", tid4);

  if(gthread_create(&tid5, thfunc2, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid5);

  if(gthread_create(&tid6, thfunc2, (void *)&th_priv_addr) < 0){
             printf("gthread_create failed\n");
             exit(-1);
  }
  printf("Created thread: %d\n", tid6);
  
  gthread_join(tid5);
  printf("Thread %d returned\n", tid5);
  gthread_join(tid6);
  printf("Thread %d returned\n", tid6);
  gthread_join(tid1);
  printf("Thread %d returned\n", tid1);

  return 0;
}

/*

GemOS# init
Setting up init process ...
Page table setup done, launching init ...
Created thread: 0
Created thread: 1
Segfault for [3] @0x100000A37 for address 0x18020D000. Exiting
Created thread: 2
Created thread: 3
Segfault for [4] @0x100000A37 for address 0x18020D000. Exiting
Segfault for [5] @0x100000A37 for address 0x18020D000. Exiting
Thread 1 returned
Thread 2 returned
Thread 3 returned
Created thread: 1
Created thread: 2
Segfault for [3] @0x100000A37 for address 0x18020D000. Exiting
Thread 1 returned
Segfault for [4] @0x100000A37 for address 0x18020D000. Exiting
Thread 2 returned
Thread 0 returned
Cleaned up init process
GemOS shell again!
GemOS# Connection closed by foreign host.

*/