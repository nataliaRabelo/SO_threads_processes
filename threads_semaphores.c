//Natalia: para compilar tem que usar "gcc at2.c -lpthread -o trabalho"

// O codigo da um problema quando tem thread e não tem semáforo:
//na hora da remoção de elementos do vetor as threads se atropelam. 
//Acaba que as threads ficam modificando a informação do mesmo vetor e números que não eram para aparecer ficam duplicados.
//Além disso, de vez enquando ele também erra o tamanho do vetor. 
//Com semáforo não vi problemas.

#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>

#define USAR_SEMAFORO 1 //Comente este codigo para nao usar semaforo

#define TAM_VETOR 100000 //Tamanho do vetor que esta recebendo os numeros
#define MAX_NUM 100 //Maximo do intervado de numeros aleatorios que estou gerando: comecao de 1 e vai até MAX_NUM

//Estrutura de vetor estatica: tem o conteudo do vetor associado ao seu tamanho
typedef struct MeuArray
{
    int conteudo[TAM_VETOR + 1]; 
    int tamanho;
} Array;

typedef long time_t; //Um codigo que peguei rapidamente da internet para contar tempo 
#ifdef USAR_SEMAFORO 
sem_t sem; //Semáforo para o remove vetor
#endif

//Retorna 1 se os dois vetores sao iguais ou zero caso contrário
int verifica_resultado(Array* vetor_1, Array* vetor_2)
{

    if(vetor_1->tamanho != vetor_2->tamanho)
    {
        printf("O tamanho dos dois vetores estão diferentes: %d / %d\n", vetor_1->tamanho, vetor_2->tamanho);
        return 0;
    }

    for(int i = 0; i < vetor_1->tamanho; i++)
    {
        if (vetor_1->conteudo[i] != vetor_2->conteudo[i])
        {
            printf("O elemento na posicao %d é diferente!!!\n", i);
            return 0;
        }
    }

    return 1;
}

//Função que imprime o struct array
void imprime_array(Array vetor)
{
    printf("Tamanho: %d\nConteudo:\n", vetor.tamanho);
    for(int i = 0; i < vetor.tamanho; i++)
    {
        printf("%d\n", vetor.conteudo[i]);
    }
}

//Removendo o elemento de um vetor estatico de inteiros
void remove_vetor(Array* vetor, int posicao)
{
    //Guarda que só deixa 1 thread passar
    for(int i = posicao; i < vetor->tamanho; ++i)
    {
        vetor->conteudo[i] = vetor->conteudo[i+1]; //É só pegar todo mundo que está na frente do elemento removido e jogar para trás.
    }
    vetor->tamanho--; //(*vetor).tamanho
}

//Remove os números pares do parametro de entrada que precisa ser um struct Array
void *removePares(void* param)
{
    Array* vetor = (Array *) param; //converte para vetor de inteiro
    #ifdef USAR_SEMAFORO 
    sem_wait(&sem); //semaforo-- [AKI]
    #endif
    for (int i = vetor->tamanho - 1; i >= 0; i--)
    {
        if (vetor->conteudo[i] % 2 == 0)
        {
            remove_vetor(vetor, i);
        }
    }
    #ifdef USAR_SEMAFORO 
    sem_post(&sem); //semaforo++ [AKI]
    #endif
}

//Remove os números pares do parametro de entrada que precisa ser precisa ser um struct Array
void *removeMultiplosCinco(void* param)
{
    Array* vetor = (Array *) param; //converte para vetor de inteiro
    #ifdef USAR_SEMAFORO 
    sem_wait(&sem); //semaforo-- [AKI]
    #endif
    for (int i = vetor->tamanho - 1; i >= 0; i--)
    {
        if (vetor->conteudo[i] % 5 == 0)
        {
            remove_vetor(vetor, i);
        }
    }
    #ifdef USAR_SEMAFORO 
    sem_post(&sem); //semaforo++ [AKI]
    #endif
}

//Faz o processamento sequencial
void simula_sem_thread(Array* vetor)
{
    removePares(vetor);
    removeMultiplosCinco(vetor);
}

//Faz o processamento paralelo
void simula_com_thread(Array* vetor)
{
    pthread_t thread_1, thread_2;
    pthread_create(&thread_1, NULL, *removePares, (void *) vetor);
    pthread_create(&thread_2, NULL, *removeMultiplosCinco, (void *) vetor);
    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);
}
 
int main()
{
    //INICIALIZA!!!!
    Array vetor_sequencial, vetor_paralelo; //Um vetor para processamento sequencial outro para paralelo
    srand((unsigned) time(NULL)); //semente aleatoria baseada no sistema
    //srand(1); //semente fixa para testes
    clock_t comeco, fim; //copie da net para contar tempo
    #ifdef USAR_SEMAFORO 
    sem_init(&sem, 0, 1); //Inicializa o semaforo com 1. Somente uma thread vai passar por vez
    #endif

    //gerando os numeros aleatorios para o vetor
    vetor_sequencial.tamanho = TAM_VETOR;
    vetor_paralelo.tamanho = TAM_VETOR;
    for (int i = 0; i < TAM_VETOR; i++)
    { 
        int b = rand() % MAX_NUM + 1; //gera um numero de 1 ate 100 aleatorio 
        //printf("%d\n", b); //print de debug para testar o meu programa
        vetor_sequencial.conteudo[i] = b; //insere os números sorteados tanto no primeiro vetor quanto no segundo 
        vetor_paralelo.conteudo[i] = b; //insere os números sorteados tanto no primeiro vetor quanto no segundo 
    }

    //!!!FAZENDO SEM THREAD!!!
    comeco = clock(); //Comeca a contagem de tempo
    simula_sem_thread(&vetor_sequencial);
    fim = clock(); //Final da contagem de tempo
    printf("Tempo total de execucao sequencial: %f (s)\n", ((double) fim - comeco)/CLOCKS_PER_SEC);
    printf("--Array sequencial--\n");
    imprime_array(vetor_sequencial); //outro print para testar o meu programa
    printf("---------\n");
     
    //!!!FAZENDO COM THREAD!!!
    comeco = clock(); //Comeca a contagem de tempo
    simula_com_thread(&vetor_paralelo);
    fim = clock(); //Final da contagem de tempo
    printf("Tempo total de execucao paralelo: %f (s)\n", ((double) fim - comeco)/CLOCKS_PER_SEC);
    printf("--Array paralelo--\n");
    imprime_array(vetor_paralelo); //outro print para testar o meu programa
    printf("---------\n");

    //Comparando!!!!
    int resultado = verifica_resultado(&vetor_sequencial, &vetor_paralelo);
    if (resultado == 0) printf("O resultado está incorreto\n");
    else printf("O resultado está correto\n");
    #ifdef USAR_SEMAFORO 
    sem_destroy(&sem); //Destruindo o semaforo que aloquei
    #endif

    return 0;
}