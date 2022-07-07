#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mpi.h"
#include <limits.h>
#include <stdbool.h>

#define MASTER 0               /* taskid of first task */
#define FROM_MASTER 1          /* setting a message type */
#define FROM_WORKER 2          /* setting a message type */

int max(int a, int b){
	return (a > b) ? a : b;
}

int min(int a, int b){
	return (a < b) ? a : b;
}

int main(int argc, char *argv[]){
	
	long size;
	int type,rc;
	if(argc == 3)
	{
		size = atoi(argv[1]);
//		printf("Size: %ld", size);
		type = atoi(argv[2]);
	}
        else{
		printf("Exactly 2 arguments required to run\n");
		return 0;
	}
    int taskid, numtasks, numworkers, a, b,mtype,offset, numData;
    

    MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

    if (numtasks < 2 ) {
        printf("Need at least two MPI tasks. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }
    numworkers = numtasks;

    
    double timeWcalc = 0, timeTotal=0, timeSR=0;
    timeTotal = MPI_Wtime();
	
	//initialization
	
	numData = size / numworkers;
	int extra = size%numworkers;
	numData = (taskid < extra) ? numData+1 : numData;
	int nums[numData];
	for(int i = 0; i < numData; i++){
		if (type == 0) nums[i] = rand() % 100 + 1;
		if (type == 1) nums[i] = i+(size/numworkers)*taskid+min(extra,taskid)+1;
		if(type == 2) nums[i] = (size - (size/numworkers)*(numworkers-taskid-1) - max(extra-taskid-1,0))-i;
	}
	
	for(int i = 0; i < numData; i++){
		printf("Task: %d, %d\n", taskid, nums[i]);
	}
		
	//calculation
	
	//sequential merge
	
	//merge with other nodes 
	
	
	
	
    /*if(taskid == 0){
        //Master node
		
		//initialization
		//nums = (int *) malloc(size*sizeof(int));

		for(int i = 0; i < size; i++){
			if (type == 0) nums[i] = rand() % 100 + 1;
			if (type == 1) nums[i] = i;
			if(type == 2) nums[i] = size-i-1;
		}
		
        //send-recieve data
	timeSR = MPI_Wtime();
        int numData = size/numworkers;
        int extra = size%numworkers;
        int offset = 0;
        mtype = 1;
		
		
        for (int dest=1; dest<=numworkers; dest++)
        {
            numData = (dest <= extra) ? numData+1 : numData;
            //printf("Sending %d numData to task %d offset=%d\n",numData,dest,offset);
            MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
            MPI_Send(&numData, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
			MPI_Send(&nums[offset], numData, MPI_INT, dest, mtype, MPI_COMM_WORLD);

            offset = offset + numData;
        }

        mtype = 1;
        for (int i=1; i<=numworkers; i++)
        {
            int source = i;
            MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
            MPI_Recv(&numData, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
			MPI_Recv(&nums[offset], numData, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
            //printf("Received results from task %d\n",source);
        }
		
//	Printing out sourted array
		
		for(int i = 0; i < size; i++){
			if(i%numData == 0) printf("\n");
			printf("%d,",nums[i]);
			
		}
		printf("\n");
		
		timeSR = MPI_Wtime() - timeSR;
	
        double calcAvg;
	MPI_Barrier(MPI_COMM_WORLD);
	
	//getting average time taken
	timeTotal = MPI_Wtime() - timeTotal;
	MPI_Reduce(&timeWcalc,&calcAvg, 1, MPI_DOUBLE, MPI_SUM, 0,MPI_COMM_WORLD);
	calcAvg /= numworkers;
	
	//printf("Bitonic Sort: %d proccessors and %d size\n", numtasks, size);
	printf("%f,%f,%f\n", timeTotal, timeSR, calcAvg);
    }
    else{
        //workers

        //receive data
        mtype = 1;
        MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
        MPI_Recv(&numData, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
		MPI_Recv(&nums, numData, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);

        //sorting
		//between offset and offset + numData
		timeWcalc = MPI_Wtime();
		int order;
		if(taskid % 2 == 1) order = 1;
		else order = 0;
		sort(nums, numData, order); 
		
		//bitonic (between nodes) 
		

		timeWcalc = MPI_Wtime() - timeWcalc;
        //send back data
        mtype = 1;
        MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
        MPI_Send(&numData, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
        MPI_Send(&nums, numData, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
	
 	double calcAvg;
	MPI_Barrier(MPI_COMM_WORLD);
	
	//getting average time taken
	timeTotal = MPI_Wtime() - timeTotal;
	MPI_Reduce(&timeWcalc,&calcAvg, 1, MPI_DOUBLE, MPI_SUM, 0,MPI_COMM_WORLD);
    }*/
	

	

    MPI_Finalize();

}
