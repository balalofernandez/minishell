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


#include "parser.h"

typedef struct job{
    int pid;
    char nombre[1024];
} TJob;



//FUNCIONES
void ejecutar1(int bg, tline *line);
void ejecutarN(int bg, tline *line);
// void ejecutarN(int bg, TLista *Cola, tline *line);
void nuevoTrabajo(pid_t pid, char* trabajo);
// void procesoTerminado(TLista *primeroJobs);
void jobs();
void jobTerminado(pid_t pid);
void manejador(int sig);
void cd (tline *line);
//GLOBALES
//Inicializamos la cola del jobs
int contadorSinFinalizar;//contador de los procesos
int contadorFinalizado;//contador de los procesos finalizados
TJob *procesosBackground;//Lista con los procesos en background
int * pidFinalizados;//Lista con los procesos finalizados

int main(void){
    //Inicializamos
    contadorFinalizado = 0;
    contadorSinFinalizar = 0;
    procesosBackground = (TJob *) malloc(sizeof(TJob));
    pidFinalizados = (int *) malloc(sizeof(int));

    char directorio[512];
    char buf[1024];//el buffer para leer de la entrada
	tline * line;//la línea que leemos
	int i,j;
    int fd;
    char *fichero;

    //guardamos los descriptores de la entrada, la salida y el error para si hay un redirección poder volver 
    //a la estándar
    int fdEntrada = dup(fileno(stdin));
	int fdSalida = dup(fileno(stdout));
	int fdError = dup(fileno(stderr));

    //SEÑALES
    signal(SIGUSR1,manejador);
    signal(SIGINT,SIG_IGN);//si nos hacen ctrl+c no cancelamos
    signal(SIGQUIT,SIG_IGN);//si nos hacen ctrl+\ no cancelamos

    printf("%s=> ",getcwd(directorio, 512));//imprimimos por primera vez el prompt
    //Vamos a hacer un bucle infinito que lea de la entrada estándar
    while(fgets(buf,1024,stdin)){
        line = tokenize(buf);//recogemos la info de la línea

        signal(SIGINT,SIG_IGN);//si nos hacen ctrl+c no cancelamos
        signal(SIGQUIT,SIG_IGN);//si nos hacen ctrl+\ no cancelamos


        if (line==NULL || !strcmp(buf, "\n")) {//Queremos que el bucle vuelva a empezar si han pulsado "ENTER"
            printf("%s=> ",getcwd(directorio, 512));
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
        if(line->ncommands == 1 && strcmp((char *) line->commands[0].argv[0], "jobs") && strcmp(line->commands[0].argv[0],"cd")){
            pid_t pid;//aqui vamos a guardar el pid de los procesos
            if (line->commands->argc > 0){
                if(line->background){
                    pid = fork();//creamos el hijo
                    if (pid < 0) { //Mensaje de error por si falla el fork
                        fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                        exit(1);
                    }
                    if (pid == 0) {
                        ejecutar1(line->background,line);
                        kill(getppid(), SIGUSR1);
                        exit(0);                        
                    }
                    else{
                        nuevoTrabajo(pid,buf);//introducimos al hijo en la lista de bg
                        printf("[%d]\n", pid);//mostramos el pid del proceso por el que espera
                    }
                }
                else if(!line->background){
                    ejecutar1(line->background,line);
                }                
            }
        }
        else if (!strcmp((char *) line->commands[0].argv[0], "jobs")){//caso en el que se ejecute jobs
            printf("vamos a ejecutar el jobs\n");
            jobs();
        }
        else if ((!strcmp((char *) line->commands[0].argv[0],"cd")) && line->ncommands == 1){//caso en el que se ejecute el cd
            cd(line);
        }
        
        else if(line->ncommands > 1){//comprobamos que nos meten más de un comando
            if(line->background){
                pid_t pid = fork();//creamos el hijo
                if (pid < 0) { //Mensaje de error por si falla el fork
                    fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                    exit(1);
                }
                if (pid == 0) {
                    ejecutarN(line->background,line);
                    exit(0);                      
                }
                else{
                    nuevoTrabajo(pid, buf);//introducimos al hijo en la lista de bg
                    printf("[%d]\n", pid);//mostramos el pid del proceso por el que espera
                }
            }
            else if(!line->background){
                ejecutarN(line->background,line);
            } 
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

        printf("%s=> ",getcwd(directorio, 512));	
    }

}

void ejecutar1(int bg, tline *line){
    pid_t pidp1;
    int status;
    pidp1 = fork();
    if (pidp1 < 0) { //Mensaje de error por si falla el fork
        fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
        exit(1);
    }
    else if (pidp1 == 0) { // Proceso Hijo 
        if(bg){
            signal(SIGINT,SIG_IGN);//si nos hacen ctrl+c no cancelamos
            signal(SIGQUIT,SIG_IGN);//si nos hacen ctrl+\ no cancelamos
        }
        else if (!bg){
            signal(SIGINT,SIG_DFL);//si nos hacen ctrl+c cancelamos
            signal(SIGQUIT,SIG_DFL);//si nos hacen ctrl+\ cancelamos
        }
        if (line->commands[0].filename == NULL) {
            fprintf(stderr, "%s : No se encuentra el mandato.\n", line->commands[0].argv[0]);
        }
        else{
            execvp(line->commands[0].argv[0], line->commands[0].argv); //ejecutar el primer comando
            //Si llega aquí es que se ha producido un error en el execvp
            exit(1);
        } 
    }
    else {
        wait(&status);//hacemos un wait del padre
        if(WIFEXITED(status) != 0)//comprobamos que el hijo haya terminado
            if(WEXITSTATUS(status) != 0)//vemos a ver si ha resultado incorrecta la resolucion del mandato
                printf("El comando no se ejecutó correctamente\n");
    }
}

void ejecutarN(int bg, tline *line){
    pid_t pid1, pid2;
    int i;
    int status;
    int p[2];//aqui declaramos los descriptores de fichero
    int entrada;//aqui vamos a guardar la salida de un pipe para ponerlo en la entrada del siguiente
    entrada = 0; //inicializamos la entrada aunque el primer valor es irrelevante
    //pues se va a guardar la salida cuando se ejecute la primera iteración
    pid1 = fork();
    if(pid1==0){
        if(bg){
            signal(SIGINT,SIG_IGN);//si nos hacen ctrl+c no cancelamos
            signal(SIGQUIT,SIG_IGN);//si nos hacen ctrl+\ no cancelamos
        }
        else if (!bg){
            signal(SIGINT,SIG_DFL);//si nos hacen ctrl+c cancelamos
            signal(SIGQUIT,SIG_DFL);//si nos hacen ctrl+\ cancelamos
        }
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
                if (!strcmp((char *) line->commands[i].argv[0], "jobs")){
                    jobs();
                }
                else{
                    execvp( line->commands[i].argv[0], line->commands[i].argv);//ejecutamos el comando
                    exit(1);//por si falla la ejecución
                }
                
            }

            close(p[1]);//cerramos el extremo de escritura

            entrada = p[0];//metemos la salida del pipe en lo que será la entrada del siguiente
        }

        dup2(entrada, 0);//para el último mandato duplicamos la salida del pipe en la entrada estándar
        close(entrada);
        close(p[1]);

        //ejecutamos el último mandato
        execvp(line->commands[line->ncommands-1].argv[0], line->commands[line->ncommands -1].argv);
        exit(1);//por si falla la ejecución
    }
    waitpid(pid1, &status, 0);// vamos a esperar al hijo que nos da la solución
}

void cd(tline *line){
    char *dir;
    if (line->commands->argc>2){
        fprintf(stderr,"Error; uso: cd [directorio]\n");
    }
    else{
        if (line->commands->argc==1){
            dir = getenv("HOME");
        }
        else {
            dir = line->commands->argv[1];
        }
        if (chdir(dir)){//cambiamos el directorio
            fprintf(stderr,"%s no encontrado\n",dir);
        }
        
    }
}

void nuevoTrabajo(pid_t pid, char* trabajo){
    (procesosBackground + contadorSinFinalizar)->pid = pid;
    strcpy((procesosBackground + contadorSinFinalizar)->nombre, trabajo);
    contadorSinFinalizar ++;
    procesosBackground = (TJob *) realloc(procesosBackground, sizeof(TJob)*(contadorSinFinalizar + 1));
}

void jobs(){
    int i;
    for(i=0; i<contadorFinalizado; i++){
        jobTerminado(*(pidFinalizados + i));
    }
    contadorFinalizado -= i;
    for(i=0; i<contadorSinFinalizar; i++){
        fprintf(stderr,"[%d]   Running                 %s\n", i+1, (procesosBackground+i)->nombre);
    }
}

void jobTerminado(int p){
    int i;
    int encontrado = 0;
    //Vamos a hacer un bucle que copie todos aquellos elementos que no son el pid que buscamos y que elimine el que si buscamos
    TJob *aux = (TJob*) malloc(sizeof(TJob)*(contadorSinFinalizar));
	for (i = 0; i < contadorSinFinalizar; i++){
		if ((p != (procesosBackground + i)->pid) && (!encontrado)) {
			(aux + i)->pid = (procesosBackground + i)->pid;
			strcpy((aux + i)->nombre, (procesosBackground + i)->nombre);			
		}
		else if(p == (procesosBackground + i)->pid){
			encontrado = 1;
			//j = i;
		}
		else{
			(aux + (i-1))->pid = (procesosBackground + i)->pid;
			strcpy((aux + (i-1))->nombre, (procesosBackground + i)->nombre);
		}
	}
    if(encontrado){
        printf("se ha encontrado el proceso\n");
        contadorSinFinalizar--;
        procesosBackground = (TJob*) realloc(procesosBackground, sizeof(TJob)*(contadorSinFinalizar));
                
        printf("%d\n",contadorSinFinalizar);
        procesosBackground = aux;
    }
    else{
        fprintf(stderr,"no se ha encontrado el proceso\n");
    }
}

void manejador(int sig){
    if(sig == SIGUSR1){
        int pidTerminado = wait(NULL);//cuando termine el background el padre da el pid
        *(pidFinalizados + (contadorFinalizado)) = pidTerminado;
        contadorFinalizado++;
        pidFinalizados = (int *) realloc(pidFinalizados,sizeof(int)*(contadorFinalizado+1));

    }
}