#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "parser.h"

typedef struct Nodo{
    char *nombre;
    struct Nodo *sig;
} TNodo;

typedef TNodo *TCola;



//FUNCIONES
void nuevoTrabajo(TCola *primero, TCola *ultimo, char *trabajo);
void procesoTerminado(TCola *primero);
void jobs(TCola *primero);

int main(void){
    char buf[1024];//el buffer para leer de la entrada
	tline * line;//la línea que leemos
	int i,j;
    int fd;
    char *fichero;

    //Inicializamos la cola del jobs
    TCola primero = NULL;
    TCola ultimo = NULL;

    //guardamos los descriptores de la entrada, la salida y el error para si hay un redirección poder volver 
    //a la estándar
    int fdEntrada = dup(fileno(stdin));
	int fdSalida = dup(fileno(stdout));
	int fdError = dup(fileno(stderr));

    signal(SIGINT,SIG_IGN);//si nos hacen ctrl+c no cancelamos
    signal(SIGQUIT,SIG_IGN);//si nos hacen ctrl+\ no cancelamos

    printf("msh>$ ");//imprimimos por primera vez el prompt
    //Vamos a hacer un bucle infinito que lea de la entrada estándar
    while(fgets(buf,1024,stdin)){
        line = tokenize(buf);//recogemos la info de la línea

        signal(SIGINT,SIG_IGN);//si nos hacen ctrl+c no cancelamos
        signal(SIGQUIT,SIG_IGN);//si nos hacen ctrl+\ no cancelamos


        if (line==NULL) {//Queremos que el bucle vuelva a empezar si han pulsado "ENTER"
			continue;
		}
        if (line->redirect_input != NULL) {
            fichero = line->redirect_input;
			fd = open(fichero, O_CREAT | O_RDONLY); 
            if(fd == -1){
                fprintf( stderr , "%s : Error. %s\n" , fichero , strerror(errno)); 
                return 1;
            } else { 
                dup2(fd,0); // Duplicamos el descriptor del fichero en la entrada estándar
            }
		}
		if (line->redirect_output != NULL) {
            fichero = line->redirect_output;
            fd = creat(fichero, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH); // Le damos tanto permisos de lectura como escritura
            if(fd == -1){
                fprintf( stderr , "%s : Error. %s\n" , fichero , strerror(errno)); 
                return 1;
            } else { 
                dup2(fd,1); // Duplicamos el descriptor del fichero en la salida estándar
            }
		}
		if (line->redirect_error != NULL) {
            fichero = line->redirect_error;
			fd = creat (fichero ,  S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            if(fd == -1){
                fprintf( stderr , "%s : Error. %s\n" , fichero , strerror(errno)); 
                return 1;
            } else { 
                dup2(fd,2); // Duplicamos el descriptor del fichero en la salida de error
            }	
		}
        


        //Ahora vamos a hacer un for que vaya leyendo los distintos mandatos que tenemos
        //de forma que por cada mandato tengamos creamos una serie de pipes
        //para concatenar los mandatos.
        //Primero vamos a tener un caso base que es que solamente tenemos un único mandato
        //por lo que ejecutamos el mandato con execvp.
        if(line->ncommands == 1){
            printf("Vamos a hacer el primer comando\n");
            pid_t pid;//aqui vamos a guardar el pid de los procesos
            int status;
            if (line->commands->argc > 0){
                printf("Se va a ejecutar el comando %s\n", line->commands[0].argv[0]);
                pid = fork();//creamos el hijo
                if (pid < 0) { //Mensaje de error por si falla el fork
                fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                exit(1);
                }
                else if (pid == 0) { // Proceso Hijo 
                    signal(SIGINT,SIG_DFL);//si nos hacen ctrl+c cancelamos
                    signal(SIGQUIT,SIG_DFL);//si nos hacen ctrl+\ cancelamos
                    if (!strcmp((char *) line->commands[0].argv[0], "jobs")){
                        jobs(&primero);
                    }
                    // esCd();
                    // esFg();
                    execvp(line->commands[0].argv[0], line->commands[0].argv); //ejecutar el primer comando
                    //Si llega aquí es que se ha producido un error en el execvp
                    printf("Error al ejecutar el comando: %s\n", strerror(errno));
                    exit(1);
                }
                else {
                    if(!line->background){
                        wait(&status);//hacemos un wait del padre
                        if(WIFEXITED(status) != 0)//comprobamos que el hijo haya terminado
                            if(WEXITSTATUS(status) != 0)//vemos a ver si ha resultado incorrecta la resolucion del mandato
                                printf("El comando no se ejecutó correctamente\n");
                    }
                    if (line->background) {
                        nuevoTrabajo(&primero,&ultimo,line->commands[0].argv[0]);
                        printf("comando a ejecutarse en background\n");
                        printf("msh>$ ");
                        continue;
                    } 
                }
            }
        }
        //Ahora vamos a hacer el for
        //for(i=0; i< line->ncommands; i++){
        else if(line->ncommands > 1){//comprobamos que nos meten más de un comando
            pid_t pid1, pid2;
            int status;
            int p[2];//aqui declaramos los descriptores de fichero
            int entrada;//aqui vamos a guardar la salida de un pipe para ponerlo en la entrada del siguiente
            entrada = 0; //inicializamos la entrada aunque el primer valor es irrelevante
            //pues se va a guardar la salida cuando se ejecute la primera iteración
            pid1 = fork();
            if(pid1==0){

                signal(SIGINT,SIG_DFL);//si nos hacen ctrl+c cancelamos el proceso
                signal(SIGQUIT,SIG_DFL);//si nos hacen ctrl+\ cancelamos el proceso

                for(i = 0; i< line->ncommands-1 ; i++){
                    pipe(p);//inicializamos la tubería

                    pid2 = fork();
                    if (pid2 == 0){

                        /*Aqui queremos hacer una iteración normal del bucle 
                        en primer lugar vamos a duplicar la entrada del pipe en la salida del último pipe (la entrada del nuevo)
                        en la entrada estándar y luego vamos a duplicar la salida del pipe en la salida estándar*/

                        if( entrada != 0 ){
                            dup2(entrada,0);//duplicamos salida del ultimo pipe en la entrada estándar
                            close(entrada);//cerramos la salida del ultimo pipe
                        }
                        if(p[1] != 1){
                            dup2(p[1],1);//duplicamos la entrada del pipe en la salida estándar
                            close(p[1]);//cerramos la entrada del pipe
                        }

                        execvp( line->commands[i].argv[0], line->commands[i].argv);//ejecutamos el comando
                    }

                    close(p[1]);//cerramos el extremo de escritura

                    entrada = p[0];//metemos la salida del pipe en lo que será la entrada del siguiente
                }

                dup2(entrada, 0);//para el último mandato duplicamos la salida del pipe en la entrada estándar
                close(entrada);
                close(p[1]);

                //ejecutamos el último mandato
                execvp(line->commands[line->ncommands-1].argv[0], line->commands[line->ncommands -1].argv);
            }

            waitpid(pid1, &status, 0);// vamos a esperar al hijo que nos da la solución
            printf("El hijo ha terminado\n");

		}

        //con esto volvemos a la estándar
        if (line->redirect_input != NULL) {
			dup2(fdEntrada, 0);
            printf("\n");
		}
		if (line->redirect_output != NULL) {
			dup2(fdSalida, 1);
            printf("\n");
		}
		if (line->redirect_error != NULL) {
			dup2(fdError, 2);
            printf("\n");
		}

        printf("msh>$ ");
    }

}

void nuevoTrabajo(TCola *primero, TCola *ultimo, char* trabajo){
    TCola nuevo;

    nuevo = (TCola) malloc(sizeof(TNodo));
    nuevo->nombre = trabajo;
    nuevo->sig = NULL;
    if(*ultimo){//Si el último no es vacío
        (*ultimo)->sig = nuevo;
    }
    *ultimo = nuevo;
    if(!*primero){//Caso en el que la lista sea vacía
        *primero = nuevo;
    } 
}

void procesoTerminado(TCola *primero){
    TCola aux;

    if(!*primero){//caso en el que el primero sea NULL
        fprintf( stderr , "Error: No hay más procesos");
        exit(1);
    }
    aux = *primero;
    *primero = (*primero)->sig;
    free(aux);

}

void jobs(TCola *primero){
    int i = 1;
    TCola aux;
    aux = *primero;
    while(aux->sig){
        fprintf(stderr,"[%d]   Running                 %s\n", i, aux->nombre);
        aux = aux->sig;
        i++;
    }
    free(aux);
}