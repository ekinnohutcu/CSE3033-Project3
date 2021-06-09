// Ekin Nohutçu 150116067
// Merve Ayer  150119828
// Zeynep Naz Akyokuş 150119073

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <ctype.h>

//creating mutex and semaphore
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
sem_t semaphore_package;





struct queue {
    unsigned int tail;	    // current tail
    unsigned int head;	    // current head
    unsigned int size;	    // current number of items
    unsigned int capacity;      // Capacity of queue
    int* data; 		    // Pointer to array of data
};

// Create Global defenition of queue_t
typedef struct queue queue_t;

// function to create queue
queue_t* create_queue(unsigned int _capacity){
    queue_t* myQueue = (queue_t*)malloc(sizeof(queue_t)); // allocate memory of size of queue struct

    if (myQueue == NULL ){
        return NULL; // if malloc was unsuccesful return NULL
    } else {
        // populate the variables of the queue :
        myQueue->tail = -1;
        myQueue->head = 0;
        myQueue->size = 0;
        myQueue->capacity = _capacity;
        myQueue->data = (int*)malloc(_capacity * sizeof(int)); // allocate memory for the array

        return myQueue;
    }
}
// To double the size of the buffer.
queue_t* queue_resize(queue_t* q, unsigned int _capacity){
    q->capacity = _capacity;
    q->data = (int*)malloc(_capacity * sizeof(int));
    return q;
}

// Check whether queue is empty or not.
int queue_empty(queue_t* q){
    if (q == NULL){
        return -1;
    }else if(q->size == 0) {
        return 1;
    }else {
        return 0;
    }
}

// Remove from queue
int queue_dequeue(queue_t *q){

    if (q == NULL){
        return -1;

    }	else if (queue_empty(q) == 1){
        return 0;
    }else{
        // first capture the item
        int item = q->data[q->head];
        q->head = (q->head + 1) % q->capacity;
        // decrease size by 1
        q->size--;

        return item;
    }
}
// Get the count of the items in queue
unsigned int queue_size(queue_t* q){
    if (q == NULL){
        return - 1;
    } else {
        return q->size;
    }
}

// Check whether queue is full or not
int queue_full(queue_t* q){
    if (q == NULL){
        return -1;
    }else if(q->size == q->capacity){
        return 1;
    }else{
        return 0;
    }
}

// Put new item to queue
int queue_enqueue(queue_t* q, int item){

    if (q == NULL){
        return -1;
    }	else if (queue_full(q) == 1){
        // make sure the queue isnt full.
        return 0;
    } else {
        // first we move the tail (insert) location up one (in the circle (size related to _capacity))
        q->tail = (q->tail + 1) % q->capacity; // this makes it go around in a circle
        // now we can add the actual item to the location
        q->data[q->tail] = item;
        // now we have to increase the size.
        q->size++;
        return 1;
    }
}

// To store type of the thread
struct thread_type{
    int publisher_type;
    pthread_t* thread;
};

// To store books
struct book
{
    long thread_id;
    queue_t* queue;
    int publisher_type;
    int book_number;
};

// to put books to buffers (Producer)
void* PutToBuffer(void* Book)
{
    struct book* b=(struct book*) Book;
    long b_thread_id=b->thread_id;
    int b_type=b->publisher_type;
    int b_number=b->book_number;
    queue_t* b_queue=b->queue;


    if (queue_full(b_queue)==1){
        int sizeQ=queue_size(b_queue);
        sizeQ=sizeQ*2;
        queue_resize(b_queue,sizeQ);
    }

    queue_enqueue(b_queue,b_number);

    printf("Publisher %ld of type %d   Book%d_%d is published and put into the buffer %d \n",b_thread_id,b_type, b_type,queue_size(b_queue), b_type);
    pthread_exit(NULL);
}

//  to put item to package (Consumer)
void* putToPackage(void* queue)
{
    int item =queue_dequeue(queue);
    printf("Put Book%d_1 into the package. \n",item);
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {
    if (argc != 10){ /* check for valid number of command-line arguments */
        fprintf(stderr, "ERROR: Usage: ./project3.out -n publisherType publisherThreadCount packagerThreadCount -b numberOfBooks -s packageSize initialBufferSize\n");
        return 1;
    }

    else if (strcmp(argv[1], "-n") != 0){ /* check for valid character */
        fprintf(stderr, "ERROR: Usage: ./project3.out -n publisherType publisherThreadCount packagerThreadCount -b numberOfBooks -s packageSize initialBufferSize\n");
        return 1;
    }

    else if (strcmp(argv[5], "-b") != 0){ /* check for valid character */
        fprintf(stderr, "ERROR: Usage: ./project3.out -n publisherType publisherThreadCount packagerThreadCount -b numberOfBooks -s packageSize initialBufferSize\n");
        return 1;
    }

    else if (strcmp(argv[7], "-s") != 0){ /* check for valid character */
        fprintf(stderr, "ERROR: Usage: ./project3.out -n publisherType publisherThreadCount packagerThreadCount -b numberOfBooks -s packageSize initialBufferSize\n");
        return 1;
    }

    // Store the values taken from command line arguments
    int PUBLISHER_TYPE = atoi(argv[2]);
    int PUBLISHER_THREAD_COUNT = atoi(argv[3]);
    int PACKAGER_THREAD_COUNT = atoi(argv[4]);
    int NUMBER_OF_BOOKS = atoi(argv[6]);
    int PACKAGE_SIZE = atoi(argv[8]);
    int INITIAL_BUFFER_SIZE = atoi(argv[9]);


    // Publisher thread initialization
    pthread_t publisherThreads [PUBLISHER_THREAD_COUNT*PUBLISHER_TYPE];
    // Package thread initialization
    pthread_t packageThreads [PACKAGER_THREAD_COUNT];
    // create package queue
    queue_t* package=create_queue(PACKAGE_SIZE);


    //mutex initialization
    if (pthread_mutex_init(&lock, NULL) != 0){
        printf("\n Mutex init has failed\n");
        return 1;
    }

    //semaphore_package initialization
    sem_init(&semaphore_package, 0, PACKAGE_SIZE);
    //buffer queue array initialization
    queue_t* buffer[PUBLISHER_TYPE];
    // semaphore_buffer initialization
    sem_t semaphore_buffer[PUBLISHER_TYPE];

    // create buffer queues and their semaphores
    for (int n=0; n<PUBLISHER_TYPE; n++){
        buffer[n]=create_queue(INITIAL_BUFFER_SIZE);
        sem_init(&semaphore_buffer[n], 0, INITIAL_BUFFER_SIZE);
    }

    char book_na[8];
    char* s = "Book";
    int rc;
    long t=-1;

    //creating the publisher threads
    for (int n=1; n<PUBLISHER_TYPE+1;n++){
        for (int m = 1; m < PUBLISHER_THREAD_COUNT+1; m++){
            t++;
            struct thread_type t_type;
            t_type.publisher_type=n;
            t_type.thread=&publisherThreads[t];
            // create books and call PutToBuffer
            for (int b=1; b<NUMBER_OF_BOOKS+1; b++){
                snprintf(book_na, 8, "%s,%d", s, b);
                struct book book_na;
                book_na.book_number=b;
                book_na.publisher_type=n;
                book_na.thread_id=t;
                book_na.queue=buffer[n-1];

                // if an error occurs while creation threads
                rc = pthread_create( &publisherThreads[t], NULL, PutToBuffer, &book_na);
                if (rc){
                    printf("ERROR; return code from pthread_create() is %d\n", rc);
                    exit(-1);
                }
            }
        }
    }

    //creating the packager threads
    int rcp;
    long pt;
    for(pt=0;pt<PACKAGER_THREAD_COUNT;pt++){
        rcp = pthread_create(&packageThreads[t], NULL, putToPackage, &package);
        if (rcp){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    //joining the package threads
    for(pt=0;pt<PACKAGER_THREAD_COUNT;pt++){
        rcp = pthread_join(packageThreads[t], NULL);
        if (rcp){
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    //joining the publisher threads
    for (int n=1; n<PUBLISHER_TYPE+1;n++){
        for (int m = 0; m < PUBLISHER_THREAD_COUNT; m++){
            t++;
            rc=pthread_join(publisherThreads[t], NULL);
            if (rc){
                printf("ERROR; return code from pthread_join() is %d\n", rc);
                exit(-1);
            }
        }
    }

    //destroying the mutex
    pthread_mutex_destroy(&lock);
    pthread_exit(NULL);
}
