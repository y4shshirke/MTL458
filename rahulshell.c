#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>

#define MAXLEN 2049 
#define MAXWORDS 100 
#define MAXBUFFER 1000 

char *historyBuffer[MAXBUFFER];
char lastDirectory[2049];
int bufferPointer=0;

void getDirectory(char* dir){
    for(int i=0;i<2049;i++){
        dir[i]='\0';
    }
    getcwd(dir,2049);
}

void prse(char* inputCmd, char** inputParsed){
    for(int i=0;i<MAXWORDS;i++){
        inputParsed[i]= strsep(&inputCmd," ");
        if(inputParsed[i]==NULL){
            break;
        }
        if(strlen(inputParsed[i])==0){
            i--;
        }
    }
    return;
}

int prsePipe(char* inputCmd, char** inputParsed){
    for(int i=0;i<2;i++){
        inputParsed[i]= strsep(&inputCmd,"|");

        if(inputParsed[i]==NULL){
            break;
        }
        if(strlen(inputParsed[i])==0){
            i--;
        }
    }
    
    if(inputParsed[1]==NULL){
        return 0;
    }
    else{
        return 1;
    }
}

void execPipeInput(char* inp, char** inputParse, char** inputPipeParse, char** chkPipe){
    prse(chkPipe[0],inputParse);
    prse(chkPipe[1],inputPipeParse);

    int fileDescriptors[2]; 
    pid_t child1, child2;
    int status1;
    int invFlg=0; 

    if (pipe(fileDescriptors) < 0) {
        printf("Pipe could not be initialized\n");
        return;
    }

    if(!strcmp(inputParse[0],"exit")){
        exit(0);
    }
    else if(!strcmp(inputParse[0],"cd")){
        inputParse[1]= &chkPipe[0][3];

        if(inputParse[1][0]=='~'){
            getDirectory(lastDirectory);
            chdir(getenv("HOME"));
        }
        else if(!strcmp(inputParse[1],"-")){
            char temp[2049];
            getDirectory(temp);

            if(chdir(lastDirectory)<0){
                printf("There is no last used Directory or The File Path Does not exist for 1st Command\n");
                invFlg=1;
            }

            for(int i=0;i<2049;i++){
                lastDirectory[i]=temp[i];
            }  
        }
        else{
            getDirectory(lastDirectory);
            if(chdir(inputParse[1])<0){
                printf("The File Path Does not exist for 1st Command\n");
                invFlg=1;
            }     
        }
    }
    else{
        child1 = fork();

        if (child1<0) {
            printf("Could not fork\n");
            exit(0);
        }
        else if (child1==0){
            close(fileDescriptors[0]);
            dup2(fileDescriptors[1], STDOUT_FILENO);
            close(fileDescriptors[1]);
    
            if(!strcmp(inputParse[0],"history")){
                if(inputParse[1]==NULL){
                    for(int i=0;i<= bufferPointer-2;i++){
                        printf("%s\n",historyBuffer[i]);
                    }
                    exit(0);
                }
                else{
                    if(strlen(inputParse[1])==1 && inputParse[1][0]=='0'){
                        exit(0);
                    }
                    int last= atoi(inputParse[1]);
                    if(last<=0){
                       printf("%s\n","Invalid History Command");
                       exit(1); 
                    }

                    int start= bufferPointer-last-1;
                    if(start<0){
                        start=0;
                    }
                    for(int i=start;i<=bufferPointer-2;i++){
                        printf("%s\n",historyBuffer[i]);
                    }
                }
                exit(0);
            }
            else if (execvp(inputParse[0], inputParse) < 0) {
                printf("Execution failed for 1st command\n");
                exit(1);
            }
            exit(0);
        }
    }

    if(invFlg){
        return;
    }

    wait(&status1);
    
    if(WIFEXITED(status1) && WEXITSTATUS(status1)==0){
        if(!strcmp(inputPipeParse[0],"exit")){
        exit(0);
        return ;
        }
        else if(!strcmp(inputPipeParse[0],"cd")){
            inputPipeParse[1]= &chkPipe[1][3];
    
            if(inputPipeParse[1][0]=='~'){
                getDirectory(lastDirectory);
                chdir(getenv("HOME"));
            }
            else if(!strcmp(inputPipeParse[1],"-")){
                char temp[2049];
                getDirectory(temp);
    
                if(chdir(lastDirectory)<0){
                    printf("There is no last used Directory or The File Path Does not exist\n");
                }
    
                for(int i=0;i<2049;i++){
                    lastDirectory[i]=temp[i];
                }  
            }
            else{
                getDirectory(lastDirectory);
                if(chdir(inputPipeParse[1])<0){
                    printf("The File Path Does not exist\n");
                }   
            }
            return ;
        }
        else if(!strcmp(inputPipeParse[0],"history")){
            if(inputPipeParse[1]==NULL){
                for(int i=0;i<= bufferPointer-2;i++){
                    printf("%s\n",historyBuffer[i]);
                }
            }
            else{
                if(strlen(inputPipeParse[1])==1 && inputPipeParse[1][0]=='0'){
                    exit(0);
                }
                int last= atoi(inputPipeParse[1]);
                if(last<=0){
                   printf("%s\n","Invalid History Command"); 
                   exit(0);
                }
                
                int start= bufferPointer-last-1;
                if(start<0){
                    start=0;
                }
                for(int i=start;i<=bufferPointer-2;i++){
                    printf("%s\n",historyBuffer[i]);
                }
            }
            return; 
        }
    
        child2 = fork();
        
        if (child2 < 0) {
            printf("Could not fork\n");
            return;
        }
        else if (child2 == 0){
            close(fileDescriptors[1]);
            dup2(fileDescriptors[0], STDIN_FILENO);
            close(fileDescriptors[0]);
    
            if (execvp(inputPipeParse[0], inputPipeParse) < 0) {
                printf("Execution failed for 2nd Command\n");
                exit(0);
            }
            exit(0);
        }
    }
    else{
       printf("Execution failed for 1st command\n");
       return ; 
    }
    
    close(fileDescriptors[0]);
    close(fileDescriptors[1]);

    wait(NULL);
    
    return ;
        
    }


void execInput(char* inputCommand, char** inputParse){
    prse(inputCommand,inputParse);
    if(!strcmp(inputParse[0],"exit")){
        exit(0);
        return ;
    }
    else if(!strcmp(inputParse[0],"cd")){
        inputParse[1]= &inputCommand[3];

        if(inputParse[1][0]=='~'){
            getDirectory(lastDirectory);
            chdir(getenv("HOME"));
        }
        else if(!strcmp(inputParse[1],"-")){
            char temp[2049];
            getDirectory(temp);

            if(chdir(lastDirectory)<0){
                printf("There is no last used Directory or The File Path Does not exist\n");
            }

            for(int i=0;i<2049;i++){
                lastDirectory[i]=temp[i];
            }  
        }
        else{
            getDirectory(lastDirectory);
            if(chdir(inputParse[1])<0){
                printf("The File Path Does not exist\n");
            }
              
        }

        return ;
    }

    int child1= fork();

    if(child1<0){
        printf("Fork Failed");
    }
    else if(child1==0){
        if(!strcmp(inputParse[0],"history")){
            if(inputParse[1]==NULL){
                for(int i=0;i<= bufferPointer-2;i++){
                    printf("%s\n",historyBuffer[i]);
                }
            }
            else{
                if(strlen(inputParse[1])==1 && inputParse[1][0]=='0'){
                    exit(0);
                }

                int last= atoi(inputParse[1]);

                if(last<=0){
                   printf("%s\n","Invalid History Command"); 
                   exit(0);
                }
                
                int start= bufferPointer-last-1;
                if(start<0){
                    start=0;
                }
                for(int i=start;i<=bufferPointer-2;i++){
                    printf("%s\n",historyBuffer[i]);
                }
            } 
        }
        else{
            if(execvp(inputParse[0],inputParse)<0){
                printf("Invalid Command\n");
            }
        }
        exit(0);
    }
    else{
        wait(NULL);
        return;
    }
}

int main()
{
	char inputCommand[MAXLEN], *inputParse[MAXWORDS];
    char *inputPipeParse[MAXLEN], *chkPipe[2];
	int pipeFlag = 0;
    getDirectory(lastDirectory);

	while(1) {
		printf("MTL458 >");

        fgets(inputCommand,MAXLEN,stdin);

        if (inputCommand[strlen(inputCommand) - 1] == '\n')
            inputCommand[strlen(inputCommand) - 1] = '\0';
    

        historyBuffer[bufferPointer]=strdup(inputCommand);
        bufferPointer++;

        pipeFlag= prsePipe(inputCommand,chkPipe);

        if(!pipeFlag){
            execInput(inputCommand,inputParse);
        }
        else{
            execPipeInput(inputCommand,inputParse,inputPipeParse,chkPipe);
        }
           
	}
    
	return 0;
}
