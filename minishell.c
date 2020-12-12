#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "parser.h"

int
main(void) {
	char buf[1024];
	tline * line;
	int i,j;

	printf("msh>$ ");	
	while (fgets(buf, 1024, stdin)) {
		
		line = tokenize(buf);
		if (line==NULL) {
			continue;
		}
		if (line->redirect_input != NULL) {
			printf("redirección de entrada: %s\n", line->redirect_input);
		}
		if (line->redirect_output != NULL) {
			printf("redirección de salida: %s\n", line->redirect_output);
		}
		if (line->redirect_error != NULL) {
			printf("redirección de error: %s\n", line->redirect_error);
		}
		if (line->background) {
			printf("comando a ejecutarse en background\n");
		} 
		/*for (i=0; i<line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			//NO se si se crea el hijo aqui, a lo mejor tengo un bucle infinito
			//VAMOS A HACER UN EJEMPLO ESTATICO POR SI NOS MANDAN 2 ARGUMENTO
			pid_t pid;
			int status;
			if (line->commands->argc > 0){
				printf("se va a ejecutar el comando %s\n", line->commands[i].argv[0]);
				pid = fork();
				if (pid < 0) { // Error 
                fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                exit(1);
				//continue;
				}
				else if (pid == 0) { // Proceso Hijo 
						execvp(line->commands[i].argv[0], line->commands[i].argv);
						//Si llega aquí es que se ha producido un error en el execvp
						printf("Error al ejecutar el comando: %s\n", strerror(errno));
						exit(1);

				}
				else {
					wait(&status);
					if(WIFEXITED(status) != 0)
						if(WEXITSTATUS(status) != 0)
							printf("El comando no se ejecutó correctamente\n");
					continue;
				}

			}*/

			//VAMOS A HACER UN EJEMPLO ESTATICO POR SI NOS MANDAN 2 ARGUMENTO
			if(line->ncommands == 1){
				printf("Vamos a hacer el primer comando\n");
				pid_t pid;
				int status;
				if (line->commands->argc > 0){
					printf("se va a ejecutar el comando %s\n", line->commands[0].argv[0]);
					pid = fork();
					if (pid < 0) { // Error 
					fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
					exit(1);
					//continue;
					}
					else if (pid == 0) { // Proceso Hijo 
							//execvp("ls","ls","-l")
							execvp(line->commands[0].argv[0], line->commands[0].argv); //ejecutar el primer comando
							//Si llega aquí es que se ha producido un error en el execvp
							printf("Error al ejecutar el comando: %s\n", strerror(errno));
							exit(1);
					}
					else {
						wait(&status);
						if(WIFEXITED(status) != 0)
							if(WEXITSTATUS(status) != 0)
								printf("El comando no se ejecutó correctamente\n");
						//podriamos hacer un continue pero no nos va a mostrar el prompt
					}
				}
			}

			if(line->ncommands > 1){
				
				pid_t pid;
				int p[2];//aqui declaramos los descriptores de fichero
				int entrada;//aqui vamos a guardar la salida de un pipe para ponerlo en la entrada del siguiente
				entrada = 0; //inicializamos la entrada aunque el primer valor es irrelevante
				//pues se va a guardar la salida cuando se ejecute la primera iteración
				for(i = 0; i< line->ncommands-1 ; i++){
					pipe(p);//inicializamos la tubería

					pid = fork();
					if (pid == 0){

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

				//ejecutamos el último mandato
				execvp(line->commands[line->ncommands-1].argv[0], line->commands[line->ncommands -1].argv);

			}
							/*for (j=0; j<line->commands[i].argc; j++) {
								printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
							}*/

		printf("msh>$ ");	
	}
	return 0;
}