#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
unsigned long long string_to_integer(char* st){
	int i=0;
	unsigned long long ans=0;
	while(st[i]!='\0'){
		ans=ans*10+(st[i++]-'0');
	}

	return ans;
}
int main(int argc,char *argv[])
{
	unsigned long long num=string_to_integer(argv[argc-1]);
	unsigned long long ans=num*num;
	if(argc==2){
		printf("%llu\n", ans);
	}

	else{
		unsigned long long temp=ans;
		int length = 0;
		while(temp){
			length++;
			temp/=10;
		}
		if(ans==0) length = 1;
		char* str = malloc( length + 1 );
		snprintf( str, length + 1, "%llu", ans );
		argv[argc-1]=str;
		if(execv(argv[1],argv+1)){
			perror("UNABLE TO EXECUTE");
		}
	}



	return 0;
}
