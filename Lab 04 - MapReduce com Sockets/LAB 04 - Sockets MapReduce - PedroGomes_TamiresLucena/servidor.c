// Pedro Spoljaric Gomes 112344
// Tamires Beatriz da Silva Lucena 111866


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BLOCK_MAX 1024
#define BLOCK_INICIAL 64

#define LEN_IN (64 * 1024)
#define MAX_CON 1000

typedef struct Titem {
    char *palavra;
    int valor;
    struct Titem *prox;
} Titem;

typedef struct {
    Titem *inicio, *fim;
} Tlista;

struct cliente_t {
	pthread_t tid;
	int sock;
	struct sockaddr_in caddr;
    char *texto;
};

void InicializarLista(Tlista *lista) {
    lista->inicio = NULL;
    lista->fim = NULL;
}

void ImprimirLista(Tlista *lista) {
    Titem *item = lista->inicio;

    while (item != NULL) {
        printf("<%s, %d> ", item->palavra, item->valor);
        item = item->prox;
    }
    printf("\n");
}

void InserirPalavra(Tlista *lista, char *palavra, int valor) {
    Titem *item = (Titem*) malloc(sizeof(Titem));

    item->palavra = (char*) malloc(sizeof(palavra));

    strcpy(item->palavra, palavra);
    item->valor = valor;
    item->prox = NULL;

    if (lista->inicio == NULL) {
        lista->inicio = item;
        lista->fim = item;
    }
    else {
        Titem *aux = lista->inicio;
        while (strcmp(palavra, aux->palavra) != 0 && aux != lista->fim) {
            aux = aux->prox;
        }
        if (aux == lista->fim) {
            lista->fim = item;
        }
        item->prox = aux->prox;
        aux->prox = item;
    }
}

void UneListas(Tlista *geral, Tlista *nova) {
   Titem *auxNova = nova->inicio;

   while (auxNova != NULL) {
       Titem *auxGeral = geral->inicio;

       int achei = 0;
       while (auxGeral != NULL) {
          if (strcmp(auxGeral->palavra, auxNova->palavra) == 0) {
              auxGeral->valor += auxNova->valor;
              achei = 1;
          }
          auxGeral = auxGeral->prox;
       }

       if (!achei) {
          Titem *item = (Titem*) malloc(sizeof(Titem));
          item->palavra = (char*) malloc(sizeof(auxNova->palavra));
	  strcpy(item->palavra, auxNova->palavra);
          item->valor = auxNova->valor;
          if (geral->fim == NULL) {
              geral->inicio = item;
          }
          else {
              geral->fim->prox = item;
          }
          geral->fim = item;
       }

       auxNova = auxNova->prox;
   }
}

void StringParaLista(Tlista *lista, char *texto) {
    int tam = strlen(texto);

    // printf("String para lista\nTexto recebido:%s\n", texto);

    int i = 0;

    while (i < tam) {
        char palavra[BLOCK_MAX];
        int j = 0;
        while (texto[i] != ' ') {
            palavra[j] = texto[i];
            j++;
            i++;
        }
        i++;
        palavra[j] = '\0';

        char numero[12];
        int k = 0;
        while (texto[i] != '\n') {
            numero[k] = texto[i];
            k++;
            i++;
        }
        i++;
        numero[k] = '\0';
        int n = atoi(numero);

        InserirPalavra(lista, palavra, n);
    }

	// printf("Lista:\n");
	// ImprimirLista(lista);
}

void *trata_cliente(void *args) {
	struct cliente_t c = *(struct cliente_t *)args;
	char msg_in[LEN_IN], msg_out[LEN_IN];
	int r, w;
	bzero(msg_in, LEN_IN);
	//recv
    
    r = send(c.sock, c.texto, strlen(c.texto), 0);

    char texto[BLOCK_MAX*2];
    bzero(texto, BLOCK_MAX*2);

    w = recv(c.sock, texto, BLOCK_MAX*2, 0);

    Tlista *lista = (Tlista*) malloc(sizeof(Tlista));
    InicializarLista(lista);
    StringParaLista(lista, texto);

	close(c.sock);
	// ImprimirLista(lista);
	pthread_exit((void*)lista);
}

int main(int argc, char **argv) {
    int sl;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    struct cliente_t c[MAX_CON+3];
    int sc;
    socklen_t clen;

    Tlista *geral = (Tlista*) malloc(sizeof(Tlista));
    InicializarLista(geral);

    // proc <porta>
    if (argc != 3) {
      printf("uso: %s <arquivo> <porta> \n", argv[0]);
      return 0;
    }

    int fd = open(argv[1], O_RDONLY);

    char *texto = (char*) malloc(BLOCK_MAX);
    int qtd;
    sl = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sl < 0) {
            perror("socket");
            return -1;
    }
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[2]));
    saddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sl, (struct sockaddr *)&saddr,
        sizeof(struct sockaddr_in)) < 0) {
        perror("bind");
        return -1;
    }
    if (listen(sl, MAX_CON) < 0) {
        perror("listen");
        return -1;
    }

    int qtdThreads = 5;
    qtd = 1;
    qtd = read(fd, texto, BLOCK_INICIAL-1);
    while (qtd > 0) {
	texto[qtd] = '\0';
        char ch[2];
        bzero(ch, 2);
        while (read(fd, ch, 1) > 0 && ch[0] != ' ' && ch[0] != '\n' && ch[0] != EOF) {
            strcat(texto, ch);
            qtd++;
        }

        clen = sizeof(struct sockaddr_in);
        bzero(&caddr, clen);
        sc = accept(sl, (struct sockaddr *)&caddr, &clen);
        if (sc < 0) {
            perror("accept");
            continue;
        }
        bzero(&c[sc], sizeof(struct cliente_t));
        c[sc].sock = sc;
        c[sc].caddr = caddr;
        c[sc].texto = (char*) malloc(BLOCK_MAX);
        strcpy(c[sc].texto, texto);

        printf("Texto enviado:\n%s\n", c[sc].texto);
        
        qtdThreads++;
        pthread_create(&c[sc].tid, NULL, trata_cliente, &c[sc]);

        Tlista *aux;
        void *vaux;

	pthread_join(c[sc].tid, &vaux);
	aux = (Tlista*) vaux;
	//ImprimirLista(aux);
	UneListas(geral, aux);
	//ImprimirLista(geral);

        bzero(texto, BLOCK_MAX);
        qtd = read(fd, texto, BLOCK_INICIAL);
    }

    close(sl);
    close(fd);

    printf("Lista geral final:\n");
    ImprimirLista(geral);

    return 0;
}
