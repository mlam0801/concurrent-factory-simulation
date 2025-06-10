#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define BUFFER1SIZE 6
#define BUFFER2SIZE 4
#define NUMITERATIONS 2

typedef struct{
    int counter;
    int type;
} product;

product getFromBuffer(product buffer[], int *head, int bufferSize);
void putInBuffer(product productNum, product buffer[], int *tail, int *count, int bufferSize);
void* consumer1Thread(void* arg);
void* consumer2Thread(void* arg);
void producer1();
void producer2();

int pipeFD[2];

int producer1Counter = 0;
int producer2Counter = 0;
int consumer1Count = 0;
int consumer2Count = 0;
int producerCompleteCounter = 0;

pthread_mutex_t consumer1Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t consumer2Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fileMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer1Cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer2Cond = PTHREAD_COND_INITIALIZER;

product buffer1[BUFFER1SIZE];
int buffer1Head = 0, buffer1Tail = 0, buffer1Counter = 0;
product buffer2[BUFFER2SIZE];
int buffer2Head = 0, buffer2Tail = 0, buffer2Counter = 0;

int main() {
    pthread_t consumer1Thread1, consumer1Thread2, consumer2Thread1, consumer2Thread2;

    if(pipe(pipeFD) == -1){
        perror("pipe error");
        exit(1);
    }

    int producer1PID = fork();
    if(producer1PID == -1){
        perror("could not fork producer 1");
    } else if(producer1PID == 0){
        close(pipeFD[0]);
        producer1();
        exit(0);
    }

    int producer2PID = fork();
    if(producer2PID == -1){
        perror("could not fork producer 2");
    } else if(producer2PID == 0){
        close(pipeFD[0]);
        producer2();
        exit(0);
    }

    // Only create threads after forking
    pthread_create(&consumer1Thread1, NULL, consumer1Thread, NULL);
    pthread_create(&consumer1Thread2, NULL, consumer1Thread, NULL);
    pthread_create(&consumer2Thread1, NULL, consumer2Thread, NULL);
    pthread_create(&consumer2Thread2, NULL, consumer2Thread, NULL);

    close(pipeFD[1]);
    product productNum;
    while(1){
        int pipeReadVal = read(pipeFD[0], &productNum, sizeof(product));
        if(pipeReadVal == -1){
            perror("read error");
            exit(1);
        }
        if(productNum.counter == -1){
            producerCompleteCounter++;
            if(producerCompleteCounter == 2) break;
            continue;
        }

        if(productNum.type == 1){
            pthread_mutex_lock(&consumer1Mutex);
            while(buffer1Counter == BUFFER1SIZE){
                pthread_cond_wait(&buffer1Cond, &consumer1Mutex);
            }
            putInBuffer(productNum, buffer1, &buffer1Tail, &buffer1Counter, BUFFER1SIZE);
            pthread_mutex_unlock(&consumer1Mutex);
            pthread_cond_signal(&buffer1Cond);
        } else if(productNum.type == 2){
            pthread_mutex_lock(&consumer2Mutex);
            while(buffer2Counter == BUFFER2SIZE){
                pthread_cond_wait(&buffer2Cond, &consumer2Mutex);
            }
            putInBuffer(productNum, buffer2, &buffer2Tail, &buffer2Counter, BUFFER2SIZE);
            pthread_mutex_unlock(&consumer2Mutex);
            pthread_cond_signal(&buffer2Cond);
        }
    }

    pthread_cancel(consumer1Thread1);
    pthread_cancel(consumer1Thread2);
    pthread_cancel(consumer2Thread1);
    pthread_cancel(consumer2Thread2);

    waitpid(producer1PID, NULL, 0);
    waitpid(producer2PID, NULL, 0);
    return 0;
}

product getFromBuffer(product buffer[], int *head, int bufferSize) {
    product item = buffer[*head];
    *head = (*head + 1) % bufferSize;
    return item;
}

void putInBuffer(product productNum, product buffer[], int *tail, int *count, int bufferSize) {
    buffer[*tail] = productNum;
    *tail = (*tail + 1) % bufferSize;
    (*count)++;
}

void* consumer1Thread(void* arg){
    product item;
    while(1){
        pthread_mutex_lock(&consumer1Mutex);
        while(buffer1Counter == 0){
            pthread_cond_wait(&buffer1Cond, &consumer1Mutex);
        }
        item = getFromBuffer(buffer1, &buffer1Head, BUFFER1SIZE);
        buffer1Counter--;
        pthread_mutex_unlock(&consumer1Mutex);
        pthread_cond_signal(&buffer1Cond);

        pthread_mutex_lock(&fileMutex);
        consumer1Count++;
        FILE* fileData = fopen("factoryRecords.txt", "a");
        if (fileData != NULL) {
            fprintf(fileData, "Product ID: Product 1 | Thread ID: %ld | Prod SEQ #: %d | Consume SEQ #: %d | Total Consumed: %d\n",
                    pthread_self(), item.counter, consumer1Count, consumer1Count + consumer2Count);
            fclose(fileData);
        }
        pthread_mutex_unlock(&fileMutex);
    }
    return NULL;
}

void* consumer2Thread(void* arg){
    product item;
    while(1){
        pthread_mutex_lock(&consumer2Mutex);
        while(buffer2Counter == 0){
            pthread_cond_wait(&buffer2Cond, &consumer2Mutex);
        }
        item = getFromBuffer(buffer2, &buffer2Head, BUFFER2SIZE);
        buffer2Counter--;
        pthread_mutex_unlock(&consumer2Mutex);
        pthread_cond_signal(&buffer2Cond);

        pthread_mutex_lock(&fileMutex);
        consumer2Count++;
        FILE* fileData = fopen("factoryRecords.txt", "a");
        if (fileData != NULL) {
            fprintf(fileData, "Product ID: Product 2 | Thread ID: %ld | Prod SEQ #: %d | Consume SEQ #: %d | Total Consumed: %d\n",
                    pthread_self(), item.counter, consumer2Count, consumer1Count + consumer2Count);
            fclose(fileData);
        }
        pthread_mutex_unlock(&fileMutex);
    }
    return NULL;
}

void producer1() {
    product item;
    item.type = 1;
    srand(time(NULL) ^ getpid());
    for (int i = 0; i < NUMITERATIONS; i++) {
        usleep(rand() % 200000 + 10000);
        item.counter = producer1Counter++;
        write(pipeFD[1], &item, sizeof(product));
    }
    item.counter = -1;
    write(pipeFD[1], &item, sizeof(product));
}

void producer2() {
    product item;
    item.type = 2;
    srand(time(NULL) ^ getpid());
    for (int i = 0; i < NUMITERATIONS; i++) {
        usleep(rand() % 200000 + 10000);
        item.counter = producer2Counter++;
        write(pipeFD[1], &item, sizeof(product));
    }
    item.counter = -1;
    write(pipeFD[1], &item, sizeof(product));
}
