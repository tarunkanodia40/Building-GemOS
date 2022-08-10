#include <gthread.h>
#include <ulib.h>

static struct process_thread_info tinfo __attribute__((section(".user_data"))) = {};
/*XXX 
      Do not modifiy anything above this line. The global variable tinfo maintains user
      level accounting of threads. Refer gthread.h for definition of related structs.
 */


/* Returns 0 on success and -1 on failure */
/* Here you can use helper system call "make_thread_ready" for your implementation */

void func_exit(){
	void* retval;
	asm volatile(
		"mov %%rax, %0;"
		: "=r"(retval)
		:
		: "memory");
	gthread_exit(retval);
}

int gthread_create(int *tid, void *(*fc)(void *), void *arg) {
  if(tinfo.num_threads==MAX_THREADS)
  	return -1;
	void *stackp = mmap(NULL, TH_STACK_SIZE, PROT_READ|PROT_WRITE, 0);
	if(!stackp || stackp == MAP_ERR){
    return -1;
 	}
	void* base = ((u64)stackp) + TH_STACK_SIZE;
	base-=8;
	*(u64 *)base = &func_exit;
	int pid = clone(fc,(u64)base,arg);
	if(pid==-1)
		return -1;
	int idx=-1;
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		if(tinfo.threads[i].status == 0){
			idx=i;
			break;
		}
	}

	if(idx==-1)
		return -1;
	tinfo.threads[idx].pid=pid;
	tinfo.threads[idx].tid=idx;
	*tid = idx;
	tinfo.threads[idx].status=1;
	tinfo.threads[idx].stack_addr=base;
	tinfo.num_threads++;

	make_thread_ready(pid);

	return 0;

}

int gthread_exit(void *retval) {

	int pid = getpid();
	int idx=-1;
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		if(tinfo.threads[i].pid == pid && tinfo.threads[i].status!=2){
			idx=i;
			break;
		}
	}
	if(idx==-1){
		exit(0);
	}
	tinfo.threads[idx].ret_addr = retval;
  tinfo.threads[idx].status = 2;

	exit(0);
}

void* gthread_join(int tid) {
        
	int idx=tid;
  if(idx<0 || idx>MAX_THREADS)
    return NULL;
  int val = 0;
  while(tinfo.threads[idx].status!= 2 && val >= 0 ){
    val = wait_for_thread(tinfo.threads[idx].pid);
  }
  void* retval = tinfo.threads[idx].ret_addr;
  tinfo.num_threads--;
	tinfo.threads[idx].status=0;
	tinfo.threads[idx].pid=-1;
	tinfo.threads[idx].tid=-1;
	tinfo.threads[idx].ret_addr=NULL;
  munmap(tinfo.threads[idx].stack_addr - TH_STACK_SIZE + 8,TH_STACK_SIZE);
  for (int i = 0; i < MAX_GALLOC_AREAS; ++i)
  {
  	if(tinfo.threads[idx].priv_areas[i].owner)
    	gfree((void*)tinfo.threads[idx].priv_areas[i].start);
  }
    
	return retval;
}


/*Only threads will invoke this. No need to check if its a process
 * The allocation size is always < GALLOC_MAX and flags can be one
 * of the alloc flags (GALLOC_*) defined in gthread.h. Need to 
 * invoke mmap using the proper protection flags (for prot param to mmap)
 * and MAP_TH_PRIVATE as the flag param of mmap. The mmap call will be 
 * handled by handle_thread_private_map in the OS.
 * */

void* gmalloc(u32 size, u8 alloc_flag)
{
   u64 arg = PROT_READ | PROT_WRITE;
   if (alloc_flag == GALLOC_OWNONLY) arg |= TP_SIBLINGS_NOACCESS;
   else if(alloc_flag == GALLOC_OTRDONLY) arg |= TP_SIBLINGS_RDONLY;
   else if(alloc_flag == GALLOC_OTRDWR) arg |= TP_SIBLINGS_RDWR;
   else return NULL;
   int pid = getpid();
   int idx=-1;
   for (int i = 0; i < MAX_THREADS; ++i)
   {
   	if(tinfo.threads[i].pid == pid && tinfo.threads[i].status!=2){
   		idx=i;
   		break;
   	}

   }
   if(idx==-1)
   	return NULL;
   void* addr = mmap(NULL,size,arg,MAP_TH_PRIVATE);
   if(!addr || addr == MAP_ERR){
    return NULL;
  }
   int mem=-1;
   for (int i = 0; i < MAX_GALLOC_AREAS; ++i)
   {
	if(!tinfo.threads[idx].priv_areas[i].owner){
		mem=i;
		break;
	}   		
   }
   if(mem==-1){
   	return NULL;
   }

   tinfo.threads[idx].priv_areas[mem].owner = &tinfo.threads[idx];
   tinfo.threads[idx].priv_areas[mem].start = (u64)addr;
   tinfo.threads[idx].priv_areas[mem].length = size;
   tinfo.threads[idx].priv_areas[mem].flags = arg;

   return addr;

}
/*
   Only threads will invoke this. No need to check if the caller is a process.
*/
int gfree(void *ptr)
{
   int pid = getpid();
   int idx=-1;
   for (int i = 0; i < MAX_THREADS; ++i)
   {
   	if(tinfo.threads[i].pid == pid && tinfo.threads[i].status!=2){
   		idx=i;
   		break;
   	}
   }
   if(idx==-1)
   	return -1;  

   for (int i = 0; i < MAX_GALLOC_AREAS; ++i)
   {
   	if(tinfo.threads[idx].priv_areas[i].start == (u64)ptr){
   		int val = munmap(ptr,tinfo.threads[idx].priv_areas[i].length);
      if(val<0)
        return -1;
   		tinfo.threads[idx].priv_areas[i].owner=NULL;
   		tinfo.threads[idx].priv_areas[i].start=-1;
   		tinfo.threads[idx].priv_areas[i].length=0;
   		tinfo.threads[idx].priv_areas[i].flags=0;
   		return 0;
   	}
   }

 	return -1;
}
