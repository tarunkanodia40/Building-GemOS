#include<ulib.h>

void do_end_handler()
{
	printf("In End Handler\n");
}

int fn_1(int a)
{
	printf("actually inside fn_1 %d st/nd time\n", (4 - a));
a--;
if( a > 0) {
  fn_1(a);
}
	if(a == 3)
		printf("exiting child now \n");
	return 0;
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{

  int cpid, btsize;
  long ret = 0;
  unsigned long btbuf[8];

  ret = become_debugger(do_end_handler);

  cpid = fork();

  if (cpid < 0) {
    printf("Error in fork\n");
  } else if (cpid == 0) {
		printf("In Child main calling fn_1()\n");
		fn_1(3);
	} else {
    ret = set_breakpoint(fn_1, 0);

    printf("In Parent main(). Set Breakpoints.\n");

    // fn_1 entry
    ret = wait_and_continue();

    printf("In Parent main(). Child in fn_1() beginning.\n");

   ret = wait_and_continue();

    printf("In Parent main(). Child in first fn_1() beginning.\n");

   ret = wait_and_continue();

    printf("In Parent main(). Child in second fn_1() beginning.\n");

   ret = wait_and_continue();

    printf("In Parent main(). Child in third fn_1() beginning.\n");
    printf("fn_1 address : %x \n ", fn_1);
    btsize = backtrace((void *)btbuf);
    for (int i = 0; i < btsize; ++i) {
     printf("BackTrace %d / %d : %x\n", i, btsize, btbuf[i]);
    }
    // child exit
    ret = wait_and_continue();
    printf("In Parent main(). Child exited.\n");
  }

  return 0;
}
