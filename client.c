#define _GNU_SOURCE



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#define MAX_MSG_SIZE 256


struct msgbuff {
    long mtype;
    int reqNo;
    int operation;
    char mtext[MAX_MSG_SIZE];
};

int main() {
   int shutdown=0;
   
    key_t key = ftok("client.c", 'C');
    	if(key==-1){
	perror("error in ftok()");
        exit(1);
	}
    
    int msgid = msgget(key, 0666 );
    if(msgid == -1) {
        perror("No message queue found, check if server is online");
        exit(1);
    }

  

        struct msgbuff message;
        message.mtype = 101;	//to ensure only the message meant for this client is read by it
    
             
               
	while(!shutdown){
		
                printf("Enter Sequence No:\n");
                scanf("%d",&message.reqNo);
                printf("Enter Operation Number\n");
      		printf("1. Add New Graph\n");
       		printf("2. Modify Existing Graph\n");
       		printf("3. BFS\n");
        	printf("4. DFS\n");
        	int choice;
       		 scanf("%d", &choice);
		if(choice>=5 && choice<=0){
		printf("Invalid choice\n");
		continue;
		}
		message.operation=choice;
		
		printf("Enter File Name:\n"); 
		char filename1[256];
                scanf("%s", filename1);
                strcpy(message.mtext , filename1);                

                if(msgsnd(msgid, &message, sizeof(message)-sizeof(long), 0)==-1){
			perror("Server has shut down\n");
        		exit(1);
		} 
		else
        		printf("Sent\n");
		
		
		key_t shkey;
        	int shmId;
        	int *sharedData;
        
        	int n;
        	
        if(choice == 1 || choice == 2){
        	printf("Enter Number of Nodes:\n");
        	scanf("%d",&n);
        	int adj[n][n];
        	printf("Enter Adjacency matrix\n");
        	for(int iter = 0 ; iter < n ; iter++){
        		for(int iter2 = 0 ; iter2 < n ; iter2++){
        			scanf("%d",&adj[iter][iter2]);
        		}
        	}
        	
        	key = ftok("client.c" , message.reqNo);
        	shmId = shmget(key , sizeof(int) + sizeof(int)* n * n , 0666 | IPC_CREAT);
        	if (shmId == -1) {
        		perror("shmget");
        		exit(1);
    			}
    			
    		sharedData = (int*)shmat(shmId, NULL, 0);
    			
    		if ((void*)sharedData == (void*)-1) {
        		perror("shmat");
        		exit(1);
    			}
    		
    		*sharedData = n;
    		
    		memcpy(sharedData + 1, adj , sizeof(int)*n*n);	
    			
        }
        else{
        	printf("Enter Vertice to start:\n");
        	scanf("%d",&n);
        	key = ftok("client.c" , message.reqNo);
        	shmId = shmget(key , sizeof(int) , 0666 | IPC_CREAT);
        	
        	if (shmId == -1) {
        		perror("shmget");
        		exit(1);
    			}
    			
    		sharedData = (int*)shmat(shmId, NULL, 0);
    			
    		if ((void*)sharedData == (void*)-1) {
        		perror("shmat");
        		exit(1);
    			}
    		
    		*sharedData = n;
        }
		
		
		
		struct msgbuff retmsg;
		
		if(msgrcv(msgid, &retmsg, sizeof(retmsg)-sizeof(long), message.reqNo, 0)==-1){
        	printf("server has shut down");
       		shutdown=1;
       			//to repeat msgrcv till a message is recieved
        	}
        
        	printf("--%s--\n" , retmsg.mtext);	
        	
        	if(shmdt(sharedData)==-1){
			perror("error in shmdt");
			exit(-1);
		}
	
        	if(shmctl(shmId, IPC_RMID, NULL)==-1){
        		perror("error in shmctl");
        		exit(-2);
        		}
        	
        	struct msgbuff shutmsg;
        	
        	
        	
        }
        printf("Server has received shut down request, no further requests are allowed\n");

    return 0;

}


