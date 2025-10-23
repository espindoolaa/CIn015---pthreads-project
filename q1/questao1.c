/* --------------------- ideia central ----------------------------
    Considerando que temos duas threads tentando incrementar em contador++,
que não é atômico, lidamos com essa condição de disputa com a utilização do mutex
para proteger a região crítica, uma vez que quando uma thread está dentro dela,
nenhuma outra pode acessar o contador até que o mutex seja liberado. Assim, o 
incremento mediante 4 threads funcionou normalmente.
*/

#include <stdio.h>
#include <pthread.h>

#define NUM_THREADS 4
#define MAX 1000000

long contador = 0;
pthread_mutex_t mutex;


// função executada por cada thread, em que cada uma delas incrementa o contador até atingir o máximo.
void* incrementar(void* arg) {
    int id = *((int*)arg);
    
    while (1) {
        // entra na seção crítica
        pthread_mutex_lock(&mutex);

        // se o contador atingiu o limite, libera o mutex e termina a thread
        if (contador >= MAX) {
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL); // encerra a thread atual
        }

        // incrementa o contador dentro da região crítica
        contador++;

        // se a thread atual alcançou o valor máximo, imprime e termina
        if (contador == MAX) {
            printf("Thread %ld alcançou o valor final!\n", id);
            pthread_mutex_unlock(&mutex); // libera o mutex antes de sair para nao bloquear outras threads
            pthread_exit(NULL);
        }

        // libera o mutex
        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    
    // inicialização do mutex
    pthread_mutex_init(&mutex, NULL);

    // cria as quatro threads definidas, passando o endereço ids[i] como argumeto
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i + 1; 
        pthread_create(&threads[i], NULL, incrementar, &ids[i]);
    }
    
    // espera cada thread terminar
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // destruição do mutex para liberar os recursos do sistema
    pthread_mutex_destroy(&mutex);

}
