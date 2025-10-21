#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 5
#define MAX 1000000

long contador = 0;
pthread_mutex_t mutex;

void* incrementar(void* arg) {
    int id = *((int*)arg);
    
    while (1) {
        pthread_mutex_lock(&mutex);
        
        if (contador >= MAX) {
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        
        contador++;
        
        if (contador == MAX) {
            printf("Thread %ld alcançou o valor final!\n", id);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        
        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS]; 
    
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i + 1; 
        pthread_create(&threads[i], NULL, incrementar, &ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&mutex);

}
