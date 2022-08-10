#include "wc.h"

extern struct team teams[NUM_TEAMS];
extern int test;
extern int finalTeam1;
extern int finalTeam2;

int processType = HOST;
const char *team_names[] = {
  "India", "Australia", "New Zealand", "Sri Lanka",   // Group A
  "Pakistan", "South Africa", "England", "Bangladesh" // Group B
};

void error(){
	exit(-1);
}

int min(int a,int b){
	if(a<b)
		return a;
	return b;
}

void teamPlay(void)
{
	char fname[100];
	sprintf(fname,"./test/%d/inp/%s",test,team_names[processType]);

	int fd=open(fname, O_RDONLY);
	if(fd<0){
		perror("open 29");
		error();
	}

	char buff[2];
	buff[1]='\0';

	while(1){

		if(read(teams[processType].commpipe[0],buff,1)<=0){
			perror("read 40");
			error();
		}
		int val = atoi(buff);
		char b[2];
		b[1]='\0';
		if(val!=2){
			read(fd,b,1);
			if(write(teams[processType].matchpipe[1],b,1)<=0){
				perror("write 49");
				error();
			}

		}
		else{
			break;
		}
	}


}

void endTeam(int teamID)
{
	if(write(teams[teamID].commpipe[1],"2",1)<=0){
		perror("write 65");
		error();
	}
}

int match(int team1, int team2)
{
	char dig1[2],dig2[2];
	if(write(teams[team1].commpipe[1],"1",1) <=0 ){
		perror("write 74");
		error();
	}
	if(write(teams[team2].commpipe[1],"1",1)<=0){
		perror("write 78");
		error();
	}

	if(read(teams[team1].matchpipe[0],dig1,1)<0){
		perror("read 84");
		error();
	}
	if(read(teams[team2].matchpipe[0],dig2,1)<0){
		perror("read 87");
		error();
	}
	dig1[1]='\0';
	dig2[1]='\0';
	int i,j;

	int d1= atoi(dig1);
	int d2= atoi(dig2);

	int bat,ball;
	if((d1+d2)%2==1){
		bat=team1;
		ball=team2;
	}
	else{
		bat=team2;
		ball=team1;
	}
	char* fname = malloc(100);
	char* dir = malloc(100);
	sprintf(dir,"./test/%d/out/",test);
	mkdir(dir,0777);
	sprintf(fname,"./test/%d/out/%sv%s",test,team_names[bat],team_names[ball]);
	if((bat <= 3 && bat>=0 && ball>=4 && ball<=7) || (ball <= 3 && ball>=0 && bat>=4 && bat<=7) )
		strcat(fname,"-Final");
	int fd = open(fname, O_WRONLY | O_CREAT);
	if(fd<0){
		perror("open 114");
		error();
	}

	char* str = malloc(100);
	char to_write[100];

	sprintf(str,"Innings1: %s bats\n",team_names[bat]);
	write(fd,str,strlen(str));
	int score=0,wkt=0,tot=0;
	
	for ( i = 0; i < 20 && wkt<10; ++i)
	{
		for ( j = 0; j < 6 && wkt<10; ++j)
		{
			char digit1[2],digit2[2];
			write(teams[ball].commpipe[1],"1",1);
			write(teams[bat].commpipe[1],"1",1);
			if(read(teams[ball].matchpipe[0],digit2,1)<0){
				perror("read 134");
				error();
			}
			if(read(teams[bat].matchpipe[0],digit1,1)<=0){
				perror("read 138");
				error();
			}
			digit1[1]='\0';digit2[1]='\0';
			int a=atoi(digit1);
			int b=atoi(digit2);
			if(a!=b){
				score+=a;
				tot+=a;
			}
			else{
				wkt++;
				sprintf(to_write,"%d:%d\n",wkt,score);
				write(fd,to_write,strlen(to_write));
				score=0;
			}
		}
	}

	if(wkt < 10){
		memset(to_write , '\0', sizeof(to_write));
		sprintf(to_write,"%d:%d*\n",wkt+1,score);
		write(fd,to_write,strlen(to_write));
		score=0;
	}

	memset(to_write , '\0', sizeof(to_write));
	sprintf(to_write,"%s TOTAL: %d\n",team_names[bat],tot);
	write(fd,to_write,strlen(to_write));
	memset(to_write , '\0', sizeof(to_write));
	sprintf(to_write,"Innings2: %s bats\n",team_names[ball]);
	write(fd,to_write,strlen(to_write));

	score=0,wkt=0;
	int tot2=0;
	i=0,j=0;
	for ( i = 0; i < 20 && wkt<10 && tot2<=tot; ++i)
	{
		for ( j = 0; j < 6 && wkt<10 && tot2<=tot; ++j)
		{
			char digit1[2],digit2[2];
			write(teams[bat].commpipe[1],"1",1);
			write(teams[ball].commpipe[1],"1",1);
			if(read(teams[bat].matchpipe[0],digit2,1)<0){
				perror("read 182");
				error();
			}
			if(read(teams[ball].matchpipe[0],digit1,1)<0){
				perror("read 186");
				error();
			}
			digit1[1]='\0';digit2[1]='\0';
			int a=atoi(digit1);
			int b=atoi(digit2);
			if(a!=b){
				score+=a;
				tot2+=a;
				
			}
			else{
				wkt++;
				sprintf(to_write,"%d:%d\n",wkt,score);
				write(fd,to_write,strlen(to_write));
				score=0;
			}
		}
	}

	if(wkt < 10){
		memset(to_write , '\0', sizeof(to_write));
		sprintf(to_write,"%d:%d*\n",wkt+1,score);
		write(fd,to_write,strlen(to_write));
		score=0;
	}
	memset(to_write , '\0', sizeof(to_write));
	sprintf(to_write,"%s TOTAL: %d\n",team_names[ball],tot2);
	write(fd,to_write,strlen(to_write));

	int win=0, tie=0;
	if(tot2>tot){
		win=ball;
	}
	else if(tot2==tot){
		if(i<19 || (i==19 && j<5)){
			win=ball;
		}
		else{
			tie=1;
			win = team1;
		}
	}
	else{
		win=bat;
	}

	memset(to_write,'\0',sizeof(to_write));

	if(tie){
		sprintf(to_write,"TIE: %s beats %s\n",team_names[win],team_names[team1+team2-win]);
	}
	else{
		if(win==bat){
			sprintf(to_write,"%s beats %s by %d runs\n",team_names[bat],team_names[ball],tot-tot2);
		}
		else{
			sprintf(to_write,"%s beats %s by %d wickets\n",team_names[ball],team_names[bat],10-wkt);
		}
	}

	write(fd,to_write,strlen(to_write));

	return win;


}

void spawnTeams(void)
{
	for (int i = 0; i < 8; ++i)
	{
		strcpy(teams[i].name,team_names[i]);
		if(pipe(teams[i].commpipe) < 0){
			perror("pipe 260");
			error();
		}
		if(pipe(teams[i].matchpipe) < 0){
			perror("pipe 264");
			error();
		}

		int pid = fork();
		if(pid==0){
			processType=i;
			close(teams[i].matchpipe[0]);
			close(teams[i].commpipe[1]);
			teamPlay();
			exit(1);
		}
		else{
			close(teams[i].matchpipe[1]);
			close(teams[i].commpipe[0]);
		}
	}
		
}

void conductGroupMatches(void)
{

	int pipe1[2],pipe2[2];
	int p1,p2;

	if (pipe(pipe1) < 0)
	{
		perror("pipe 291");
		error();
	}

	p1=fork();

	if(p1==0){
		int winner[5];
		close(pipe1[0]);
		memset(winner,0,sizeof(winner));
		for (int i = 1; i <= 4; ++i)
		{
			for (int j = i+1; j <= 4; ++j)
			{
				winner[match(i-1,j-1)+1]++;
			}
		}
		int mx=0;

		for (int i = 1; i <= 4; ++i)
		{
			if(winner[i] > mx){
				mx=winner[i];
				finalTeam1=i-1;
			}
		}

		for (int i = 1; i < 5; ++i)
		{
			if((i-1)!=finalTeam1)
				endTeam(i-1);
		}
		char* str = malloc(2);
		sprintf( str, "%d", finalTeam1 );
		if(write(pipe1[1],str,1)<0){
			perror("write 327");
			error();
		}
		exit(0);
	}

	close(pipe1[1]);

	if (pipe(pipe2) < 0)
	{
		perror("pipe 336");
		error();
	}	

	p2=fork();
	if(p2==0){
		int winner[5];
		close(pipe2[0]);
		memset(winner,0,sizeof(winner));
		for (int i = 5; i <= 8; ++i)
		{
			for (int j = i+1; j <= 8; ++j)
			{
				winner[match(i-1,j-1)-3]++;
			}
		}
		
		int mx=0;

		for (int i = 5; i <= 8; ++i)
		{
			if(winner[i-4] > mx){
				mx=winner[i-4];
				finalTeam2=i-1;
			}
		}

		for (int i = 5; i <= 8; ++i)
		{
			if((i-1)!=finalTeam2)
				endTeam(i-1);
		}
		char* str = malloc(2);
		sprintf( str, "%d", finalTeam2 );
		if(write(pipe2[1],str,1)<0){
			perror("write 372");
			error();
		}
		exit(0);
	}
	close(pipe2[1]);
	int status;
	waitpid(p1,&status,0);
	waitpid(p2,&status,0);

	char fin1[2],fin2[2];
	if(read(pipe1[0],fin1,1) <= 0){
		perror("read 385");
		error();
	}
	pipe1[1]='\0';
	if(read(pipe2[0],fin2,1) <= 0){
		perror("read 389");
		error();
	}
	pipe2[1]='\0';
	finalTeam1 = atoi(fin1);
	finalTeam2 = atoi(fin2);

}
