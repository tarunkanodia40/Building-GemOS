#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int min(int a,int b){
	if(a<b) return a;
	return b;
}

int main(int argc,char* argv[])
{
	
	char* slash = malloc(2);
	slash[0]='/';
	slash[1]='\0';

	char* space = malloc(2);
	char* newline = malloc(2);

	space[0]=' ';
	newline[0]='\n';
	space[1]=newline[1]='\0';
	

	int chunk_size = 2048;
	if(argv[1][1]=='c'){
		int len1 = strlen(argv[2]);
		int len2 = strlen(argv[3]);
		char* fullPath = malloc(len1+len2+2);
		fullPath[0]='\0';
		strncat(fullPath,argv[2],len1);
		strncat(fullPath,slash,2);
		strncat(fullPath,argv[3],len2);
		int fd=open(fullPath, O_WRONLY | O_CREAT, 0644);
		if(fd<0){
			perror("Failed to complete create operation");
			exit(1);
		}

		struct dirent *de;
		DIR *dr = opendir(argv[2]);

		while((de=readdir(dr))!=NULL){
			if(de -> d_type == DT_DIR){
				continue;
			}
			
			int len_of_file = strlen(de->d_name);
			if((len_of_file==1 && de->d_name[0]=='.') || (len_of_file==2 && de->d_name[0]=='.' && de->d_name[1]=='.'))
				continue;
			char* curr_file_name = malloc(len1+len_of_file+2);
			curr_file_name[0]='\0';
			strncat(curr_file_name,argv[2],len1);
			strncat(curr_file_name,slash,2);
			strncat(curr_file_name,de->d_name,len_of_file);
			curr_file_name[len1+len_of_file+1]='\0';
			if(!strncmp(curr_file_name,fullPath,len1+len2+1))
				continue;
			int curr_fd = open(curr_file_name, O_RDONLY);
			if(curr_fd<0){
				perror("Failed to complete creation operation");
				exit(1);
			}

			int file_size = lseek(curr_fd,0,SEEK_END);
			int length = 0;
			int temp = file_size;
			while(temp){
				length++;
				temp/=10;
			}
			if(file_size==0) length = 1;
			char* fsz = malloc( length + 1 );
			snprintf( fsz, length + 1, "%d", file_size );
			char* file_info = malloc(len_of_file + file_size + 3);
			file_info[0]='\0';
			strncat(file_info,de->d_name,len_of_file);
			strcat(file_info,"\n");
			strncat(file_info,fsz,length+1);
			if(write(fd,file_info,strlen(file_info))<0){
				perror("Failed to complete creation operation")	;
				exit(1);   
			}
			if(write(fd,"\n",1)<1){
				perror("Failed to complete creation operation")	;
				exit(1);
			}
			char* buffer = malloc(chunk_size+1);
			int bytes_read=0;
			lseek(curr_fd,0,SEEK_SET);
			while((bytes_read = read(curr_fd,buffer,chunk_size))>0){
				if(write(fd,buffer,bytes_read)<0){
					perror("Failed to complete creation operation")	;
					exit(1);
				};
				memset(buffer,0,sizeof(buffer));
			}

			close(curr_fd);
		}

	}
	else if(argv[1][1]=='d'){
		int len = strlen(argv[2]);
		char *fullPath = malloc(1001);
		memset(fullPath,'\0',1001);
		sprintf(fullPath,"%s",argv[2]);

		char* token = strtok(fullPath, "/");
		char* dir = malloc(1001);
		memset(dir,'\0',1001);

		strcat(dir,slash);
		char *dump = "Dump";
	    while (token != NULL) {
	    	int len=strlen(token);
	    	if(len >= 4 && token[len-4]=='.' && token[len-3]=='t' && token[len-2]=='a' && token[len-1]=='r'){
	    		strncat(dir,token,len-4);
	    		strncat(dir,dump,5);
	    		break;
	    	}
	    	strcat(dir,token);
	    	strcat(dir,slash);
	        token = strtok(NULL, "/");
	    	
	    }
	    mkdir(dir,0777);
	    int c=0;

	    char* buffer = malloc(chunk_size+1);

	    int fd= open(argv[2],O_RDONLY );

	    if(fd<0){
	    	perror("Failed to complete extraction operation");
	    	exit(1);
	    }

	    int tot = lseek(fd,0,SEEK_END);
	    lseek(fd,0,SEEK_SET);
	    for(int i=0;;i++){
		    char buff[2];
		    buff[1]='\0';
		    char* file_name = malloc(20);
		    file_name[0]='\0';
		    do{
		    	read(fd,buff,1);
		    	if(buff[0]!='\n'){
			    	strcat(file_name,buff);
		    	}
			    else 
			    	break;
		    }while(1);
		    int sz=0;
		    do{
		    	read(fd,buff,1);
		    	if(buff[0] != '\n')
		    		sz=sz*10+(buff[0]-'0');
		    	else
		    		break;
		   	}while(1);

		   	c++;
		   	int to_read=min(chunk_size,sz);
		   	char* path_to_file = malloc(len+sz+2);
		   	path_to_file[0]='\0';
		   	strcat(path_to_file,dir);
		   	strcat(path_to_file,slash);
		   	strcat(path_to_file,file_name);
		   	int cfd = open(path_to_file,O_WRONLY | O_CREAT,0644);
		   	if(cfd < 0){
		   		perror("Failed to complete extraction operation");
		   		exit(1);
		   	}
		   	int r=0;
		   	while(to_read){
		   		r=read(fd,buffer,to_read);
		   		if (r<0)
		   		{
		   			perror("Failed to complete extraction operation");
		   			exit(1);
		   		}
		   		sz-=to_read;
		   		if(write(cfd,buffer,r)<0){
		   			perror("Failed to complete extraction operation");
		   			exit(1);
		   		}

		   		to_read = min(chunk_size,sz);
		   	}
		   	close(cfd);

		   	if(lseek(fd,0,SEEK_CUR)==tot)
		   		break;
		  }


	}
	else if(argv[1][1]=='e'){
		int len = strlen(argv[2]);
		char *fullPath = malloc(len+20);
		fullPath[0]='\0';
		strncat(fullPath,argv[2],len);
		char* token = strtok(fullPath, "/");
		char* dir = malloc(len+1);
		dir[0]='\0';
		strcat(dir,slash);
	    char* buffer = malloc(chunk_size+1);

	    while (token != NULL) {
	    	int len=strlen(token);
	    	if(len >= 4 && token[len-4]=='.' && token[len-3]=='t' && token[len-2]=='a' && token[len-1]=='r'){
	    		break;
	    	}
	    	strcat(dir,token);
	    	strcat(dir,slash);
	        token = strtok(NULL, "/");
	    }
		int fd = open(argv[2],O_RDONLY);
		if(fd<0){
				perror("Failed to complete extraction operation");
				exit(1);
		}
		int tot = lseek(fd,0,SEEK_END);
	    lseek(fd,0,SEEK_SET);
		for(int i=0;;i++){
		    char buff[2];
		    buff[1]='\0';
		    char* file_name = malloc(20);
		    file_name[0]='\0';
		    do{
		    	read(fd,buff,1);
		    	if(buff[0]!='\n'){
			    	strcat(file_name,buff);
		    	}
			    else 
			    	break;
		    }while(1);
		    int sz=0;
		    do{
		    	read(fd,buff,1);
		    	if(buff[0] != '\n')
		    		sz=sz*10+(buff[0]-'0');
		    	else
		    		break;
		   	}while(1);
		   	int temp=sz;
		   	if(strlen(file_name)==strlen(argv[3]) && !strncmp(file_name,argv[3],strlen(argv[3]))){
		   		strcat(dir,"/IndividualDump");
		   		mkdir(dir,0777);
		   		int to_read=min(chunk_size,sz);
			   	char* path_to_file = malloc(len+sz+2);
			   	path_to_file[0]='\0';
			   	strcat(path_to_file,dir);
			   	strcat(path_to_file,slash);
			   	strcat(path_to_file,file_name);
			   	int cfd = open(path_to_file,O_WRONLY | O_CREAT,0644);
			   	if(cfd<0){
					perror("Failed to complete creation operation");
					exit(1);
				}
			   	int r=0;
			   	while(to_read){
			   		r=read(fd,buffer,to_read);
			   		
			   		sz-=to_read;
			   		if(write(cfd,buffer,r)<0){
			   			perror("Failed to complete extraction operation");
			   			exit(1);

			   		};
			   		to_read = min(chunk_size,sz);
			   	}
			   	close(cfd);
			   	exit(0);
		   	}
		   	lseek(fd,sz,SEEK_CUR);

		   	if(lseek(fd,0,SEEK_CUR)==tot){
		   		printf("No such file is present in tar file.\n" );
		   		break;
		   	}
		  }


	}
	else if(argv[1][1]=='l'){
		int len = strlen(argv[2]);
		char *fullPath = malloc(1001);
		fullPath[0]='\0';
		strncat(fullPath,argv[2],len);
		char* token = strtok(fullPath, "/");
		char* dir = malloc(1001);
		dir[0]='\0';
		strcat(dir,slash);
	    char* buffer = malloc(chunk_size+1);

	    while (token != NULL) {
	    	int len=strlen(token);
	    	if(len >= 4 && token[len-4]=='.' && token[len-3]=='t' && token[len-2]=='a' && token[len-1]=='r'){
	    		break;
	    	}
	    	strcat(dir,token);
	    	strcat(dir,slash);
	        token = strtok(NULL, "/");

	    }
		int fd = open(argv[2],O_RDONLY);
		int tot = lseek(fd,0,SEEK_END);
	    lseek(fd,0,SEEK_SET);
	    if(fd<0){
	    	perror("Failed to complete list operation");
	    	exit(1);
	    }
	    strcat(dir,"/tarStructure");
	    int p = open(dir,O_WRONLY | O_CREAT, 0644);
		if(p<0){
		    	perror("Failed to complete list operation");
		    	exit(1);
			}

	    int temp = tot,length=0;
		while(temp){
			length++;
			temp/=10;
		}
		if(tot==0) length = 1;
		char* fsz = malloc( length + 1 );
		snprintf( fsz, length + 1, "%d", tot );
		if(write(p,fsz,length+1)<0){
			perror("Failed to complete list operation");
			exit(1);
		};
		write(p,"\n",1);

		int cnt=0;
		for(int i=0;;i++){
		    char buff[2];
		    buff[1]='\0';
		    char* file_name = malloc(20);
		    file_name[0]='\0';
		    do{
		    	read(fd,buff,1);
		    	if(buff[0]!='\n'){
			    	strcat(file_name,buff);
		    	}
			    else 
			    	break;
		    }while(1);
		    int sz=0;
		    do{
		    	read(fd,buff,1);
		    	if(buff[0] != '\n')
		    		sz=sz*10+(buff[0]-'0');
		    	else
		    		break;
		   	}while(1);
		   	int temp=sz;
		   	cnt++;
		   	lseek(fd,sz,SEEK_CUR);

		   	if(lseek(fd,0,SEEK_CUR)==tot){
		   		temp = cnt,length=0;
				while(temp){
					length++;
					temp/=10;
				}
				if(cnt==0) length = 1;
				char* fz = malloc( length + 1 );
				snprintf( fz, length + 1, "%d", cnt );
				if(write(p,fz,length+1)<0){
					perror("Failed to complete list operation");
					exit(1);
				}
				write(p,"\n",1);
		   		break;
		   	}
		   }

		  lseek(fd,0,SEEK_SET);
		  for (int i = 0; ; ++i)
		  {
		  	char buff[2];
		    buff[1]='\0';
		    char* file_name = malloc(20);
		    file_name[0]='\0';

		  do{
		    	read(fd,buff,1);
		    	if(buff[0]!='\n'){
			    	strcat(file_name,buff);
		    	}
			    else 
			    	break;
		    }while(1);
		    char *file_size = malloc(12);
		    file_size[0]='\0';
		    int sz=0;
		    do{
		    	read(fd,buff,1);
		    	if(buff[0] != '\n'){
		    		sz=sz*10+(buff[0]-'0');
		    		strcat(file_size,buff);
		    	}
		    	else
		    		break;
		   	}while(1);
		   	if(write(p,file_name,strlen(file_name))<0){
		   		perror("Failed to complete list operation");
		   		exit(1);
		   	};
		   	write(p," ",1);
		   	if(write(p,file_size,strlen(file_size))<0){
		   		perror("Failed to complete list operation");
		   		exit(1);
		   	};
		   	write(p,"\n",1);
		   	lseek(fd,sz,SEEK_CUR);

		   	if(lseek(fd,0,SEEK_CUR)==tot){
		   		break;
		   	}
		  }
	

	}
	return 0;
}
