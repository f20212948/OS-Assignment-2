#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <errno.h> 
#define MAX_MSG_SIZE 256

struct msgbuff {
    long mtype;
     int reqNo;
    int operation;
    char mtext[MAX_MSG_SIZE];
};

typedef struct threaddat {
	
	int ClientID;
	char filename[256];
	int op;
	int msgid;	

} ThreadData;




void* createFile(void* arg ){ //ClientID , void* filename , void* op){


	ThreadData* thredd;
	thredd = (ThreadData*)arg;	
	sem_t *write;
	char name[100];
	strcpy(name,"/");
	strcat(name,thredd->filename);
	printf("%s\n",name);
	
	write = sem_open(name,O_CREAT,0666,1);
    	if(write== SEM_FAILED){
	perror("semaphore error");

	}	
	
	int msgid = thredd->msgid;
	int oper = thredd->op;
       	int len;
       	key_t shkey;
        int shmId;
        int *sharedData;
        int reqNo = thredd->ClientID;
        
     
    	
       	key_t key = ftok("client.c" , reqNo ) ;
        shmId = shmget(key , sizeof(int) + sizeof(int)* 100 * 100 , 0666 | IPC_CREAT);
        if (shmId == -1) {
        	perror("shmget");
        	exit(1);
    		}
    			
    	sharedData = (int*)shmat(shmId, NULL, 0);
    			
    	if ((void*)sharedData == (void*)-1) {
        	perror("shmat");
        	exit(1);
    		}
    		
    	if (sem_wait(write) == -1) {	//CRITICAL SECTION
        	perror("sem_wait");
        	exit(EXIT_FAILURE);
    	}
    	printf("Semaphore acquired!\n");
    		
    	while( 0 ==  *sharedData) ;
    	len = *sharedData;
    	
    	int adj[len][len];
    	
    	memcpy(adj , sharedData + 1 , sizeof(int)*len*len);	
    	
    	
    	printf("n is - %d\n" , len);
    	for(int it = 0 ; it < len ; it++){
    		for(int it2 = 0 ; it2 < len ; it2++){
    			
    			printf("%d ",adj[it][it2]);
    		
    		}
    		printf("\n");
    	}
    	
    		
       	
       	
       	FILE* file = fopen(thredd->filename, "w");
       	
       	if (file == NULL) {
        perror("Error creating file");
        pthread_exit(NULL);
    	}
       	fprintf(file , "%d\n" , len); 
       	for (int i = 0; i < len; ++i) {
        	for (int j = 0; j < len; ++j) {
            		fprintf(file, "%d ", adj[i][j]);
        	}
        	fprintf(file, "\n"); 
    	}
       	
       	fclose(file);
       	
       	struct msgbuff retmsg;
       	retmsg.mtype = reqNo;
       	
       	
       	retmsg.operation = oper;
       	retmsg.reqNo = reqNo;
       	
       	printf("reqNo is %d\n",reqNo);
       	printf("threadNo is %lu\n",pthread_self());
       	if(oper == 1)
       	strcpy(retmsg.mtext , "Created");
       	else if(oper == 2)
       	strcpy(retmsg.mtext , "Modified");
       	
       	if(msgsnd(msgid, &retmsg, sizeof(retmsg)-sizeof(long), 0)==-1){
			perror("error in msgsnd()");
        		exit(1);
		} 
		else{
        		printf("Sent\n");
		}
	sem_post(write);
	pthread_exit(NULL);
}




int main() {
	int shutdown=0;
		int i=0;
			pthread_t pt[100];
	key_t key = ftok("client.c", 'C');
        if(key==-1){
		perror("error in ftok()");
        	exit(1);
	}
        int msgid = msgget(key, 0666 );	//creates the message queue
        if (msgid == -1) {
        	perror("msgget");
        	exit(1);
        }
	
    	while (!shutdown) {		//recieve messages in loop
	
        struct msgbuff message;
	
        if(msgrcv(msgid, &message, sizeof(message)-sizeof(long), 102, 0)==-1){
        	perror("error1");
       		continue;	//to repeat msgrcv till a message is recieved
        }
	if(message.operation==5){
	printf("Shut Received\n");
	shutdown=1;
	break;
	}
        
	printf("%s\n",message.mtext);
       	printf("%d\n",message.reqNo);
       	printf("%d\n",message.operation);
       	
       	int clientID = message.reqNo;
       	int opno = message.operation;
       	char filename[256];
       	strcpy(filename , message.mtext);
       	
       	ThreadData thred;
       	thred.ClientID = message.reqNo;
       	thred.op = message.operation;
       	thred.msgid = msgid;
       	strcpy(thred.filename , message.mtext);
       	
       
       
       	if (pthread_create(&pt[i] , NULL, createFile, (void*)(&thred) ) != 0) {
        perror("Error creating thread");
        return EXIT_FAILURE;
        }
        
        i++;

	
        
	}
	if(i!= 0){
	for(i-1;i>=0;i--)
        		pthread_join(pt[i],NULL);
	}
	struct msgbuff shutmsg;
        	strcpy(shutmsg.mtext,"OK");
        	shutmsg.mtype=107;		//beacon shut down request to all
        	if(msgsnd(msgid, &shutmsg, sizeof(shutmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			}
	printf("Primary Server shutting down\n");		
        return 0;
}
