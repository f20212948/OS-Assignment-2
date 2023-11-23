#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <semaphore.h>
 #include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */

#define MAX_MSG_SIZE 256

struct msgbuf {
    long mtype;
     int reqNo;
    int operation;
    char mtext[MAX_MSG_SIZE];
};


int main() {
	sem_t *write;
	sem_t *read;
	int numreader=0;
	
	int shutdown=0;	
        // Initialize message queue
        key_t key = ftok("client.c", 'C');
        if(key==-1){
		perror("error in ftok()");
        	exit(1);
	}
        int msgid = msgget(key, 0666 | IPC_CREAT);	//creates the message queue
        if (msgid == -1) {
        	perror("msgget");
        	exit(1);
        }
	
    	while (!shutdown) {		//recieve messages in loop
	
        struct msgbuf message;
        
       
	
        if(msgrcv(msgid, &message, sizeof(message)-sizeof(long), 101, 0)==-1){
        	perror("error1");
       		continue;	//to repeat msgrcv till a message is recieved
        }
	
        
	printf("%s\n",message.mtext);
       	printf("%d\n",message.reqNo);
       	printf("%d\n",message.operation);
       		int op= message.operation;
       	struct msgbuf retmsg;
       	retmsg.mtype = (message.operation == 1 || message.operation == 2) ? 102 : (message.reqNo % 2 == 1 ? 103 : 104);
       	
       	retmsg.operation = message.operation;
       	retmsg.reqNo = message.reqNo;
       	
       	strcpy(retmsg.mtext , message.mtext);
       	char name[100];
	strcpy(name,"/");
	char name_read[100];
	strcpy(name_read,"/");
	strcat(name,message.mtext);
	strcat(name_read,message.mtext);
	strcat(name_read,"-r");
       	if(op ==1 || op==2){			//writer requests
			//write = sem_open(name_write,O_CREAT,0666,1);
			     		
       											// critical section
       			if(msgsnd(msgid, &retmsg, sizeof(retmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			} 
			else
        			printf("Sent\n");
				
        	}
        else if(op==3 || op==4){					//reader requests
        		
        		
        		if(msgsnd(msgid, &retmsg, sizeof(retmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			} 
			else
        			printf("Sent %d\n",message.reqNo);
			
	//msgctl(msgid , IPC_RMID  , NULL);
        }
        else{
        	printf("Shutdown state\n");
        	shutdown=1;
        	struct msgbuf shutmsg;
        	strcpy(shutmsg.mtext,"SHUT DOWN");
        	shutmsg.operation=5;
        	shutmsg.mtype=102;		//beacon shut down request to all
        	if(msgsnd(msgid, &shutmsg, sizeof(shutmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			}
		shutmsg.mtype=103;		//beacon shut down request to all
        	if(msgsnd(msgid, &shutmsg, sizeof(shutmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			}
			shutmsg.mtype=104;		//beacon shut down request to all
        	if(msgsnd(msgid, &shutmsg, sizeof(shutmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			}
			}
       
	}
	//waiting for OK from all servers
	
	int i=3;		//for 3 servers
	while(i>0){
	struct msgbuf message;
	 if(msgrcv(msgid, &message, sizeof(message)-sizeof(long), 107, 0)==-1){
        	perror("error1");
       		continue;	//to repeat msgrcv till a message is recieved
        }
        
        	i--;
	}
	
	sleep(5);
	for(int i=1; i<21;i++){			//CLOSING ALL NAMED SEMAPHORES
		char temp[10];
		char temp2[10];
		strcpy(temp , "/G");
		char num[2];
		sprintf(num , "%d" , i);
		strcat(temp , num);
		strcat(temp, ".txt");
		strcpy(temp2 , temp);
		strcat(temp2 , "-r");
		
		sem_t* sem;
		sem = sem_open(temp , O_CREAT , 0666 , 1);
		sem_close(sem);
		sem_unlink(temp);
		sem_t* sem1 ;
		sem1= sem_open(temp2 , O_CREAT , 0666 , 20);
		sem_close(sem1);
		sem_unlink(temp2);
		
	}
			if(msgctl(msgid,IPC_RMID,NULL)==-1){	//closing message queue
		           perror("Error in msgctl() in line 52");
		           exit(4);
	            	}
	            	printf("Shutting down\n");	
        return 0;
}


