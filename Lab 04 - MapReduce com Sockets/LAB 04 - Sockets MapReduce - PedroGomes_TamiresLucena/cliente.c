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

typedef struct Titem {
    char *palavra;
    int valor;
    struct Titem *prox;
} Titem;

typedef struct {
    Titem *inicio, *fim;
} Tlista;

void InicializarLista(Tlista *lista) {
    lista->inicio = NULL;
    lista->fim = NULL;
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

void ImprimirLista(Tlista *lista) {
    Titem *item = lista->inicio;

    while (item != NULL) {
        printf("<%s, %d> ", item->palavra, item->valor);
        item = item->prox;
    }
    printf("\n");
}

Tlista *Mapper(void *args) {
    Tlista *lista = (Tlista*) malloc(sizeof(Tlista));
    InicializarLista(lista);

    char *texto = *((char **) args);

    int i = 0;
    while (i < strlen(texto))
    {
        char *strAux = (char*) malloc(sizeof(char));

        int chrAtual = 0;

        while (texto[i] != ' ' && texto[i] != '\r' && texto[i] != '\n' && i < strlen(texto))
        {
            strAux[chrAtual] = texto[i];
            chrAtual++;

            strAux = (char*) realloc(strAux, (chrAtual+1)*sizeof(char));

            i++;
        }
        i++;
        strAux[chrAtual] = '\0';

        InserirPalavra(lista, strAux, 1);
    }

   return lista;
}

void Reducer(void *args) {
   Tlista *lista = (Tlista*) args;

   Titem *aux = lista->inicio;
   Titem *atual = lista->inicio;

   while (atual != NULL && atual != lista->fim) {
     while (atual->prox != NULL && strcmp(atual->palavra, atual->prox->palavra) == 0) {
        atual->valor += atual->prox->valor;
        aux = atual->prox;
	atual->prox = aux->prox;
	free(aux);
     }
     atual = atual->prox;
   }
}

char *ListaParaString(Tlista *lista) {
    char *texto = (char*) malloc(BLOCK_MAX*2);
    bzero(texto, BLOCK_MAX*2);

    Titem *aux = lista->inicio;

    while (aux != NULL) {
        char *strItem = (char*) malloc(BLOCK_MAX);
        bzero(strItem, BLOCK_MAX);
        strcat(strItem, aux->palavra);
        strcat(strItem, " ");
        char numero[12];
        sprintf(numero, "%d", aux->valor);
        strcat(strItem, numero);
        strcat(strItem, "\n");

        strcat(texto, strItem);

        aux = aux->prox;
    }
    return texto;
}

int main(int argc, char **argv) {
    struct sockaddr_in saddr;
    socklen_t slen;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[1]));
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    slen = sizeof(struct sockaddr_in);

    int s;
    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    connect(s, (struct sockaddr *)&saddr, slen);

    char *texto = (char*) malloc(BLOCK_MAX);
    bzero(texto, BLOCK_MAX);

    
    int w;
    w = recv(s, texto, BLOCK_MAX, 0);

    printf("Texto recebido:\n%s\n", texto);

    Tlista *lista = Mapper(&texto);

    Reducer(lista);

    printf("Lista depois da reducao:\n");
    ImprimirLista(lista);

    char *txt = ListaParaString(lista);
    
    int r;

    r = send(s, txt, strlen(txt), 0);

    close(s);
}
