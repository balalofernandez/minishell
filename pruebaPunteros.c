#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

typedef struct Nodo{
    int pid;
    struct Nodo *sig;
} TNodo;

typedef TNodo *TLista;


//FUNCIONES
void nuevoTrabajo(int pid);
void procesoTerminado(TLista *primeroJobs);
void jobs();
void jobTerminado(int pid);

//GLOBALES
//Inicializamos la cola del jobs
    TLista primeroJobs = NULL;
    TLista ultimoJobs = NULL;
    TLista primeroFinalizado = NULL;//esta es la lista de finalizados

int main(){
    nuevoTrabajo(4);
    nuevoTrabajo(2);
    nuevoTrabajo(3);
    jobs();
    jobTerminado(2);
    printf("\n");
    jobs();
}

void nuevoTrabajo(int pid){
    TLista nuevo;

    nuevo = (TLista) malloc(sizeof(TNodo));
    nuevo->pid = pid;
    nuevo->sig = NULL;
    if(ultimoJobs){//Si el último no es vacío
        (ultimoJobs)->sig = nuevo;
    }
    ultimoJobs = nuevo;
    // printf("%s\n", (*ultimoJobs)->nombre);
    if(!primeroJobs){//Caso en el que la lista sea vacía
        primeroJobs = nuevo;
    } 
}

void procesoTerminado(TLista *primeroJobs){
    TLista aux;

    if(!*primeroJobs){//caso en el que el primeroJobs sea NULL
        fprintf( stderr , "Error: No hay más procesos");
        exit(1);
    }
    aux = *primeroJobs;
    *primeroJobs = (*primeroJobs)->sig;
    free(aux);

}

void jobs(){
    int i = 1;
    TLista aux;
    aux = primeroJobs;
    while(aux){
        printf("[%d]\n", aux->pid);
        aux = aux->sig;
        i++;
    }
    free(aux);
}

void jobTerminado(int p){
    TLista aux1,aux2;
    aux1 = primeroJobs;
    if(primeroJobs->pid == p){
        aux2 = primeroJobs;
        printf("Es el primero");
    }
    else{
        aux2 = primeroJobs->sig;
    }
    while(aux2 && aux2->pid != p){
        aux1 = aux2;
        aux2 = aux2->sig;
    }
    if(!aux2){//caso en el que el primeroJobs sea NULL
        fprintf( stderr , "Error: No se ha encontrado el proceso");
        exit(1);
    }
    else{//hemos encontrado el pid
        aux1->sig = aux2->sig;
        if (primeroFinalizado){//añadimos el proceso a la lista de finalizados
            aux2->sig = primeroFinalizado->sig;
            primeroFinalizado = aux2;
        }
        else{
            aux2->sig = NULL;
            primeroFinalizado = aux2;
            // printf("%d", primeroFinalizado->pid);
        } 
    }
    free(aux1);
    free(aux2);
}