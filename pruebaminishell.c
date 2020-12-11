#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "parser.h"

int main(void){
    char buf[1024];//el buffer para leer de la entrada
	tline * line;//la línea que leemos
	int i,j;
    printf("msh>$ ");//imprimimos por primera vez el prompt
    //Vamos a hacer un bucle infinito que lea de la entrada estándar
    while(fgets(buf,1024,stdin)){
        line = tokenize(buf);//recogemos la info de la línea
        if (line==NULL) {//Queremos que el bucle vuelva a empezar si han pulsado "ENTER"
			continue;
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
                    execvp(line->commands[0].argv[0], line->commands[0].argv); //ejecutar el primer comando
                    //Si llega aquí es que se ha producido un error en el execvp
                    printf("Error al ejecutar el comando: %s\n", strerror(errno));
                    exit(1);
                }
                else {
                    wait(&status);//hacemos un wait del padre
                    if(WIFEXITED(status) != 0)//comprobamos que el hijo haya terminado
                        if(WEXITSTATUS(status) != 0)//vemos a ver si ha resultado incorrecta la resolucion del mandato
                            printf("El comando no se ejecutó correctamente\n");
                }
            }
        }
        //Ahora vamos a hacer el for
        //for(i=0; i< line->ncommands; i++){
        else if(line->ncommands > 1){//comprobamos que nos meten más de un comando
            pid_t pid[line->ncommands];//vamos a crear tantos hijos como comandos tengamos
            int p[line->ncommands -1][2];//Declaramos tantas pipes como hijos-1
            int status;
            //Ahora inicializamos las pipes
            for (i = 0; i<line->ncommands-1;i++){
                pipe(p[i]);
            }

            for (j=0; j<line->ncommands; j++){
                //pipe(p[j]);//inicializamos el pipe
                pid[j]=fork();
                if (pid[j] < 0) { //Mensaje de error por si falla el fork
                    fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                    exit(1);
                }
                if(pid[j] == 0){//Habla el hijo
                    if(j == 0){//el primer hijo
                        close(p[j][0]);//Cerramos el extremo de lectura porque el hijo solo escribe en el pipe
                        dup2(p[j][1],1);//duplicamos la entrada del pipe en la salida estandar (1), redireccionamos stdout al in del pipe
                        //close(p[j][1]) esto lo podemos hacer porque ya hemos puesto el descriptor del pipe en la salida
                        execvp(line->commands[j].argv[0], line->commands[j].argv);
                        //solo pasa a este código si falla el execvp
                        fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
						exit(1);
                    }
                    else if ( j == line->ncommands-1){// Habla el último hijo
                        close(p[j-1][1]);//Cerramos el extremo de escritura porque el hijo solo escribe en el pipe
                        dup2(p[j-1][0],0);//duplicamos la salida del pipe en la entrada estandar (1), redireccionamos stdout al out del pipe
                        //close(p[j][0]) esto lo podemos hacer porque ya hemos puesto el descriptor del pipe en la salida
                        execvp(line->commands[j].argv[0], line->commands[j].argv);
                        //solo pasa a este código si falla el execvp
                        fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
						exit(1);
                    }
                    else{ //Hijos intermedios
                        close(p[i][0]);	//cierro pues del nuevo pipe no vamos a leer
                        close(p[i-1][1]);	//cierro pues del anterior pipe no vamos a escibir
                        dup2(p[i-1][0],0);  //duplicamos la salida del pipe en la entrada estandar (1), redireccionamos stdout al out del pipe
                        dup2(p[i][1],1);    //duplicamos la entrada del pipe en la salida estandar (1), redireccionamos stdout al in del pipe
                        execvp(line->commands[j].argv[0], line->commands[j].argv);
                        //solo pasa a este código si falla el execvp
                        fprintf(stderr, "Error al ejecutar el comando: %s\n", strerror(errno));
						exit(1);

                    }
                }
                else{//habla el padre
                    if(!(j==(line->ncommands-1))){ // Por esto la violacion del core
							close(p[j][1]);
						}
                }
            }
            //esperamos por los hijos
            waitpid(-1,&status,0);



            // for(i=0;i<line->ncommands-1;i++){
			// 	free(p[i]);
			// }
			// free(p);
        }
        printf("msh>$ ");
    }

}