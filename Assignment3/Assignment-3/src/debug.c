#include <debug.h>
#include <context.h>
#include <entry.h>
#include <lib.h>
#include <memory.h>


/*****************************HELPERS******************************************/

/*
 * allocate the struct which contains information about debugger
 *
 */
struct debug_info *alloc_debug_info()
{
	struct debug_info *info = (struct debug_info *) os_alloc(sizeof(struct debug_info));
	if(info)
		bzero((char *)info, sizeof(struct debug_info));
	return info;
}
/*
 * frees a debug_info struct
 */
void free_debug_info(struct debug_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct debug_info));
}



/*
 * allocates a page to store registers structure
 */
struct registers *alloc_regs()
{
	struct registers *info = (struct registers*) os_alloc(sizeof(struct registers));
	if(info)
		bzero((char *)info, sizeof(struct registers));
	return info;
}

/*
 * frees an allocated registers struct
 */
void free_regs(struct registers *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct registers));
}

/*
 * allocate a node for breakpoint list
 * which contains information about breakpoint
 */
struct breakpoint_info *alloc_breakpoint_info()
{
	struct breakpoint_info *info = (struct breakpoint_info *)os_alloc(
		sizeof(struct breakpoint_info));
	if(info)
		bzero((char *)info, sizeof(struct breakpoint_info));
	return info;
}

/*
 * frees a node of breakpoint list
 */
void free_breakpoint_info(struct breakpoint_info *ptr)
{
	if(ptr)
		os_free((void *)ptr, sizeof(struct breakpoint_info));
}

/*
 * Fork handler.
 * The child context doesnt need the debug info
 * Set it to NULL
 * The child must go to sleep( ie move to WAIT state)
 * It will be made ready when the debugger calls wait
 */
void debugger_on_fork(struct exec_context *child_ctx)
{
	// printk("DEBUGGER FORK HANDLER CALLED\n");
	child_ctx->dbg = NULL;
	child_ctx->state = WAITING;
}


/******************************************************************************/


void copy_stack(struct exec_context* parent, struct exec_context* child,int hit_from_end) {
	int cnt=0;
	
	if(!hit_from_end)
		parent->dbg->trace[cnt++] = child->regs.entry_rip-1;

	int idx = parent->dbg->ret_size-1;
	u64 base_pointer=child->regs.rbp, it=child->regs.entry_rsp;
	while(*(u64 *)it != END_ADDR) {
		// Use original return address instead of the end_handler
		if(*(u64 *)it == ( u64 )parent->dbg->end_handler)
			parent->dbg->trace[cnt++] = parent->dbg->return_addr[idx--];
		else
			parent->dbg->trace[cnt++] = *(u64 *) it;
		it = base_pointer+8;
		base_pointer = *(u64 *)base_pointer; // Jump to old rbp in stack
	} 
	parent->dbg->trace_size = cnt;
	return;

}

void copy_regs(struct exec_context* parent, struct exec_context* child,int val){
	struct registers parent_register;
	parent_register.r15 = child->regs.r15;
	parent_register.r14 = child->regs.r14;
	parent_register.r13 = child->regs.r13;
	parent_register.r12 = child->regs.r12;
	parent_register.r11 = child->regs.r11;
	parent_register.r10 = child->regs.r10;
	parent_register.r9 = child->regs.r9;
	parent_register.r8 = child->regs.r8;
	parent_register.rbp = child->regs.rbp;
	parent_register.rdi = child->regs.rdi;
	parent_register.rsi = child->regs.rsi;
	parent_register.rdx = child->regs.rdx;
	parent_register.rcx = child->regs.rcx;
	parent_register.rbx = child->regs.rbx;
	parent_register.rax = child->regs.rax;
	parent_register.entry_rip = child->regs.entry_rip-1;
	parent_register.entry_cs = child->regs.entry_cs;
	parent_register.entry_rflags = child->regs.entry_rflags;
	parent_register.entry_rsp = child->regs.entry_rsp - val;
	parent_register.entry_ss = child->regs.entry_ss;

	parent->dbg->regs = parent_register;

	return;

}

/* This is the int 0x3 handler
 * Hit from the childs context
 */
long int3_handler(struct exec_context *ctx)
{
	if(!ctx)
		return -1;
	struct exec_context* parent = get_ctx_by_pid(ctx->ppid);
	u64 addr = ctx->regs.entry_rip - 1;
	parent->regs.rax = addr; // Return value for wait and continue into RAX.
	// Entering from end handler if entry_rip-1==end_handler
	if(((u64) parent->dbg->end_handler == addr)){
		// Modify return address back to original caller, pop the stack, simulate push rbp
		copy_regs(parent,ctx,8);
		parent->dbg->stack_size--;
		parent->dbg->stack[parent->dbg->stack_size] = 0;
		ctx->regs.entry_rsp-=8;
		parent->dbg->ret_size--;
		*(u64 *)(ctx->regs.entry_rsp) = parent->dbg->return_addr[parent->dbg->ret_size];
		parent->dbg->return_addr[parent->dbg->ret_size] = 0;
		copy_stack(parent,ctx,1);
		ctx->regs.entry_rsp -=8;
		u64 stack_top = ctx->regs.entry_rsp;
		*(u64 *) stack_top = ctx->regs.rbp;	
	}
	else{
		copy_regs(parent,ctx,0);
		int eb_enable=0;
		struct breakpoint_info* it = parent->dbg->head;
		while(it){
			if(it->addr == addr){
				eb_enable = it->end_breakpoint_enable;
				break;
			}
			it=it->next;
		}
		copy_stack(parent,ctx,0);
		if(eb_enable){
			// Modify return address and store in own stack array (consists only of ebp fns)
			u64 st = ctx->regs.entry_rsp;
			parent->dbg->stack[parent->dbg->stack_size] = addr;
			parent->dbg->stack_size++;
			parent->dbg->return_addr[parent->dbg->ret_size]= *(u64 *)(ctx->regs.entry_rsp);
			parent->dbg->ret_size++;
			*(u64 *)(st) = (u64)(parent->dbg->end_handler);
		}
		// Push rbp simulated
		ctx->regs.entry_rsp -=8;
		*(u64 *) (ctx->regs.entry_rsp) = ctx->regs.rbp;
	}

	ctx->state = WAITING;
	parent->state = READY;
	schedule(parent);

	return 0;
}

/*
 * Exit handler.
 * Deallocate the debug_info struct if its a debugger.
 * Wake up the debugger if its a child
 */
void debugger_on_exit(struct exec_context *ctx)
{
	if(!ctx)
		return;
	if(!ctx->dbg){
		struct exec_context* par = get_ctx_by_pid(ctx->ppid);
		par->state = READY;
	}
	else{
		struct breakpoint_info *curr = ctx->dbg->head;
		while(curr){
			struct breakpoint_info *nxt = curr->next;
			curr->next=NULL;
			free_breakpoint_info(curr);
			curr=nxt;
		}
		free_debug_info(ctx->dbg);
		ctx->dbg=NULL;
	}

	return;
}


/*
 * called from debuggers context
 * initializes debugger state
 */
int do_become_debugger(struct exec_context *ctx, void *addr)
{
	if(!ctx)
		return -1;
	struct debug_info* dbg = ctx->dbg;

	dbg = alloc_debug_info();
	if(!dbg)
		return -1;
	*(unsigned char*) addr = INT3_OPCODE; // Setting address to raise interrupt.

	dbg->head = NULL;
	dbg->breakpoint_num = 0;
	dbg->end_handler = addr;
	dbg->breakpoint_count = 0;
	dbg->trace_size= 0;
	dbg->stack_size= 0;
	dbg->ret_size= 0;
	ctx->dbg = dbg;


	return 0;
}

/*
 * called from debuggers context
 */
int do_set_breakpoint(struct exec_context *ctx, void *addr, int flag)
{
	if(!ctx || !(ctx->dbg))
		return -1;
	struct debug_info* dbg = ctx->dbg;
	struct breakpoint_info* it = ctx->dbg->head;
	int breakpoint_set = 0;
	int on_stack = 0;
	for (int i = 0; i < ctx->dbg->stack_size; ++i)
	{	
		if(ctx->dbg->stack[i] == (u64)addr){
			on_stack=1;
			break;
		}
	}
	while(it){
		if(it->addr == (u64)addr){
			breakpoint_set=1;
			break;
		}
		it=it->next;
	}
	if(breakpoint_set && on_stack){
		// When on stack, can't set flag from one to zero.
		if(it->end_breakpoint_enable ==1 && flag==0)
			return -1;
	}
	if(breakpoint_set){
		it->end_breakpoint_enable = flag;
		return 0;
	}

	if(dbg->breakpoint_count >= MAX_BREAKPOINTS)
		return -1;
	struct breakpoint_info *curr = alloc_breakpoint_info();
	if(!curr)
		return -1;

	dbg->breakpoint_num++;
	dbg->breakpoint_count++;
	curr->num = dbg->breakpoint_num;
	curr->addr = (u64) addr;
	curr->end_breakpoint_enable = flag;
	curr->next = NULL;
	*(unsigned char*)addr = INT3_OPCODE;
	if(dbg->head==NULL)
		dbg->head=curr;
	else{
		it=dbg->head;
		while(it->next){
			it=it->next;
		}
		it->next=curr;
	}


	return 0;
}

/*
 * called from debuggers context
 */


int do_remove_breakpoint(struct exec_context *ctx, void *addr)
{
	if(!ctx || !(ctx->dbg))
	  return -1;
	struct breakpoint_info *it = ctx->dbg->head;
	struct breakpoint_info* it2 = ctx->dbg->head;

	int breakpoint_set = 0;
	while(it){
		if(it->addr == (u64)addr){
			breakpoint_set = 1;
			break;
		}
		it=it->next;
	}

	if(!breakpoint_set)
		return -1;
	int on_stack=0;
	for (int i = 0; i < ctx->dbg->stack_size; ++i)
	{
		if(ctx->dbg->stack[i] == (u64)addr){
			on_stack=1;
			break;
		}	
	}

	if(on_stack){
		if(it->end_breakpoint_enable)
			return -1;
	}
	*(unsigned char *) addr = PUSHRBP_OPCODE;
	ctx->dbg->breakpoint_count--;
	if(it == ctx->dbg->head){
		ctx->dbg->head=it->next;
		it->next=NULL;
	}
	else{
		while(it2 && it2->next!=it){
			it2=it2->next;
		}
		it2->next = it->next;
		it->next=NULL;
	}

	free_breakpoint_info(it);

	return 0;
}


/*
 * called from debuggers context
 */

int do_info_breakpoints(struct exec_context *ctx, struct breakpoint *ubp)
{
	if(!ctx || !(ctx->dbg))
		return -1;
	int cnt=0;
	struct breakpoint_info *it=ctx->dbg->head;
	struct breakpoint current;
	while(it){
		current.addr = it->addr;
		current.num = it->num;
		current.end_breakpoint_enable = it->end_breakpoint_enable;
		ubp[cnt++] = current;
		it = it->next;
	}

	return cnt;
}


/*
 * called from debuggers context
 */
int do_info_registers(struct exec_context *ctx, struct registers *regs)
{
	if(!ctx || !ctx->dbg)
		return -1;
	*regs = ctx->dbg->regs;
	return 0;
}

/*
 * Called from debuggers context
 */
int do_backtrace(struct exec_context *ctx, u64 bt_buf)
{
	if(!ctx || !(ctx->dbg))
		return -1;
	u64* arr = (u64*) bt_buf;
	for(int i=0;i<ctx->dbg->trace_size;i++) {
		arr[i] = ctx->dbg->trace[i];
	}
	return ctx->dbg->trace_size;

}

/*
 * When the debugger calls wait
 * it must move to WAITING state
 * and its child must move to READY state
 */

s64 do_wait_and_continue(struct exec_context *ctx)
{
	if(!ctx || !(ctx->dbg))
		return -1;
	struct exec_context *child = NULL;
	for (int i = 0; i < MAX_PROCESSES; ++i)
	{
		child = get_ctx_by_pid(i);
		if(child!=NULL && child-> ppid == ctx -> pid) break;
	}

	// Child does not exist
	if(!child || child->ppid != ctx -> pid)
		return CHILD_EXIT;
	ctx->state = WAITING;
	child->state = READY;

	schedule(child);
	return 0;
}






