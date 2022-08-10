#include<ulib.h>

void do_end_handler()
{
	printf("In End Handler\n");
}

void fn_3()
{
  printf("In fn_3\n");
}

int fn_2()
{
  printf("In fn_2\n");
  return 4;
}


int fn_1(int x1, int x2, int x3, int x4, int x5)
{
	printf("In fn1 == %d %d %d %d %d\n", x1, x1 + x2, x3 + x4, x4, x5);
  fn_2();
  fn_3();
	return 0;
}


int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{


	printf("end_handler address: %x \n",do_end_handler);
  printf("Main address: %x \n", main);
  printf("fn_1 address: %x \n", fn_1);

	int cpid;
	long ret = 0;
	int i, rem1, rem2;
	struct registers regs;	// store registers info here

	ret = become_debugger(do_end_handler);

	cpid = fork();

	if(cpid < 0){
		printf("Error in fork\n");
	}
	else if(cpid == 0){
    printf("In Child main calling fn_1()\n");
		fn_1(2, 3, 5, 7, 11);
    printf("In Child main(). fn_1 finished\n");
	}
	else{
		ret = set_breakpoint(fn_1, 0);
    ret = set_breakpoint(fn_3, 0);

    printf("In Parent main(). Set Breakpoints.\n");

		// fn_1
		ret = wait_and_continue();

    printf("In Parent main(). Child in fn_1() beginning.\n");

		info_registers(&regs);

		printf("Registers:\n");
		printf("\t RIP: %x\n", regs.entry_rip);
    printf("\t RSP: %x\n", regs.entry_rsp);
    printf("\t RBP: %x\n", regs.rbp);
    printf("\t RAX: %x\n", regs.rax);
    printf("\t RDI: %x\n", regs.rdi);
    printf("\t RSI: %x\n", regs.rsi);
    // printf("\t RDX: %x\n", regs.rdx);
    printf("\t RCX: %x\n", regs.rcx);
    printf("\t R8: %x\n", regs.r8);
    // printf("\t RIP: %x\n", regs.r9);

    // fn_3
		ret = wait_and_continue();

    printf("In Parent main(). Child in fn_1() beginning.\n");

		info_registers(&regs);

		printf("Registers:\n");
		printf("\t RIP: %x\n", regs.entry_rip);
    printf("\t RSP: %x\n", regs.entry_rsp);
    printf("\t RBP: %x\n", regs.rbp);
    printf("\t RAX: %x\n", regs.rax);
    printf("\t RDI: %x\n", regs.rdi);
    printf("\t RSI: %x\n", regs.rsi);
    // printf("\t RDX: %x\n", regs.rdx);
    printf("\t RCX: %x\n", regs.rcx);
    printf("\t R8: %x\n", regs.r8);
    // printf("\t RIP: %x\n", regs.r9);

		// for exit
		ret = wait_and_continue();

    printf("In Parent main(). Child exited.\n");
	}

	return 0;
}
