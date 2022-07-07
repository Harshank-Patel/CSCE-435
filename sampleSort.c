#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mpi.h"
#include <limits.h>
#include <stdbool.h>
#include <string.h>

#define MASTER 0 /* rank of first task */

#define TRAND 0
#define TORDE 1
#define TREVE 2

/*
a : unsorted data
sz : size of data
k : oversampling factor = p
p : numSplitters [Usually number of processors availible
Note : k*p should never be over sz
*/

int binSearch(int arr[], int l, int r, int x)
{
    // Returns the index of the bucket it is to be placed within.
    if (r > l)
    {
        int mid = l + (r - l) / 2;
        if (arr[mid] > x)
        {
            return binSearch(arr, l, mid - 1, x);
        }
        else
        {
            return binSearch(arr, mid + 1, r, x);
        }
    }
    return l;
}
static int intcompare(const void *i, const void *j)
{
    if ((*(int *)i) > (*(int *)j))
        return (1);
    if ((*(int *)i) < (*(int *)j))
        return (-1);
    return (0);
}

// Make and gather splitters (P-1) as it goes from [-Inf,s1,s2,s3...,sp-1,Inf]
// All processes recieve the array and reports back with P splitters from evenly* spaced out splitter elements sampled from the array subsection
// All processors report back to the master node their findings
// Sort the splitters via qsort
// Determine the pivots/splitters by traversing the array and incrementing the space by P-1
// Sends buckets for all processes to use along with the section of the array it is to sort
// Send back out all the splitters
// Processors now utilize binary search on each element to sort them into each bucket
// Buckets are all send back root process
// Buckets are then distributed to each process to run quicksort upon.
// Once all are sorted, the buckets are sent back to the root process and conglomerated.

int main(int argc, char *argv[])
{
    int **bucketCollect;
    int *splitSampled, *recievedArray, *data, *splitRefined, *sCount, *splitTotal, *bucketAmt, *bucketFinalized, *recDispl, *bucketDispl, *bucketAmtDispl, *recAmt, *finalBlockDispl, *dataFinal, *finalBlockSz, *recievedArrayFinal;
    int taskid, p, type, blockSz, rc, ind, total, sz, w;
    double commTime, calcTime, tempTime;
    commTime = 0;
    calcTime = 0;
    // Size
    sz = 1 << atoi(argv[1]);
    // Type 1 : Random Num Array
    // Type 2 : Ordered Array
    // Type 3 : Reverse Ordered Array
    type = atoi(argv[2]);

    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    data = (int *)malloc(sizeof(int) * (sz));

    double timeWcalc = 0, timeTotal = 0, timeSR = 0, initTime = 0;
    initTime = MPI_Wtime();
    if (taskid == MASTER)
    {
        // printf("Starting Init\n");
        //  Start init timer
        //  Start Array Init
        if (type == TRAND)
        {
            for (int q = 0; q < sz; q++)
            {
                data[q] = rand() % 100 + 1;
                // printf("%d, ", data[q]);
            }
        }
        else if (type == TORDE)
        {
            for (int q = 0; q < sz; q++)
            {
                data[q] = q;
                // printf("%d, ", data[q]);
            }
        }
        else
        {
            // Type == TReverse
            for (int q = 0; q < sz; q++)
            {
                data[q] = sz - q - 1;
                // printf("%d, ", data[q]);
            }
        }
        // printf("\n\n");
    }
    initTime = MPI_Wtime() - initTime;

    if (sz <= p)
    {
        if (taskid == MASTER)
        {
            qsort(data, sz, sizeof(int), intcompare);
            for (int q = 0; q < sz; q++)
            {
                // printf(" %d", data[q]);
            }
        }
        MPI_Finalize();
        return 0;
    }

    // End Array Init
    // End init timer

    timeTotal = MPI_Wtime();
    blockSz = sz / p;
    // tempTime = MPI_Wtime();
    // MPI_Bcast(&blockSz, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // tempTime = MPI_Wtime() - tempTime;
    // commTime += tempTime;
    recievedArray = (int *)malloc(sizeof(int) * (blockSz));
    tempTime = MPI_Wtime();

    MPI_Scatter(data, blockSz, MPI_INT, recievedArray, blockSz, MPI_INT, 0, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    // printf("[%d] : BCast and Scatter\n", taskid);
    splitSampled = (int *)malloc(sizeof(int) * (p - 1));

    for (int q = 0; q < p - 1; q++)
    {
        splitSampled[q] = recievedArray[q * p];
        // printf("%d\n", splitSampled[q]);
    }

    free(data);
    splitTotal = (int *)malloc(sizeof(int) * (p * (p - 1)));

    // printf("[%d] : MPI_Barrier0\n", taskid);
    // MPI_Barrier(MPI_COMM_WORLD);
    tempTime = MPI_Wtime();

    MPI_Gather(splitSampled, p - 1, MPI_INT, splitTotal, p - 1, MPI_INT, 0, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    free(splitSampled);
    // printf("[%d] : MPI_Barrier1\n", taskid);
    // MPI_Barrier(MPI_COMM_WORLD);
    splitRefined = (int *)malloc(sizeof(int) * (p - 1));
    if (taskid == MASTER)
    {
        qsort(splitTotal, p * (p - 1), sizeof(int), intcompare);
        w = 0;
        for (unsigned int q = 1; q < p * p; q += p)
        {
            splitRefined[w++] = splitTotal[q];
        }
    }
    free(splitTotal);
    tempTime = MPI_Wtime();

    // printf("[%d] : MPI_BCast1\n", taskid);
    MPI_Bcast(splitRefined, p - 1, MPI_INT, 0, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    // All Processes have the splitters
    bucketCollect = (int **)malloc(sizeof(int *) * p);

    for (int q = 0; q < p; q++)
    {
        bucketCollect[q] = (int *)malloc(sizeof(int) * blockSz);
    }
    bucketAmt = (int *)malloc(sizeof(int) * (p));
    for (int q = 0; q < p; q++)
    {
        bucketAmt[q] = 0;
    }
    for (int q = 0; q < blockSz; q++)
    {
        // printf("[%d] : PriorBinSearch x = %d \n", taskid, recievedArray[q]);
        ind = binSearch(splitRefined, 0, p - 1, recievedArray[q]);
        // printf("[%d] : Index %d \n", taskid, ind);
        bucketCollect[ind][bucketAmt[ind]] = recievedArray[q];
        // printf("[%d] : Value %d \n", taskid, bucketCollect[ind][bucketAmt[ind]]);
        bucketAmt[ind]++;
    }
    free(splitRefined);

    // printf("[%d] : MPI_Barrier1.5\n", taskid);
    // tempTime = MPI_Wtime();

    // MPI_Barrier(MPI_COMM_WORLD);
    // tempTime = MPI_Wtime() - tempTime;
    // commTime += tempTime;
    // Darnit, I can't send it by 2d array into MPIALLTOALL, it must be in a 1d array
    // Reuse recievedArray to send it accordingly
    int recievedArrayItr = 0;
    bucketDispl = (int *)malloc(sizeof(int) * (p));
    for (int q = 0; q < p; q++)
    {
        int w = bucketAmt[q];
        for (int e = 0; e < w; e++)
        {
            recievedArray[recievedArrayItr++] = bucketCollect[q][e];
        }
        if (q > 0)
        {
            bucketDispl[q] = bucketDispl[q - 1] + bucketAmt[q - 1];
        }
        else
        {
            bucketDispl[0] = 0;
        }
    }
    for (int q = 0; q < p; q++)
    {
        free(bucketCollect[q]);
    }
    free(bucketCollect);
    // Notify all processes the amount each proccess will send to each other for accurate recieve amount recieve displacement
    recAmt = (int *)malloc(sizeof(int) * (p));
    // tempTime = MPI_Wtime();
    tempTime = MPI_Wtime();

    MPI_Alltoall(bucketAmt, 1, MPI_INT, recAmt, 1, MPI_INT, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    // printf("[%d] : MPI_Barrier2\n", taskid);
    // MPI_Barrier(MPI_COMM_WORLD);
    // tempTime = MPI_Wtime() - tempTime;
    // commTime += tempTime;
    total = 0;
    recDispl = (int *)malloc(sizeof(int) * (p));

    for (int q = 0; q < p; q++)
    {
        recDispl[q] = total;
        total += recAmt[q];
    }

    recievedArrayFinal = (int *)malloc(sizeof(int) * (total));
    tempTime = MPI_Wtime();

    MPI_Alltoallv(recievedArray, bucketAmt, bucketDispl, MPI_INT, recievedArrayFinal, recAmt, recDispl, MPI_INT, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    free(recievedArray);
    free(recDispl);
    free(recAmt);
    free(bucketDispl);
    free(bucketAmt);
    // printf("[%d] : MPI_Barrier3\n", taskid);
    // MPI_Barrier(MPI_COMM_WORLD);
    // Each process now has its own work
    qsort(recievedArrayFinal, total, sizeof(int), intcompare);
    // printf("[%d] : MPI_Barrier4\n", taskid);
    // MPI_Barrier(MPI_COMM_WORLD);
    finalBlockSz = (int *)malloc(sizeof(int) * (p));
    tempTime = MPI_Wtime();

    MPI_Gather(&total, 1, MPI_INT, finalBlockSz, 1, MPI_INT, 0, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    // Provides the varying block sizes of each process so the ultimate gather is accurate.
    finalBlockDispl = (int *)malloc(sizeof(int) * (p));

    if (taskid == MASTER)
    {
        for (int q = 0; q < p; q++)
        {
            if (q > 0)
            {
                finalBlockDispl[q] = finalBlockDispl[q - 1] + finalBlockSz[q - 1];
            }
            else
            {
                finalBlockDispl[q] = 0;
            }
        }
    }
    dataFinal = (int *)malloc(sizeof(int) * sz);
    tempTime = MPI_Wtime();

    MPI_Gatherv(recievedArrayFinal, total, MPI_INT, dataFinal, finalBlockSz, finalBlockDispl, MPI_INT, 0, MPI_COMM_WORLD);
    tempTime = MPI_Wtime() - tempTime;
    commTime += tempTime;
    calcTime += MPI_Wtime() - timeTotal;
    timeTotal = MPI_Wtime() - timeTotal;
    calcTime = timeTotal - commTime;
    // if (taskid == MASTER){
    //     for (int q = 0; q < sz; q+=blockSz){
    //         printf("%d ", dataFinal[q]);
    //     }
    //     printf("\n\n\n");
    // }
    free(recievedArrayFinal);
    free(finalBlockSz);
    free(finalBlockDispl);
    free(dataFinal);

    double calcAvg, totalAvg, commAvg, initAvg;

    // getting average time taken
    MPI_Reduce(&calcTime, &calcAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&timeTotal, &totalAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&commTime, &commAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&initTime, &initAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    calcAvg /= p;
    totalAvg /= p;
    commAvg /= p;
    initAvg /= p;
    if (taskid == MASTER)
    {
        printf("%d, %ld, %d, %.6f, %.6f, %.6f, %.6f\n", p, sz, type, totalAvg, calcAvg, commAvg, initAvg);
    }
    MPI_Finalize();
    return 0;
}
