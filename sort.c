#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "mpi.h"
#include <limits.h>
#include <stdbool.h>
#include <string.h>
//#include <algorithm>

#define MASTER 0      /* taskid of first task */
#define FROM_MASTER 1 /* setting a message type */
#define FROM_WORKER 2 /* setting a message type */

#define BITONIC 0
#define MERGESORT 1
#define SAMPLESORT 2

double commTime, calcTime;

//Return largest d such that 2^d <= a
long log2(long a)
{
    long d = 0;
    while (a > 0)
    {
        d++;
        a >> 1;
    }
    return d;
}

void compAndSwap(int *arr, int i, int j, int order)
{
    if (order == (arr[i] > arr[j]))
    {
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void bitMerge(int *arr, int low, int cnt, int order)
{
    if (cnt > 1)
    {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++)
        {
            compAndSwap(arr, i, i + k, order);
        }
        bitMerge(arr, low, k, order);
        bitMerge(arr, low + k, k, order);
    }
}

void bitSort(int *arr, int low, int cnt, int order)
{
    if (cnt > 1)
    {
        int k = cnt / 2;
        bitSort(arr, low, k, 1);

        bitSort(arr, low + k, k, 0);

        bitMerge(arr, low, cnt, order);
    }
}

void merge(int *arr1, int *arr2, int n, int *dest)
{
    int i = 0, j = 0, k = 0;
    while (i < n && j < n)
    {
        if (arr1[i] < arr2[j])
        {
            dest[k++] = arr1[i++];
        }
        else
        {
            dest[k++] = arr2[j++];
        }
    }
    while (i < n)
    {
        dest[k++] = arr1[i++];
    }
    while (j < n)
    {
        dest[k++] = arr2[j++];
    }
}

void mergeSort(int *arr, int N)
{
    int *second = (int *)malloc(sizeof(int) * N);
    int *next = second;
    int *prev = arr;
    int *temp;
    for (int i = 2; i <= N; i *= 2)
    {
        for (int j = 0; j < N; j += i)
        {
            merge((int *)(prev + j), (int *)(prev + i / 2 + j), i / 2, (int *)(next + j));
        }
        if (i != N)
        {
            temp = prev;
            prev = next;
            next = temp;
        }
    }
    memcpy(arr, next, sizeof(int) * N);
    free(second);
}

static int intcompare(const void *i, const void *j)
{
    if ((*(int *)i) > (*(int *)j))
        return (1);
    if ((*(int *)i) < (*(int *)j))
        return (-1);
    return (0);
}

void quickSort(int *array, int low, int high)
{
    int i = low;
    int j = high;
    int pivot = array[(i + j) / 2];
    int temp;

    while (i <= j)
    {
        while (array[i] < pivot)
            i++;
        while (array[j] > pivot)
            j--;
        if (i <= j)
        {
            temp = array[i];
            array[i] = array[j];
            array[j] = temp;
            i++;
            j--;
        }
    }
    if (j > low)
        quickSort(array, low, j);
    if (i < high)
        quickSort(array, i, high);
}
void sampleSort(int *arr, int N)
{
    // Fudge not working
    quickSort(arr, 0, N);
}
/* type 0 - bitonic sort
 * type 1 - mergesort 
 * type 2 - sampleSort */

void sort(int *arr, int N, int order, int algorithm)
{
    if (algorithm == 0)
    {
        bitSort(arr, 0, N, order);
    }
    else if (algorithm == 1)
    {
        mergeSort(arr, N);
    }
    else if (algorithm == 2)
    {
        sampleSort(arr, N);
    }
}

bool isSorted(int *arr, int N)
{
    for (int i = 1; i < N; i++)
    {
        if (arr[i] < arr[i - 1])
        {
            return false;
        }
    }
    return true;
}

int max(int a, int b)
{
    return (a > b) ? a : b;
}

int min(int a, int b)
{
    return (a < b) ? a : b;
}

void compareLow(int *arr, int size, int taskid, int j)
{

    int pairP = taskid ^ (1 << j);
    double timeTemp = MPI_Wtime();
    //send max
    int *send = (int *)malloc((size + 1) * sizeof(int));
    MPI_Send(&arr[size - 1], 1, MPI_INT, pairP, 2, MPI_COMM_WORLD);

    //receive min
    int recvIndex;
    int *recv = (int *)malloc((size + 1) * sizeof(int));
    int min;
    MPI_Recv(&min, 1, MPI_INT, pairP, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    timeTemp = MPI_Wtime() - timeTemp;
    commTime += timeTemp;
    //printf("min: %d\n", min);

    timeTemp = MPI_Wtime();
    //sends values greather than min
    int sendIndex = 0;
    for (int i = size - 1; i >= 0; i--)
    {
        if (arr[i] > min)
        {
            send[sendIndex + 1] = arr[i];
            sendIndex++;
        }
        else
        {
            break;
        }
    }

    send[0] = sendIndex;
    //printf("%d\n", sendIndex);
    timeTemp = MPI_Wtime() - timeTemp;
    calcTime += timeTemp;

    timeTemp = MPI_Wtime();
    MPI_Send(send, size + 1, MPI_INT, pairP, 2, MPI_COMM_WORLD);
    MPI_Recv(recv, size + 1, MPI_INT, pairP, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    timeTemp = MPI_Wtime() - timeTemp;
    commTime += timeTemp;

    //for(int i = 0; i < size+1; i++){
    //	printf("Sent: %d\n", send[i]);
    //}
    //printf("\n");

    timeTemp = MPI_Wtime();
    //replaces the current max with the recv values if smaller
    int index = size - 1;
    for (int i = 1; i < recv[0] + 1; i++)
    {
        if (arr[index] > recv[i])
        {
            arr[index] = recv[i];
            index--;
        }
        else
        {
            break;
        }
    }

    //sort remaining
    sort(arr, size, 1, BITONIC);

    free(send);
    free(recv);
    timeTemp = MPI_Wtime() - timeTemp;
    calcTime += timeTemp;
}

void compareHigh(int *arr, int size, int taskid, int j)
{
    int pairP = taskid ^ (1 << j);
    //send min
    int *send = (int *)malloc((size + 1) * sizeof(int));
    double timeTemp = MPI_Wtime();
    MPI_Send(&arr[0], 1, MPI_INT, pairP, 2, MPI_COMM_WORLD);

    //receive max
    int recvIndex;
    int *recv = (int *)malloc((size + 1) * sizeof(int));
    int max;
    MPI_Recv(&max, 1, MPI_INT, pairP, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    timeTemp = MPI_Wtime() - timeTemp;
    commTime += timeTemp;

    //sends values less than max
    timeTemp = MPI_Wtime();
    int sendIndex = 0;
    for (int i = 0; i < size; i++)
    {
        if (arr[i] < max)
        {
            send[sendIndex + 1] = arr[i];
            sendIndex++;
        }
        else
        {
            break;
        }
    }

    send[0] = sendIndex; //send size
    timeTemp = MPI_Wtime() - timeTemp;
    calcTime += timeTemp;

    timeTemp = MPI_Wtime();
    MPI_Send(send, size + 1, MPI_INT, pairP, 2, MPI_COMM_WORLD);
    MPI_Recv(recv, size + 1, MPI_INT, pairP, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    timeTemp = MPI_Wtime() - timeTemp;
    commTime += timeTemp;

    //for(int i = 0; i < size+1; i++){
    //	printf("Received: %d\n", recv[i]);
    //}
    //printf("\n");
    timeTemp = MPI_Wtime();
    //replaces the current min with the recv values if bigger
    int index = 0;
    for (int i = 1; i < recv[0] + 1; i++)
    {
        if (recv[i] > arr[index])
        {
            arr[index] = recv[i];
            index++;
        }
        else
        {
            break;
        }
    }

    //sort remaining
    sort(arr, size, 1, BITONIC);
    free(send);
    free(recv);
    timeTemp = MPI_Wtime() - timeTemp;
    calcTime += timeTemp;
}

int main(int argc, char *argv[])
{

    long size;
    int type, algorithm, rc;
    if (argc == 4)
    {
        size = 1 << atoi(argv[1]);
        //printf("Size: %ld", size);
        type = atoi(argv[2]);
        algorithm = atoi(argv[3]);
        if (algorithm != 0 && algorithm != 1 && algorithm != 2)
        {
            printf("Sort options are 0 - bitonic, 1 - mergesort, and 2 - samplesort\n");
            return 0;
        }
    }
    else
    {
        printf("Exactly 3 arguments required to run\n");
        return 0;
    }

    int taskid, numtasks, numworkers, a, b, mtype, offset, numData;
    commTime = 0;
    calcTime = 0;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    if (numtasks < 1)
    {
        printf("Need at least one MPI task. Quitting...\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
        exit(1);
    }

    numworkers = numtasks;

    double timeWcalc = 0, timeTotal = 0, timeSR = 0, initTime = 0;

    //initialization
    initTime = MPI_Wtime();
    numData = size / numworkers;
    int nums[numData];

    for (int i = 0; i < numData; i++)
    {
        if (type == 0 || type == 1)
            nums[i] = i + (size / numworkers) * taskid + 1;
        else if (type == 2)
            nums[i] = (size - (size / numworkers) * (taskid)) - i;
    }
    /*
    int copy[numData];
    int slice = numData / numworkers;

    //Randomize order and spread among nodes
    if (type == 0)
    {
        std::random_shuffle(nums, (int *)(nums + numData));
        for (int i = 0; i < numworkers; i++)
        {
            if (i == taskid)
            {
                memcpy((int *)(copy + i * slice), (int *)(nums + i * slice), sizeof(int) * slice);
            }
            else
            {
                MPI_Send((int *)(nums + i * slice), slice, MPI_LONG, i, 0, MPI_COMM_WORLD);
                MPI_Recv((int *)(copy + i * slice), slice, MPI_LONG, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        memcpy(nums, copy, numData);
    }
    if (taskid == 0)
    {
        for (int i = 0; i < numData; i++)
        {
            printf("%d ", nums[i]);
        }
        printf("\n");
    }
    //return 0;
    */

    initTime = MPI_Wtime() - initTime;
    timeTotal = MPI_Wtime();

    if (algorithm == BITONIC)
    {
        //sorting
        sort(nums, numData, 1, BITONIC);
        calcTime += MPI_Wtime() - timeTotal;

        //bitonic (between nodes)
        for (int i = 0; i < log2(numworkers); i++)
        {
            for (int j = i; j >= 0; j--)
            {
                if (((taskid >> (i + 1)) % 2 == 0 && (taskid >> j) % 2 == 0) || ((taskid >> (i + 1)) % 2 != 0 && (taskid >> j) % 2 != 0))
                {
                    compareLow(nums, numData, taskid, j);
                }
                else
                {
                    compareHigh(nums, numData, taskid, j);
                }
            }
        }
    }
    else if (algorithm == MERGESORT)
    {
        sort(nums, numData, 1, MERGESORT);
        calcTime += MPI_Wtime() - timeTotal;
    }
    else if (algorithm == SAMPLESORT)
    {
        /*
        //Ref : https://github.com/peoro/Parasort/blob/master/doc/Sorting_Algorithms/Sample%20Sort/samplesort.c
        int i, j, k, NoofElements_Bloc, NoElementsToSort;
        int count, temp;
        int *InputData;
        int *Splitter, *AllSplitter;
        int *Buckets, *BucketBuffer, *LocalBucket;
        int *OutputBuffer, *Output;

        NoofElements_Bloc = numData / numworkers;
        InputData = (int *)malloc(NoofElements_Bloc * sizeof(int));
        MPI_Bcast(&numData, 1, MPI_INT, 0, MPI_COMM_WORLD);

        MPI_Scatter(nums, NoofElements_Bloc, MPI_INT, InputData, NoofElements_Bloc, MPI_INT, 0, MPI_COMM_WORLD);

        qsort((int *)InputData, NoofElements_Bloc);

        MPI_Gather(InputData, numData, MPI_INT, Output, numData, MPI_INT, 0, MPI_COMM_WORLD);

        Splitter = (int *)malloc(sizeof(int) * (numtasks - 1));
        for (i = 0; i < (numtasks - 1); i++)
        {
            Splitter[i] = InputData[numData / (numtasks * numtasks) * (i + 1)];
        }

        AllSplitter = (int *)malloc(sizeof(int) * numtasks * (numtasks - 1));
        MPI_Gather(Splitter, numtasks - 1, MPI_INT, AllSplitter, numtasks - 1,
                   MPI_INT, 0, MPI_COMM_WORLD);

        if (taskid == 0)
        {
            qsort((int *)AllSplitter, numtasks * (numtasks - 1));

            for (i = 0; i < numtasks - 1; i++)
                Splitter[i] = AllSplitter[(numtasks - 1) * (i + 1)];
        }

        MPI_Bcast(Splitter, numtasks - 1, MPI_INT, 0, MPI_COMM_WORLD);

        Buckets = (int *)malloc(sizeof(int) * (numData + numtasks));

        j = 0;
        k = 1;

        for (i = 0; i < NoofElements_Bloc; i++)
        {
            if (j < (numtasks - 1))
            {
                if (InputData[i] < Splitter[j])
                    Buckets[((NoofElements_Bloc + 1) * j) + k++] = InputData[i];
                else
                {
                    Buckets[(NoofElements_Bloc + 1) * j] = k - 1;
                    k = 1;
                    j++;
                    i--;
                }
            }
            else
                Buckets[((NoofElements_Bloc + 1) * j) + k++] = InputData[i];
        }
        Buckets[(NoofElements_Bloc + 1) * j] = k - 1;

        BucketBuffer = (int *)malloc(sizeof(int) * (numData + numtasks));
        MPI_Alltoall(Buckets, NoofElements_Bloc + 1, MPI_INT, BucketBuffer,
                     NoofElements_Bloc + 1, MPI_INT, MPI_COMM_WORLD);

        LocalBucket = (int *)malloc(sizeof(int) * 2 * numData / numtasks);

        count = 1;

        for (j = 0; j < numtasks; j++)
        {
            k = 1;
            for (i = 0; i < BucketBuffer[(numData / numtasks + 1) * j]; i++)
                LocalBucket[count++] = BucketBuffer[(numData / numtasks + 1) * j + k++];
        }
        LocalBucket[0] = count - 1;

        NoElementsToSort = LocalBucket[0];
        qsort((int *)&LocalBucket[1], NoElementsToSort);

        if (taskid == 0)
        {
            OutputBuffer = (int *)malloc(sizeof(int) * 2 * numData);
            Output = (int *)malloc(sizeof(int) * numData);
        }

        MPI_Gather(LocalBucket, 2 * NoofElements_Bloc, MPI_INT, OutputBuffer,
                   2 * NoofElements_Bloc, MPI_INT, 0, MPI_COMM_WORLD);

        if (taskid == 0)
        {
            count = 0;
            for (j = 0; j < numtasks; j++)
            {
                k = 1;
                for (i = 0; i < OutputBuffer[(2 * numData / numtasks) * j]; i++)
                    Output[count++] = OutputBuffer[(2 * numData / numtasks) * j + k++];
            }
            free(OutputBuffer);
            free(Output);
        }
        free(InputData);
        free(Splitter);
        free(AllSplitter);
        free(Buckets);
        free(BucketBuffer);
        free(LocalBucket);
        */
        sort(nums, numData, 1, SAMPLESORT);
        calcTime += MPI_Wtime() - timeTotal;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    timeTotal = MPI_Wtime() - timeTotal;
    double calcAvg, totalAvg, commAvg, initAvg;

    //getting average time taken
    MPI_Reduce(&calcTime, &calcAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&timeTotal, &totalAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&commTime, &commAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&initTime, &initAvg, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    calcAvg /= numtasks;
    totalAvg /= numtasks;
    commAvg /= numtasks;
    initAvg /= numtasks;

    if (taskid == 0 && algorithm == BITONIC)
    {
        printf("Bitonic sort %d nodes and 2^%d size with type %d\n", numtasks, (int)log2(size), type);
        printf("%f\n", totalAvg);
        printf("%f\n", calcAvg);
        printf("%f\n", commAvg);
        printf("%f\n", initAvg);
    }
    else if (taskid == 0 && algorithm == MERGESORT)
    {
        printf("%d, %ld, %d, %.6f, %.6f, %.6f, %.6f\n", numtasks, size, type, totalAvg, calcAvg, commAvg, initAvg);
    }
    else if (taskid == 0 && algorithm == SAMPLESORT)
    {
        printf("%d, %ld, %d, %.6f, %.6f, %.6f, %.6f\n", numtasks, size, type, totalAvg, calcAvg, commAvg, initAvg);
    }

    //for(int i = 0; i < numData; i++){
    //	printf("Task: %d, %d\n", taskid, nums[i]);
    //}

    MPI_Finalize();
}
