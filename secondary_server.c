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
#define MAX_NODES 30


struct msgbuf {
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


typedef struct {
    int node;
    int level;
    int numNodes;
    int reqNo;
} ThreadArgs;


pthread_mutex_t mutex[100] = PTHREAD_MUTEX_INITIALIZER;

int adjacencyMatrix[100][30][30] = {0};		
	
int visited[100][MAX_NODES];
int ans[100][30][30] = {0};
int count[100][30] = {0};

int dfs[100][30] = {0};
int cnt[100] = {0};

int numreader=0;

void* depthFirstTraversal(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    int node = threadArgs->node;
    int level = threadArgs->level;
    int numNodes = threadArgs->numNodes;
    
    int reqNo = threadArgs->reqNo;

    pthread_mutex_lock(&mutex[reqNo]);

    
    visited[reqNo][node] = 1;

    pthread_mutex_unlock(&mutex[reqNo]);

	int i;
	int flg = 0;
    for (i = 0; i < numNodes; ++i) {
        if (adjacencyMatrix[reqNo][node][i] == 1 && !visited[reqNo][i]) {
            flg = 1;
            ThreadArgs newThreadArgs;
            newThreadArgs.node = i;
            newThreadArgs.level = level + 1;
            newThreadArgs.numNodes = numNodes;
            newThreadArgs.reqNo  = reqNo;

            pthread_t newThread;
            if (pthread_create(&newThread, NULL, depthFirstTraversal, (void*)&newThreadArgs) != 0) {
                perror("Error creating thread");
                exit(EXIT_FAILURE);
            }

            if (pthread_join(newThread, NULL) != 0) {
                perror("Error joining thread");
                exit(EXIT_FAILURE);
            }
        }
        
    }
    if(i == numNodes && flg == 0){
    	dfs[reqNo][cnt[reqNo]++] = node+1;
    }

    pthread_exit(NULL);
}



void* breadthFirstTraversal(void* args) {
    ThreadArgs* threadArgs = (ThreadArgs*)args;
    int node = threadArgs->node;
    int level = threadArgs->level;
    int numNodes = threadArgs->numNodes;
    
    int reqNo = threadArgs->reqNo;

    pthread_mutex_lock(&mutex[reqNo]);

    ans[reqNo][level][count[reqNo][level]++] = node;
    visited[reqNo][node] = 1;

    pthread_mutex_unlock(&mutex[reqNo]);

    for (int i = 0; i < numNodes; ++i) {
        if (adjacencyMatrix[reqNo][node][i] == 1 && !visited[reqNo][i]) {
            ThreadArgs newThreadArgs;
            newThreadArgs.node = i;
            newThreadArgs.level = level + 1;
            newThreadArgs.numNodes = numNodes;
            newThreadArgs.reqNo = reqNo;

            pthread_t newThread;
            if (pthread_create(&newThread, NULL, breadthFirstTraversal, (void*)&newThreadArgs) != 0) {
                perror("Error creating thread");
                exit(EXIT_FAILURE);
            }

            if (pthread_join(newThread, NULL) != 0) {
                perror("Error joining thread");
                exit(EXIT_FAILURE);
            }
        }
    }

    pthread_exit(NULL);
}


void* readOperation(void* arg){
	sem_t *write;
	sem_t *read;
	int numNodes;
	
	ThreadData* thredd;
	thredd = (ThreadData*)arg;
	
	int msgid = thredd->msgid;
	int oper = thredd->op;
	int reqNo = thredd->ClientID;
	char filename[256];
	strcpy(filename , thredd->filename);
	
	key_t shkey;
        int shmId;
        int *sharedData;
        
        
        
        key_t key = ftok("client.c" , reqNo ) ;
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
	
	int startVertex;
	
	char name[100];
		
	strcpy(name,"/");
	strcat(name ,filename);
	char name_read[100];
	strcpy(name_read,"/");
	
	strcat(name_read,filename);
	strcat(name_read,"-r");
	read=sem_open(name_read,O_CREAT,0666,20);
	if(read==SEM_FAILED)
		perror("sem");
        write=sem_open(name,O_CREAT,0666,1);
        if(write==SEM_FAILED)
        	perror("sem");
	
        		
        		sem_wait(read);
        		printf("Semaphore acquired!\n");
        		numreader++;
        		if(numreader==1)
        		{
        			sem_wait(write);
        			printf("Writing Semaphore acquired!\n");}
        		sem_post(read);	
	//CRITICAL SECTION
	while( 0 == *sharedData);
	startVertex = *sharedData  - 1;
	printf("Start is %d\n",startVertex);
	
	FILE* file = fopen(filename , "r");
	fscanf(file , "%d" , &numNodes );
	
	for(int k = 0 ; k < numNodes ; k++){
	
		for(int l = 0 ; l < numNodes ; l++){
		
			fscanf(file, "%d", &adjacencyMatrix[reqNo][k][l]);	
			
		}
		
	}
	
    pthread_t thread;
    ThreadArgs threadArgs;
    threadArgs.node = startVertex;  // Assuming the root is at index 0
    threadArgs.level = 0;
    threadArgs.numNodes = numNodes;
    threadArgs.reqNo = reqNo;

	char Traversal[100] = "" ;
	if(oper == 3){

    if (pthread_create(&thread, NULL, breadthFirstTraversal, (void*)&threadArgs) != 0) {
        perror("Error creating thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("Error joining thread");
        exit(EXIT_FAILURE);
    }

	

	for(int i = 0 ; i < numNodes ; i++){
		for(int j = 0 ; j < count[reqNo][i] ; j++){
			
			printf("%d ",ans[reqNo][i][j]+1);
			
			char temp[3] ;
			sprintf(temp  ,  "%d" , ans[reqNo][i][j] + 1);
			
			strcat(Traversal,  temp);
			strcat(Traversal, " ");
		}
		
	}
	}
	else if(oper == 4){
	printf("aaya");
	 if (pthread_create(&thread, NULL, depthFirstTraversal, (void*)&threadArgs) != 0) {
        perror("Error creating thread");
        exit(EXIT_FAILURE);
    }

    if (pthread_join(thread, NULL) != 0) {
        perror("Error joining thread");
        exit(EXIT_FAILURE);
    }
	printf("v big sad");

	
	
	for(int i = 0; i < cnt[reqNo] ; i++){
		printf("%d " , dfs[reqNo][i]);
		char temp[3] ;
		sprintf(temp  ,  "%d" , dfs[reqNo][i]);
		strcat(Traversal,  temp);
		strcat(Traversal, " ");
	}
	
	}
	
	
	printf("\n%s\n" , Traversal);
	
	struct msgbuf retmsg;
       	retmsg.mtype = reqNo;
       	
       	
       	retmsg.operation = oper;
       	retmsg.reqNo = reqNo;
       	
       	strcpy(retmsg.mtext , Traversal);
       	
       	if(msgsnd(msgid, &retmsg, sizeof(retmsg)-sizeof(long), 0)==-1){
			perror("error in msgsnd()");
        		exit(1);
		} 
		else{
        		printf("Sent\n");
		}
	
	
		sem_wait(read);
				numreader--;
				if(numreader==0)
					sem_post(write);
				sem_post(read);	
	pthread_exit(NULL);


}

int main() {
	int listen = 103;
	printf("Secondary server number: ");
	int server;
	
	scanf("%d", &server);
	
	if(server==1)
	listen=103;
	else if(server==2)
	listen=104;
	
	
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
        int shutdown=0;
	int i=0;
	pthread_t pt[100];
    	while (!shutdown) {		//recieve messages in loop
	
        struct msgbuf message;
		
        if(msgrcv(msgid, &message, sizeof(message)-sizeof(long), listen, 0)==-1){
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
       	
       
       	if (pthread_create(&pt[i] , NULL, readOperation , (void*)(&thred) ) != 0) {
        perror("Error creating thread");
        return EXIT_FAILURE;
    }
    	i++;
       

	//msgctl(msgid , IPC_RMID  , NULL);
        
	}
	if(i!= 0){
	for(int j = 0 ; j < i ; j++){
		
        	pthread_join(pt[j],NULL);
        	}
        	}
	struct msgbuf shutmsg;
        	strcpy(shutmsg.mtext,"OK");
        	shutmsg.mtype=107;		//beacon shut down request to all
        	if(msgsnd(msgid, &shutmsg, sizeof(shutmsg)-sizeof(long), 0)==-1){
				perror("error in msgsnd()");
        			exit(1);
			}
	printf("Secondary Server shutting down\n");	
        return 0;
}
