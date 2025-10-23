/* --------------------------- ideia geral da resolução ---------------------------
   - Existem P threads produtoras e C threads consumidoras.
   - Cada buffer tem tamanho fixo (BUFFER_SIZE) e é protegido por mutex próprio.
   - As variáveis de condição "empty"' e "fill" controlam quando o buffer está
     cheio ou vazio, evitando acessos concorrentes incorretos.
   - Produtores escolhem aleatoriamente um buffer e tentam inserir um item.
     Se o buffer estiver cheio, tentam outro após uma breve espera.
   - Consumidores fazem o inverso: escolhem aleatoriamente um buffer e tentam retirar um item.
     Se estiver vazio, aguardam ou tentam outro buffer.
*/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define P 2            // Qtd produtores
#define C 3            // qtd consumidores
#define B 3            // Qtd buffers
#define BUFFER_SIZE 5  // Tam dos buffers
#define NUM_ITEMS 100  // qtd máxima de itens

typedef struct {
    int buff[BUFFER_SIZE];
    int items;
    int first;
    int last;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t fill;
} Buffer;

Buffer buffers[B];

// Inicialização dos buffers
void init_buffers() {
    for (int i = 0; i < B; i++) {
        buffers[i].items = 0;
        buffers[i].first = 0;
        buffers[i].last = 0;
        pthread_mutex_init(&buffers[i].mutex, NULL);
        pthread_cond_init(&buffers[i].empty, NULL);
        pthread_cond_init(&buffers[i].fill, NULL);
    }
}

// Coloca um item em algum buffer disponível
void put_item(int id, int item) {
    int b;
    while (1) {
        b = rand() % B;
        Buffer *buf = &buffers[b];

        pthread_mutex_lock(&buf->mutex);
        if (buf->items < BUFFER_SIZE) { // encontrou espaço
            buf->buff[buf->last] = item;
            buf->last = (buf->last + 1) % BUFFER_SIZE;
            buf->items++;

            printf("[Produtor %d] colocou %d no buffer %d (itens: %d)\n", id, item, b, buf->items);

            pthread_cond_signal(&buf->fill);
            pthread_mutex_unlock(&buf->mutex);
            break;
        }
        pthread_mutex_unlock(&buf->mutex);
        usleep(100); // espera um pouco antes de tentar outro buffer
    }
}

// Pega um item de algum buffer que não tá vazio
int get_item(int id) {
    int b;
    int item;
    while (1) {
        b = rand() % B;
        Buffer *buf = &buffers[b];

        pthread_mutex_lock(&buf->mutex);
        if (buf->items > 0) { // encontrou item
            item = buf->buff[buf->first];
            buf->first = (buf->first + 1) % BUFFER_SIZE;
            buf->items--;

            printf("[Consumidor %d] pegou %d do buffer %d (itens: %d)\n", id, item, b, buf->items);

            pthread_cond_signal(&buf->empty);
            pthread_mutex_unlock(&buf->mutex);
            break;
        }
        pthread_mutex_unlock(&buf->mutex);
        usleep(100); // espera antes de tentar outro buffer
    }
    return item;
}

// Thread do produtor
void *producer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < NUM_ITEMS; i++)
        put_item(id, i);
    pthread_exit(NULL);
}

// Thread do consumidor
void *consumer(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < NUM_ITEMS; i++)
        get_item(id);
    pthread_exit(NULL);
}

int main() {
    srand(time(NULL));
    init_buffers();

    pthread_t prod[P], cons[C];
    int idp[P], idc[C];

    // cria threads produtores
    for (int i = 0; i < P; i++) {
        idp[i] = i;
        pthread_create(&prod[i], NULL, producer, &idp[i]);
    }

    // Cria threads consumidores
    for (int i = 0; i < C; i++) {
        idc[i] = i;
        pthread_create(&cons[i], NULL, consumer, &idc[i]);
    }

    // Espera todos terminarem
    for (int i = 0; i < P; i++)
        pthread_join(prod[i], NULL);
    for (int i = 0; i < C; i++)
        pthread_join(cons[i], NULL);

    return 0;
}
