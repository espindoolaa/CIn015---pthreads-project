/* --------------------------- ideia geral da resolução ---------------------------
   Simulação do problema leitores-escritores com prioridade para escritores.
   - Temos N_LEITORES leitores e M_ESCRITOES escritores acessando um array compartilhado.
   - Mutex protege variáveis de controle e regiões críticas.
   - Condições permitem que threads esperem quando não podem acessar.
   - Escritores têm prioridade: novos leitores aguardam se houver escritores esperando.
   - Threads rodam infinitamente, simulando concorrência real.
   - Foi adicionada uma intercalação obrigatória, pois nas primeiras implementações um escritor monopolizava a escrita.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>

#define N_LEITORES 5
#define M_ESCRITOES 2
#define TAM 10

int array[TAM];              
int leitores = 0;            
int escritores = 0;          
int esperando_escritor = 0;  

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_leitor = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_escritor = PTHREAD_COND_INITIALIZER;

// --------------------------- função leitor ---------------------------
void *leitor(void *id) {
    int tid = *((int *)id);

    while (1) {
        // --- Seção de controle: entrada para leitura ---
        pthread_mutex_lock(&mutex);
        while (escritores > 0 || esperando_escritor > 0) {
            pthread_cond_wait(&cond_leitor, &mutex); // aguarda se houver escritor ativo ou esperando
        }
        leitores++; // registra leitor ativo
        pthread_mutex_unlock(&mutex);

        // --- Região crítica de leitura ---
        int pos = rand() % TAM;
        printf("Leitor %d lendo posição %d: %d\n", tid, pos, array[pos]);
        usleep(100000); // simula tempo de leitura

        // --- Saída da seção de leitura ---
        pthread_mutex_lock(&mutex);
        leitores--;
        if (leitores == 0) // último leitor libera escritor
            pthread_cond_signal(&cond_escritor);
        pthread_mutex_unlock(&mutex);

        // força yield para intercalar threads
        usleep(50000 + rand() % 50000);
    }

    return NULL;
}

// --------------------------- função escritor ---------------------------
void *escritor(void *id) {
    int tid = *((int *)id);

    while (1) {
        // --- Seção de controle: entrada para escrita ---
        pthread_mutex_lock(&mutex);
        esperando_escritor++; // registra escritor aguardando
        while (leitores > 0 || escritores > 0) {
            pthread_cond_wait(&cond_escritor, &mutex); // espera se há leitores ou escritores ativos
        }
        esperando_escritor--;
        escritores++; // escritor entra na região crítica
        pthread_mutex_unlock(&mutex);

        // --- Região crítica de escrita ---
        int pos = rand() % TAM;
        int valor = rand() % 1000;
        array[pos] = valor;
        printf(">>> Escritor %d escreveu %d na posição %d\n", tid, valor, pos);
        usleep(150000); // simula tempo de escrita

        // --- Saída da escrita ---
        pthread_mutex_lock(&mutex);
        escritores--;
        if (esperando_escritor > 0)
            pthread_cond_signal(&cond_escritor); // prioriza próximo escritor
        else
            pthread_cond_broadcast(&cond_leitor); // acorda leitores
        pthread_mutex_unlock(&mutex);

        // força a intercalação threads
        usleep(50000 + rand() % 50000);
    }

    return NULL;
}

// --------------------------- função main ---------------------------
int main() {
    pthread_t t_leitor[N_LEITORES];
    pthread_t t_escritor[M_ESCRITOES];
    int ids[N_LEITORES > M_ESCRITOES ? N_LEITORES : M_ESCRITOES];

    srand(time(NULL));

    // inicializa array compartilhado
    for (int i = 0; i < TAM; i++)
        array[i] = 0;

    // cria threads de leitores
    for (int i = 0; i < N_LEITORES; i++) {
        ids[i] = i;
        pthread_create(&t_leitor[i], NULL, leitor, &ids[i]);
    }

    // cria threads de escritores
    for (int i = 0; i < M_ESCRITOES; i++) {
        ids[i] = i;
        pthread_create(&t_escritor[i], NULL, escritor, &ids[i]);
    }

    // mantém main ativa enquanto threads executam
    pthread_exit(NULL);
}
