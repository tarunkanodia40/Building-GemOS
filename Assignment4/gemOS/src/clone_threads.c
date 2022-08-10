#include<clone_threads.h>
#include<entry.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<mmap.h>


u64 pt_walk(struct exec_context *ctx, u64 addr,int remove,int from_switch,int comp_off) {

  u64 *base = (u64 *)osmap(ctx->pgd);
  u64 *entry;
  u64 pfn;
  
  entry = base + ((addr & PGD_MASK) >> PGD_SHIFT);
  if(*entry & 0x1) {
    *entry = (*entry) | 0x7;
    pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
    base = (u64 *)osmap(pfn);
  }else{
    if(from_switch)
      return 0;
    pfn = os_pfn_alloc(OS_PT_REG);
    *entry = (pfn << PTE_SHIFT) | 0x7;
    base = osmap(pfn);
  }
  
  entry = base + ((addr & PUD_MASK) >> PUD_SHIFT);
  if(*entry & 0x1) {
    *entry = (*entry) | 0x7;
    pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
    base = (u64 *)osmap(pfn);
  }else{
    if(from_switch)
      return 0;
    pfn = os_pfn_alloc(OS_PT_REG);
    *entry = (pfn << PTE_SHIFT) | 0x7;
    base = osmap(pfn);
  }

  entry = base + ((addr & PMD_MASK) >> PMD_SHIFT);
  if(*entry & 0x1) {
    *entry = (*entry) | 0x7;
    pfn = (*entry >> PTE_SHIFT) & 0xFFFFFFFF;
    base = (u64 *)osmap(pfn);
  }else{
    if(from_switch)
      return 0;
    pfn = os_pfn_alloc(OS_PT_REG);
    *entry = (pfn << PTE_SHIFT) | 0x7;
    base = osmap(pfn);
  }

  entry = base + ((addr & PTE_MASK) >> PTE_SHIFT);
  if(*entry & 0x1) {
    if(!comp_off){
      *entry = (*entry) | 0x7;
       if(remove)
        *entry = *entry & (~0x2);
    }
    else{
      *entry = (*entry) & (~0x6);
    }
  }else{
    if(from_switch)
      return 0;
    pfn = os_pfn_alloc(USER_REG);
    *entry = (pfn << PTE_SHIFT) | 0x7;
    if(remove)
        *entry = *entry & (~0x2);
  }
  return 0;
}


long do_clone(void *th_func, void *user_stack, void *user_arg) 
{
  
  struct exec_context *new_ctx = get_new_ctx();
  struct exec_context *ctx = get_current_ctx();

  u32 pid = new_ctx->pid;
  
  if(!ctx->ctx_threads){  // This is the first thread
          ctx->ctx_threads = os_alloc(sizeof(struct ctx_thread_info));
          bzero((char *)ctx->ctx_threads, sizeof(struct ctx_thread_info));
          ctx->ctx_threads->pid = ctx->pid;
  }
     
 /* XXX Do not change anything above. Your implementation goes here*/
  
  
  // allocate page for os stack in kernel part of process's VAS
  // The following two lines should be there. The order can be 
  // decided depending on your logic.
   *new_ctx = *ctx;
   new_ctx->pid = pid;
   new_ctx->ppid = ctx->pid;
   new_ctx->type = EXEC_CTX_USER_TH;    // Make sure the context type is thread
   int idx=-1;
   for (int i = 0; i < MAX_THREADS; ++i)
   {
     if(ctx->ctx_threads->threads[i].status== TH_UNUSED){
      idx=i;
      break;
     }
   }
   if(idx==-1)
    return -1;
   ctx->ctx_threads->threads[idx].status = TH_USED;
   ctx->ctx_threads->threads[idx].pid = pid;
   ctx->ctx_threads->threads[idx].parent_ctx = ctx;
   new_ctx->ctx_threads = NULL;
   new_ctx->regs.entry_rip = (u64)th_func ;
   new_ctx->regs.entry_rsp = (u64)user_stack;
   new_ctx->regs.rdi= (u64)user_arg;

   setup_child_context(new_ctx);
  
   new_ctx->state = WAITING;         

	
  return pid;

}

/*This is the page fault handler for thread private memory area (allocated using 
 * gmalloc from user space). This should fix the fault as per the rules. If the the 
 * access is legal, the fault handler should fix it and return 1. Otherwise it should
 * invoke segfault_exit and return -1*/

int handle_thread_private_fault(struct exec_context *current, u64 addr, int error_code)
{
  struct exec_context* parent = NULL;
    if(isThread(current)){
      parent = get_ctx_by_pid(current->ppid);
    }
    else{
      parent=current;
    }
    if(error_code == 0x7)
      goto label;
    struct ctx_thread_info *info = parent->ctx_threads;
    int own = -1,map=-1;

    for (int i = 0; i < MAX_THREADS; ++i)
    {
      if(info->threads[i].status == TH_UNUSED) continue;
      for (int j = 0; j < MAX_PRIVATE_AREAS; ++j)
      {
        if(!info->threads[i].private_mappings[j].owner) continue;
        u64 start = info->threads[i].private_mappings[j].start_addr;
        if(addr >= start && addr < start + info->threads[i].private_mappings[j].length){
          own=i;
          map=j;
          break;
        }
      }
    }
    if(own == -1 || map==-1)
      goto label;

    if(!isThread(current) || current->pid == info->threads[own].pid || info->threads[own].private_mappings[map].flags & TP_SIBLINGS_RDWR ){
      pt_walk(current,addr,0,0,0);
      asm volatile (
        "invlpg (%0);" 
        :: "r"(addr) 
        : "memory"
        );
      return 1;
    }
    else if((info->threads[own].private_mappings[map].flags & TP_SIBLINGS_RDONLY) && (error_code & 0x2 == 0)){
      pt_walk(current,addr,1,0,0);
      asm volatile (
        "invlpg (%0);" 
        :: "r"(addr) 
        : "memory"
        );
      return 1;
    }
    else
      goto label;
    label:
      segfault_exit(current->pid, current->regs.entry_rip, addr);
    return -1;
}

/*This is a handler called from scheduler. The 'current' refers to the outgoing context and the 'next' 
 * is the incoming context. Both of them can be either the parent process or one of the threads, but only
 * one of them can be the process (as we are having a system with a single user process). This handler
 * should apply the mapping rules passed in the gmalloc calls. */

int handle_private_ctxswitch(struct exec_context *current, struct exec_context *next)
{
  struct exec_context *parent = NULL;
  if(!isThread(current)){
    parent=current;
  }
  else
    parent = get_ctx_by_pid(current->ppid);

  struct ctx_thread_info *info = parent->ctx_threads;

  for (int i = 0; i < MAX_THREADS; ++i)
  {
    if(info->threads[i].status==TH_UNUSED) continue;
    for (int j = 0; j < MAX_PRIVATE_AREAS; ++j)
    {
      if(!info->threads[i].private_mappings[j].owner) continue;
      u64 start = info->threads[i].private_mappings[j].start_addr;
      u32 length = info->threads[i].private_mappings[j].length;
      u32 flags = info->threads[i].private_mappings[j].flags;
      u64 val = length/PAGE_SIZE;
      for (int k = 0; k < val; ++k)
      {
        u64 addr = start + (u64)k*PAGE_SIZE; 
        if(next == parent){
          pt_walk(next,addr,0,1,0);
        }
        else{
          if(info->threads[i].pid == next->pid){
            pt_walk(next,addr,0,1,0);
          }
          else{
            if(flags & TP_SIBLINGS_RDWR){
              pt_walk(next,addr,0,1,0);
            }
            else if(flags & TP_SIBLINGS_RDONLY){
              pt_walk(next,addr,1,1,0);
            }
            else{
              pt_walk(next,addr,0,1,1);
            }
          }
        }
        asm volatile (
        "invlpg (%0);" 
        :: "r"(addr) 
        : "memory"
        );
      }
    }
  }



  return 0;	

}
