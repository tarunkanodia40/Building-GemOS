#include<ulib.h>
#include<gthread.h>


static void *thfunc1(void *arg){

   int *a = (int *)arg;
              *a = 1; 
            return arg;
}

static void *thfunc2(void *arg){
	
     // printf("excuted func2 by thread 2");
     int *b = (int *)arg;
              *b = 2; 
              sleep(10);
            return arg;
}
static void *thfunc3(void *arg){
	
     // printf("excuted func2 by thread 2");
     int *b = (int *)arg;
              *b = 3; 
              sleep(10);
            return arg;
}static void *thfunc4(void *arg){
	
     // printf("excuted func2 by thread 2");
     int *b = (int *)arg;
              *b = 4; 
              sleep(10);
            return arg;
}static void *thfunc5(void *arg){
	
     // printf("excuted func2 by thread 2");
     int *b = (int *)arg;
              *b = 5; 
              sleep(10);
            return arg;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{ 
   int s_count[8];
  int tid[8];
  int ctr;
  void *retval;
  //tc1
  // create one thread. and join should be success (pthread_exit)
  for(ctr=0; ctr<4; ++ctr){
        s_count[ctr] = ctr + 1;
        if(gthread_create(&tid[ctr], thfunc1, &s_count[ctr]) < 0)
        {
             printf("gthread_create failed\n");
             exit(-1);
        }
	printf("Created thread: %d\n", tid[ctr]);
   }        
  for(ctr=0; ctr<4; ++ctr){
        retval = gthread_join(tid[ctr]);
        printf("Thread %d returned is %d\n", ctr , *((int *)retval));
   }

     return 0;
}


/*

GemOS# init
Setting up init process ...
Page table setup done, launching init ...
Created thread: 0
Created thread: 1
Created thread: 2
Created thread: 3
Thread 0 returned is 1
Thread 1 returned is 1
Thread 2 returned is 1
Thread 3 returned is 1
Cleaned up init process
GemOS shell again!
GemOS# Connection closed by foreign host.

*/