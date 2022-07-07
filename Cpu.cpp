//
// Created by ColtonMulkey on 11/12/2021.
//

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "tensorflow/core"



int main(int argc, char *argv[]){

    int taskid, numtasks, numworkers, a, b,mtype,offset, rows;
    int dataSetSize = 20000;

    MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&taskid);
    MPI_Comm_size(MPI_COMM_WORLD,&numtasks);

    if (numtasks < 2 ) {
        printf("Need at least two MPI tasks. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }
    numworkers = numtasks-1;

    if(taskid == 0){
        //Master node


        //master initialize random weights for each neural net


        //send-recieve data
        int rows = dataSetSize/numworkers;
        int extra = dataSetSize%numworkers;
        int offset = 0;
        mtype = 1;
        for (int dest=1; dest<=numworkers; dest++)
        {
            rows = (dest <= extra) ? rows+1 : rows;
            printf("Sending %d rows to task %d offset=%d\n",rows,dest,offset);
            MPI_Send(&offset, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);
            MPI_Send(&rows, 1, MPI_INT, dest, mtype, MPI_COMM_WORLD);

            offset = offset + rows;
        }

        mtype = 1;
        for (int i=1; i<=numworkers; i++)
        {
            int source = i;
            MPI_Recv(&offset, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
            MPI_Recv(&rows, 1, MPI_INT, source, mtype, MPI_COMM_WORLD, &status);
            printf("Received results from task %d\n",source);
        }
    }
    else{
        //workers

        //receive data
        mtype = 1;
        MPI_Recv(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);
        MPI_Recv(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD, &status);

        //Data Training

        //send back data
        mtype = 1;
        MPI_Send(&offset, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);
        MPI_Send(&rows, 1, MPI_INT, MASTER, mtype, MPI_COMM_WORLD);

    }

    MPI_Finalize();

}