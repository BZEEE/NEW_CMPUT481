
#include <iostream>
#include <time.h>
#include <cstdlib>
#include <pthread.h>
#include <cmath>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <random>

// g++ sharedMemory.cpp -std=c++11 -lpthread

using namespace std;

struct thread_data {
    int threadId;
    long int *list;
    // int listStartIndex;
    long int listSize;
};

struct thread_data_for_merge {
    int threadId;
    long int *resultArray;
    long int *list;
    long int listSize;
    long int partitionSize;
    long int numThreads;
    long int minPivot;
    long int maxPivot;
    long int exchangeSize;
};

struct section {
    long int startIndex;
    long int size;
};

int compare(const void* a, const void* b) {
    // returns a negative integer if the first argument is less than the second
    // returns a positive integer if the first argument is greater than the second
    // returns zero if they are equal
	const long int* x = (long int*) a;
	const long int* y = (long int*) b;

	if (*x > *y) {
        return 1;
    }	
	else if (*x < *y) {
        return -1;
    } else {
        return 0;
    }
}

void *sortOnThread(void *threadData) {
    // get args passed to thread
    struct thread_data *threadArgs;
    threadArgs = (struct thread_data *) threadData;
    int id = threadArgs->threadId;
    long int* array = threadArgs->list;
    long int listSize = threadArgs->listSize;
    // perform quick sort on partitioned data using quicksort, sorts in place
    qsort(array, listSize, sizeof(long int), compare);
    //close the thread
    pthread_exit(NULL);
}

void *mergeOnThread(void *threadData) {
    // create a new vector
    struct thread_data_for_merge *threadArgs;
    threadArgs = (struct thread_data_for_merge *) threadData;
    int id = threadArgs->threadId;
    long int *resultArray = threadArgs->resultArray;
    long int *list = threadArgs->list;
    long int listSize = threadArgs->listSize;
    long int partitionSize = threadArgs->partitionSize;
    long int numThreads = threadArgs->numThreads;
    long int minPivot = threadArgs->minPivot;
    long int maxPivot = threadArgs->maxPivot;

    struct section sections[numThreads];
    // find all exchange sections from each partition for this particular processor
    long int left;
    long int right;
    bool leftStop;
    bool rightStop;
    long int size;
    long int totalSize = 0;
    long int sectionSize;
    for (long int i = 0; i < numThreads; i++) {
        if (i == numThreads - 1) {
            sectionSize = listSize - (partitionSize * (numThreads - 1));
        } else {
            sectionSize = partitionSize;
        }
        // use a sliding window approach
        left = (i * partitionSize);
        right = (i * partitionSize) + sectionSize - 1;
        leftStop = false;
        rightStop = false;
        while ( left <= right && !(leftStop && rightStop) ) {
            if (list[left] > minPivot) {
                leftStop = true;
            } else {
                left++;
            }
            if (list[right] <= maxPivot) {
                rightStop = true;
            } else {
                right--;
            }
        }
        // cout << "left: " << left << "right: " << right << "\n";
        size = right - left + 1;
        totalSize = totalSize + size;
        // sections[i] = new section(left, size);
        struct section sec;
        sec.startIndex = left;
        sec.size = size;
        sections[i] = sec;
        // cout << "processor: " << id << " section: " << sections[i].startIndex << " " << sections[i].size << " left: " << left << " right: " << right << "\n";
    }
    cout << "processor: " << id << " total size: " << totalSize << "\n";
    // merge sections
    resultArray = (long int *) realloc(resultArray, totalSize);
    long int partitionCounter = 0;
    for (int i = 0; i < numThreads; i++) {
        merge(resultArray, resultArray + partitionCounter, list + sections[i].startIndex, list + sections[i].startIndex + sections[i].size, resultArray);
        partitionCounter = partitionCounter + sections[i].size;
    }
    threadArgs->exchangeSize = totalSize;
    // cout << threadArgs->exchangeSize << " \n";
    
    // long int sectionSize;
    // long int item;
    // // merge partitions together and sort elements as they are inserted
    // for (long int i = 0; i < numThreads; i++) {
    //     if (i == numThreads - 1) {
    //         sectionSize = listSize - (partitionSize * (numThreads - 1));
    //     } else {
    //         sectionSize = partitionSize;
    //     }
    //     for (long int j = 0; j < sectionSize; j++) {
    //         // cout << "access array item: " << (i * partitionSize) + j << "\n";
    //         item = list[(i * partitionSize) + j];
    //         // perform a sort as we merge
    //         if (item > minPivot && item <= maxPivot) {
    //             // insert item at beginning
    //             resultArray->insert(resultArray->begin(), item);
    //             // always keep vector sorted
                
                
    //         }
    //     }
    // }
    // sort(resultArray->begin(), resultArray->end());
    // cout << "thread exit: processor " << id << "\n";
    //close the thread
    pthread_exit(NULL);
}

int main() {
    // generate a random array of 1 million integers
    // use time as the seed since it is a number that changes all the time
    // default_random_engine generator;
    srand(time(NULL));
    
    const long int listSize = 16;
    long int *array = new long int[listSize];
    long int MAX_NUM = 2147483647;
    for (long int i = 0; i < listSize; i++) {
        // array[i] = ((unsigned long int) (rand() * rand() * rand())) % MAX_NUM;
        array[i] = ((unsigned long int) (rand() * rand() * rand())) % 50;
    }

    // display unsorted list
    cout << "\nBefore:\n";
    for(int i = 0; i < listSize; i++) {
        cout << array[i] << " ";
    };
    cout << "\n";

    // set parameters
    const long int numThreads = 4;
	pthread_t phase1Threads [numThreads];
    struct thread_data phase1ThreadData[numThreads];
    // set start and end partition indices
    long int *start = &array[0];
    long int partitionSize = floor(listSize / numThreads);

    // start the clock
    clock_t startClock = clock(); 

    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 1 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // pass (n/p) items to each processor, create a thread for each process
    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        phase1ThreadData[i].threadId = i;
        phase1ThreadData[i].list = &array[i * partitionSize];
        // threadData[i].listStartIndex = i * partitionSize;
        // the list and number of processors isnt always evenly divisible
        // have to account for the last array
        if (i == numThreads - 1) {
            phase1ThreadData[i].listSize = listSize - (i * partitionSize);
        } else {
            phase1ThreadData[i].listSize = partitionSize;
        };
        pthread_create(&phase1Threads[i], NULL, sortOnThread, (void *) &phase1ThreadData[i]);
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        // pthread_join(phase1Threads[i], NULL);
    }

    // join all the threads together to sync end of phase 1 with beginning of phase 2
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase1Threads[i], NULL);
    }

    // stop the clock for Phase 1
    clock_t phase1Stop = clock();

    cout << "\nAfter Phase 1:\n";
    for(int i = 0; i < listSize; i++) {
        cout << array[i] << " ";
    };
    cout << "\n";


    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 2 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // pick (number of processors - 1) pivots from each thread partition

    long int sampleSize = numThreads * numThreads;
    long int gatheredRegularSample[sampleSize];
    long int partitionPivotSeparation = floor(partitionSize / numThreads);
    for (long int i = 0; i < numThreads; i++) {
        for (long int j = 0; j < numThreads; j++) {
            // create a smaller array with all the pivot values from the partitioned array
            gatheredRegularSample[(i * numThreads) + j] = array[(i * partitionSize) + (j * partitionPivotSeparation)];
        }
    }
    // sort the smaller pivot array, sorts in place
    qsort(gatheredRegularSample, sampleSize, sizeof(long int), compare);

    // pick new pivots from pivot Array
    long int regularSamplePivots[numThreads-1];
    long int regularSamplePivotSeparation = floor(sampleSize / numThreads);
    for (long int i = 1; i < numThreads; i++) {
        regularSamplePivots[i-1] = gatheredRegularSample[i * regularSamplePivotSeparation];
        // cout << regularSamplePivots[i-1] << " ";
    }

    // stop the clock for Phase 2
    clock_t phase2Stop = clock();
    
    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 3 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // exchange partitions with each processor based on regular sample pivots
    // loop through list size_t
    // but know when to compare to a certain pivot

    cout << "pivots: ";
    for (int i = 0; i < numThreads - 1; i++) {
        cout << regularSamplePivots[i] << " ";
    }
    // cout << partitionSize << "\n";
    cout << "\n";

    pthread_t phase3Threads [numThreads];
    struct thread_data_for_merge phase3ThreadData[numThreads];
    // let each thread merge its parts based on the pivots
    long int * processorResults[numThreads];
    // std::vector<std::vector<int>> v(10, std::vector<int>(5));

    for (long int i = 0; i < numThreads; i++) {
        // segment list into (list size / number of threads) partitions, pass each partition to a thread to sort it
        // processorResults[i] = vector<long int>(0);
        // thread arguments
        processorResults[i] = (long int *) malloc(0 * sizeof(int));

        phase3ThreadData[i].threadId = i;
        phase3ThreadData[i].resultArray = processorResults[i];
        phase3ThreadData[i].list = array;
        phase3ThreadData[i].listSize = listSize;
        phase3ThreadData[i].partitionSize = partitionSize;
        phase3ThreadData[i].numThreads = numThreads;
        phase3ThreadData[i].exchangeSize = 0;

        if (i == 0) {
            phase3ThreadData[i].minPivot = -1;
            phase3ThreadData[i].maxPivot = regularSamplePivots[i];
        } else if (i == numThreads - 1) {
            phase3ThreadData[i].minPivot = regularSamplePivots[i-1];
            phase3ThreadData[i].maxPivot = MAX_NUM;
        } else {
            phase3ThreadData[i].minPivot = regularSamplePivots[i-1];
            phase3ThreadData[i].maxPivot = regularSamplePivots[i];
        }

        pthread_create(&phase3Threads[i], NULL, mergeOnThread, (void *) &phase3ThreadData[i]);
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
    }

    // join all the threads together to sync end of phase 1 with beginning of phase 2
    for (long int i = 0; i < numThreads; i++) {
        // allows us to wait till all threads are completed in the group before continuing to do the sequential part of our algorithms
        pthread_join(phase3Threads[i], NULL);
    }

    // cout << "\nAfter Phase 3\n";
    // for (long int i = 0; i < numThreads; i++) {
    //     cout << "processor: " << i << " exchange size" << phase3ThreadData[i].exchangeSize << "\n";
    // }
    // cout << "\n";


    long int counter = 0;
    for (long int i = 0; i < numThreads; i++) {
        cout << "processor: " << i << " exchange size" << phase3ThreadData[i].exchangeSize << "\n";
        for (int j = 0; j < phase3ThreadData[i].exchangeSize; j++) {
            cout << phase3ThreadData[i].resultArray[j] << " ";
            array[counter] = phase3ThreadData[i].resultArray[j];
            counter++;
        }
        cout << "\n";
    }


    // stop the clock for Phase 3
    clock_t phase3Stop = clock();

    // for (int i = 0; i < numThreads; i++) {
    //     for (int j = 0; j < exchangedPartitions[i].size(); j++) {
    //         cout << exchangedPartitions[i][j] << " ";
    //     };
    //     cout << "\n";
    // }


    // --------------------------------------------------------------------------------------------------------//
    // ------------------------------------- Phase 4 ----------------------------------------------------------//
    // --------------------------------------------------------------------------------------------------------//
    // merger the vectors back to the main array memory slots
    // this allows us to skip the merge step for phase 4, since
    // the exchanged partitions or sorted in place right next to each other
    // by each process

    // concatenate pointers together
    // long int arrayCounter = 0;
    // for (int i = 0; i < numThreads; i++) {
    //     for (long int num : processorResults[i]) {
    //         array[arrayCounter] = num;
    //         arrayCounter++;
    //     }
    // }

    //print sorted list
    // cout << "\nAfter Phase 4\n";
    // for (long int i = 0; i < listSize; i++) {
	// 	cout << array[i] << " ";
	// }
    // cout << "\n";

    // stop the clock for Phase 4
    clock_t phase4Stop = clock();

    // Calculating total time taken by the program. 
    cout << "Time taken for Phase 1: " << fixed  << double(phase1Stop - startClock) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 2: " << fixed  << double(phase2Stop - phase1Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 3: " << fixed  << double(phase3Stop - phase2Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;
    cout << "Time taken for Phase 4: " << fixed  << double(phase4Stop - phase3Stop) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl; 
    cout << "Total time: " << fixed  << double(phase4Stop - startClock) / double(CLOCKS_PER_SEC) << setprecision(5); cout << " seconds " << endl;  

    // print sorted array
    // cout << "\nAfter:\n";
    // for(int i = 0; i < listSize; i++) {
    //     cout << array[i] << " ";
    // };
    // cout << "\n";


    // cout << "\nTest Correctness:\n";
    // for(long int i = 0; i < listSize - 1; i++) {
    //     if (array[i] > array[i+1]) {
    //         cout << "false\n";
    //     }
    // };
    // cout << "\n";

    pthread_exit(NULL); // last thing that main should do

  
    
    // return 0;
}


