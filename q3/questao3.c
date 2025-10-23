/* --------------------------- ideia geral da resolução ---------------------------
  Para a resolução desse problema de gerenciamento de impressão concorrente, foram criadas threads de
usuários que enviam documentos simultaneamente para uma fila de impressao compartilhada, com uma única thread
impressora, que consome os documentos e imprime sempre o de maior prioridade primeiro. Dado o fato da concorrência,
basicamente implementamos um mutex para proteger a fila, impedindo que duas threads modifiquem a lista ao mesmo tempo,
além de uma variável de condição, para que a impressora espere quando não houver documentos, e seja notificada quando um n
novo documento for adicionado. Por fim, foram criadas duas filas independentes, uma para alta prioridade e outra para baixa, 
visando eliminar a necessidade de percorrer uma lista única buscando o maior valor. 
    Assim, nosso programa consegue lidar com condições de disputa, prioridade dos documentos mediante inserção de múltiplos usuários
simultaneamente.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_NAME 64 // tamanho máximo do nome de um documento
#define N_USUARIOS 3 // número de threads que simulam usuários
#define DOCS_POR_USUARIO 3 // quantos documentos cada usuário enviará 

typedef struct Documento {
    char nome[MAX_NAME];
    int usuario_id;
    int prioridade; // 0 = baixa, 1 = alta
    struct Documento* prox;
} Documento;

// filas separadas por prioridade
Documento* fila_alta = NULL;
Documento* fila_baixa = NULL;

// objetos para a sincronização
pthread_mutex_t mutex_fila = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonempty = PTHREAD_COND_INITIALIZER;

// controle do número da threads dos usuários
int usuarios_ativos = N_USUARIOS;

// --------------------------- funções auxiliares ---------------------------

// 1. maneja a fila, fazendo a inserção dos documentos no final da fila
void inserir_fila(Documento** fila, Documento* doc) {
    doc->prox = NULL;
    if (*fila == NULL) {
        *fila = doc;
    } else {
        Documento* p = *fila;
        while (p->prox) p = p->prox;
        p->prox = doc;
    }
}

// 2. remove o primeiro documento da fila 
Documento* remover_fila(Documento** fila) {
    if (*fila == NULL) return NULL;
    Documento* doc = *fila;
    *fila = doc->prox;
    doc->prox = NULL;
    return doc;
}

// --------------------------- thread usuário ---------------------------

void* usuario_thread(void* arg) {
    int id = (int)(long)arg;

    for (int i = 0; i < DOCS_POR_USUARIO; i++) {
        Documento* d = malloc(sizeof(Documento));
        if (!d) {
          exit(1);
        }
      
        // monta o nome do doc. e define prioridade alternada
        snprintf(d->nome, MAX_NAME, "Doc_U%d_%d", id, i + 1);
        d->usuario_id = id;
        d->prioridade = (i % 2 == 0) ? 1 : 0; // alterna: alta ou baixa.

        // entra na região crítica para inserir na fila
        pthread_mutex_lock(&mutex_fila);

        // insere na fila correspondente à prioridade
        if (d->prioridade == 1)
            inserir_fila(&fila_alta, d);
        else
            inserir_fila(&fila_baixa, d);

        // mostra a ação no terminal.
        printf("[Usuário %d] Enviou %s (prioridade %d)\n",
               d->usuario_id, d->nome, d->prioridade);
        // acorda a impressora caso esteja esperando por documentos
        pthread_cond_signal(&cond_nonempty);
      
        // sai da região crítica
        pthread_mutex_unlock(&mutex_fila);
    }
  
    pthread_mutex_lock(&mutex_fila);
    usuarios_ativos--; // decremento no número de usuários ativos
    pthread_cond_signal(&cond_nonempty); //acorda impressora se estiver esperando
    pthread_mutex_unlock(&mutex_fila);

    return NULL;
}

// --------------------------- thread impressora ---------------------------

void* impressora_thread(void* arg) {
    (void)arg;

    while (1) {
        pthread_mutex_lock(&mutex_fila);

        // espera enquanto não há documentos e usuários ainda ativos
        while (fila_alta == NULL && fila_baixa == NULL && usuarios_ativos > 0) {
            pthread_cond_wait(&cond_nonempty, &mutex_fila);
        }

        // se todos os usuários acabaram e a fila está vazia, finaliza o programa.
        if (fila_alta == NULL && fila_baixa == NULL && usuarios_ativos == 0) {
            pthread_mutex_unlock(&mutex_fila);
            break;
        }

        // remove documento da fila 
        Documento* doc = NULL;

        if (fila_alta != NULL)
            doc = remover_fila(&fila_alta);
        else if (fila_baixa != NULL)
            doc = remover_fila(&fila_baixa);

        pthread_mutex_unlock(&mutex_fila);

        // processamento do documento
        if (doc != NULL) {
            printf(">>> Impressora processou %s (usuário %d, prioridade %d)\n",
                   doc->nome, doc->usuario_id, doc->prioridade);
            free(doc); // libera a memória do documento
        }
    }

    printf(">>> Impressora finalizou.\n");
    return NULL;
}

// --------------------------- função main ---------------------------

int main() {
    pthread_t usuarios[N_USUARIOS]; // threads de usuários
    pthread_t impressora; // thread da impressora

    // cria a thread da impressora
    pthread_create(&impressora, NULL, impressora_thread, NULL);

    // cria as threads dos usuários
    for (long i = 0; i < N_USUARIOS; i++)
        pthread_create(&usuarios[i], NULL, usuario_thread, (void*)i);

    // espera que as threads de usuários terminem
    for (int i = 0; i < N_USUARIOS; i++)
        pthread_join(usuarios[i], NULL);

    // aguarda o encerramento da impressora
    pthread_join(impressora, NULL);

    // liberação dos recursos utilizads: mutex e variável condicional.
    pthread_mutex_destroy(&mutex_fila);
    pthread_cond_destroy(&cond_nonempty);
  
    return 0;
}
