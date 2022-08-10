#include<ulib.h>
void do_end_handler()
{
	printf("In End Handler\n");
}



int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{

  long ret = 0;

  ret = become_debugger(do_end_handler);
  int a = test(1, do_end_handler);

  if(a != 0)
	  printf("BECOME DEBUGGER FAILED\n");
  else
	  printf("BECOME DEBUGGER SUCCESS\n");
  return 0;
}
