//Natalia: para compilar tem que usar "gcc at3.c -lrt -lpthread -o trabalho"


#include <unistd.h> /* defines fork(), and pid_t. */
#include <sys/shm.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include <fcntl.h> 

#define TAM_VETOR 100000 //Tamanho do vetor que esta recebendo os numeros
#define MAX_NUM 100 //Maximo do intervado de numeros aleatorios que estou gerando: comecao de 1 e vai até MAX_NUM
#define CHAVE 38 //Chave da area de memoria compartilhada do vetor

//Estrutura de vetor estatica: tem o conteudo do vetor associado ao seu tamanho
typedef struct MeuArray
{
    int conteudo[TAM_VETOR + 1]; 
    int tamanho;
} Array;

typedef long time_t; //Um codigo que peguei rapidamente da internet para contar tempo 
pid_t pid_filho = -1; //Id do processo filho
int memoria_compartilhada_id; //id da memoria compartilhada
int* memoria_compartilhada; //Conteudo da memoria compartilhada
sem_t* semaphore; //Semáforo para o remove vetor
sem_t* semaphore_sync; //Semáforo para sincronizar prender o pai
sem_t* semaphore_sync2; //Semáforo para sincronizar para prender o filho

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
void remove_vetor(int* vetor, int posicao, int* tamanho)
{
    //Guarda que só deixa 1 thread passar
    for(int i = posicao; i < *tamanho; ++i)
    {
        vetor[i] = vetor[i+1]; //É só pegar todo mundo que está na frente do elemento removido e jogar para trás.
    }
    *tamanho -= 1; //(*vetor).tamanho
}

//Faz o processamento sequencial
void simula_sem_filho(Array* vetor)
{
    //Remove pares
    for (int i = vetor->tamanho - 1; i >= 0; i--)
    {
        if (vetor->conteudo[i] % 2 == 0)
        {
            remove_vetor((int *) &vetor->conteudo, i, (int *) &vetor->tamanho);
        }
    }

    //Remove multiplos de cinco
    for (int i = vetor->tamanho - 1; i >= 0; i--)
    {
        if (vetor->conteudo[i] % 5 == 0)
        {
            remove_vetor((int *) &vetor->conteudo, i, (int *) &vetor->tamanho);
        }
    }
}

//Codigo do processo filho para processar a memoria compartilhada
void filho()
{
        //Imprimindo os semaforos
        int valor_semaphore, valor_semaphore_sync, valor_semaphore_sync2;
        sem_getvalue(semaphore, &valor_semaphore);
        sem_getvalue(semaphore_sync, &valor_semaphore_sync);
        sem_getvalue(semaphore_sync2, &valor_semaphore_sync2);
        printf("Valor dos semaforos: %d %d %d (%d).\n", valor_semaphore, valor_semaphore_sync, valor_semaphore_sync2, getpid());
        fflush(stdout);

        //Espera o pai preencher a memoria
        printf("Eu sou o filho %d: Estou esperando o pai liberar a memoria .\n", getpid());
        fflush(stdout);
        sem_wait(semaphore_sync2);
        printf("Eu sou o filho %d: Fui liberado pelo pai.\n", getpid());
        fflush(stdout);

        // Faz o remove multiplo de cinco
        fflush(stdout);
        sem_wait(semaphore); //só um pode processar por vez, senão da errado
        printf("Eu sou o filho: comecei a processar (%d).\n", getpid());
        fflush(stdout);
        // printf("Vou imprimir a memoria compartilhada de tamanho %d: (%d).\n",  memoria_compartilhada[0], getpid());
        // fflush(stdout);
        // for(int i = 1; i <= memoria_compartilhada[0]; i++){
        //     printf("%d (%d).\n", memoria_compartilhada[i], getpid());
        //     fflush(stdout);
        // }
        for (int i = memoria_compartilhada[0]; i >= 1; i--)
        {
            if (memoria_compartilhada[i] % 5 == 0)
            {
                remove_vetor((int *) &memoria_compartilhada[1], i-1, (int *) &memoria_compartilhada[0]);
            }
        }
        // printf("Vou imprimir após o processamento a memoria compartilhada de tamanho %d: (%d).\n",  memoria_compartilhada[0], getpid());
        // for(int i = 1; i <= memoria_compartilhada[0]; i++){
        //     printf("%d (%d).\n", memoria_compartilhada[i], getpid());
        //     fflush(stdout);
        // }
        sem_post(semaphore); //só um pode processar por vez, senão da errado

        printf("Eu sou o filho: vou liberar o pai (%d).\n", getpid()); //print para debugar o código
        fflush(stdout);
        sem_post(semaphore_sync); //Libera o pai

        //Desligando dos semaforos
        sem_close(semaphore);  
        sem_close(semaphore_sync);  
        sem_close(semaphore_sync2);  

        printf("Eu sou o filho: terminei (%d).\n", getpid());
        exit(0); //Filho já processou com sucesso
}

//Codigo do processo pai para processar a memoria compartilhada
void pai(Array* vetor){
    printf("Eu sou o processo pai: %d\n", getpid()); //Print para debugar o meu programa        
    fflush(stdout);
    
    //Imprimindo os semaforos
    int valor_semaphore, valor_semaphore_sync, valor_semaphore_sync2;
    sem_getvalue(semaphore, &valor_semaphore);
    sem_getvalue(semaphore_sync, &valor_semaphore_sync);
    sem_getvalue(semaphore_sync2, &valor_semaphore_sync2);
    printf("Valor dos semaforos: %d %d %d (%d).\n", valor_semaphore, valor_semaphore_sync, valor_semaphore_sync2, getpid());
    fflush(stdout);

    // Preenche a memoria compartilhada
    printf("Eu sou o processo pai: estou preenchendo a memoria compartilhada (%d).\n", getpid()); //print de debug para testar o meu programa
    fflush(stdout);
    memoria_compartilhada[0] = vetor->tamanho;
    for(int i = 1; i <= vetor->tamanho; i++)
    {
        memoria_compartilhada[i] = vetor->conteudo[i-1];
    }


    //Vou liberar o processo filho
    printf("Eu sou o processo pai: Estou liberando o filho (%d).\n", getpid());
    fflush(stdout);
    sem_post(semaphore_sync2);

    //Faz o processamento do remove pares encima da memoria compartilhada
    fflush(stdout);
    sem_wait(semaphore); //só um pode processar por vez, senão da errado
    printf("Eu sou o processo pai: vou processar o vetor para remover os pares (%d).\n", getpid()); //print de debug para testar o meu programa
    // printf("Vou imprimir a memoria compartilhada de tamanho %d: (%d).\n",  memoria_compartilhada[0], getpid());
    // for(int i = 1; i <= memoria_compartilhada[0]; i++){
    //     printf("%d (%d).\n", memoria_compartilhada[i], getpid());
    //     fflush(stdout);
    // }
    for (int i =  memoria_compartilhada[0]; i >= 1; i--)
    {
        if (memoria_compartilhada[i] % 2 == 0)
        {
            remove_vetor((int *) &memoria_compartilhada[1], i-1, (int *) &memoria_compartilhada[0]);
        }
    }
    // printf("Vou imprimir após o processamento a memoria compartilhada de tamanho %d: (%d).\n",  memoria_compartilhada[0], getpid());
    // for(int i = 1; i <= memoria_compartilhada[0]; i++){
    //     printf("%d (%d).\n", memoria_compartilhada[i], getpid());
    //     fflush(stdout);
    // }
    sem_post(semaphore); //só um pode processar por vez, senão da errado


    //Pai espera o filho sincronização
    printf("Eu sou o processo pai: estou esperando o filho (%d).\n", getpid()); //print de debug para testar o meu programa
    fflush(stdout);
    sem_wait(semaphore_sync); 
    printf("Eu sou o processo pai %d: Fui liberado pelo filho.\n", getpid());
    fflush(stdout);

    // Passa a resposta para para a memória local
    vetor->tamanho = memoria_compartilhada[0];
    for(int i = 0; i < vetor->tamanho; i++)
    {
        vetor->conteudo[i] = memoria_compartilhada[i+1];
    }

    //Libera o semaforo 
    sem_unlink("sema1");
    sem_unlink("sema2");
    sem_unlink("sema3");
    sem_close(semaphore);  
    sem_close(semaphore_sync);  
    sem_close(semaphore_sync2);  

    //Libera a memoria compartilhada
    shmctl(memoria_compartilhada_id, IPC_RMID, 0);
}
 
int main()
{
    //INICIALIZA!!!!
    printf("Eu sou o processo: %d\n", getpid()); //Print para debugar o meu programa        
    fflush(stdout);
    Array vetor_sequencial, vetor_paralelo; //Um vetor para processamento sequencial outro para paralelo
    srand((unsigned) time(NULL)); //semente aleatoria baseada no sistema
    //srand(1); //semente fixa para testes
    clock_t comeco, fim; //copie da net para contar tempo

    //gerando os numeros aleatorios para o vetor
    printf("Preenchendo o vetor (%d).\n", getpid()); //print de debug para testar o meu programa
    fflush(stdout);
    vetor_sequencial.tamanho = TAM_VETOR;
    vetor_paralelo.tamanho = TAM_VETOR;
    for (int i = 0; i < TAM_VETOR; i++)
    { 
        int b = rand() % MAX_NUM + 1; //gera um numero de 1 ate 100 aleatorio 
        //printf("%d\n", b); //print de debug para testar o meu programa
        vetor_sequencial.conteudo[i] = b; //insere os números sorteados tanto no primeiro vetor quanto no segundo 
        vetor_paralelo.conteudo[i] = b; //insere os números sorteados tanto no primeiro vetor quanto no segundo 
    }

    comeco = clock(); //Comeca a contagem de tempo do paralelo
    printf("Estou criando ou obtendo a referencia dos semaforos (%d).\n", getpid());
    fflush(stdout);
    semaphore = sem_open("sema1", O_CREAT, 0777, 1); //Inicializa o semaforo com 1. Somente um processo vai mudar o vetor por vez
    semaphore_sync = sem_open("sema2", O_CREAT, 0777, 0);  //Inicializa o semaforo com 0. O filho seta esse semaforo para 1 para liberar o pai
    semaphore_sync2 = sem_open("sema3", O_CREAT, 0777, 0);  //Inicializa o semaforo com 0. O pai seta esse semaforo para 1 para liberar o filho
 
    // Aloca o segmento de memoria compartilhada com pedacos do tamanho de inteiros 4 bytes
    printf("Estou construindo a area de memoria compartilhada (%d).\n", getpid()); //print de debug para testar o meu programa
    fflush(stdout);
    memoria_compartilhada_id = shmget(CHAVE, sizeof(int) * (TAM_VETOR + 2), IPC_CREAT | 0777);
    if (memoria_compartilhada_id == -1)
    {
        printf("Erro na geracao da memoria compartilhada: %d\n", memoria_compartilhada_id);
        exit(1);
    }

    // Obtem uma referencia para o espaco de memoria compartilhado
    printf("Estou pegando a referencia de memoria compartilhada (%d).\n", getpid()); //print de debug para testar o meu programa
    fflush(stdout);
    memoria_compartilhada = shmat(memoria_compartilhada_id, NULL, 0);
    if (!memoria_compartilhada) 
    { 
        printf("Erro no attach da memoria compartilhada\n");
        exit(1);
    }

    //Cria o filho
    pid_filho = fork(); //O pai vai ter valor nao zero. O filho vai ter valor 0.
    
    if(pid_filho != 0){ //Processamento do pai
        comeco = clock(); //Comeca a contagem de tempo
        pai(&vetor_paralelo);
        fim = clock(); //Final da contagem de tempo
        printf("Tempo total de execucao paralelo: %f (s)\n", ((double) fim - comeco)/CLOCKS_PER_SEC);
        // printf("--Array paralelo--\n");
        // imprime_array(vetor_paralelo); //outro print para testar o meu programa
        // printf("---------\n");
    }
    else{ //Processamento do filho
       filho();
    }

    //!!!FAZENDO SEM FILHO!!!
    comeco = clock();
    simula_sem_filho(&vetor_sequencial);
    fim = clock(); //Final da contagem de tempo
    printf("Tempo total de execucao sequencial: %f (s)\n", ((double) fim - comeco)/CLOCKS_PER_SEC);
    //printf("--Array sequencial--\n");
    //imprime_array(vetor_sequencial); //outro print para testar o meu programa
    //printf("---------\n");

    //Comparando!!!!
    int resultado = verifica_resultado(&vetor_sequencial, &vetor_paralelo);
    if (resultado == 0) printf("O resultado está incorreto\n");
    else printf("O resultado está correto\n");

    return 0;
}