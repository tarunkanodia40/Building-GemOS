#include<ulib.h>

void do_end_handler()
{
	printf("In End Handler\n");
}

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{
  long ret = 0;
  int cpid;

  ret = become_debugger(do_end_handler);

	cpid = fork();

  if (cpid < 0){
		printf("Error in fork\n");
	} else if (cpid == 0){
		printf("In Child\n");
	} else {
    ret = wait_and_continue();
    printf("In Parent main(). Child exited. %d\n", (int)ret);
  }

  return 0;
}
