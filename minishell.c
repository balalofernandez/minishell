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
					printf("se va a ejecutar el comando %s\n", line->commands[i].argv[0]);
					pid = fork();
					if (pid < 0) { // Error 
					fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
					exit(1);
					//continue;
					}
					else if (pid == 0) { // Proceso Hijo 
							execvp(line->commands[0].argv[0], line->commands[0].argv);
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
				printf("Vamos a hacer los dos primeros comandos\n");
				pid_t pid1,pid2;
				int p[2];

				pipe(p);
				pid1 = fork();//hijo1

				/*el programa que escribe en el pipe es el programa ls
				lo suyo es redireccionar la salida del ls
				lo ponemos para que apunte al pipe
				*/

				if(pid1 == 0){//aqui habla el hijo 1
					close(p[0]);//Cerramos el extremo de lectura porque el hijo solo escribe en el pipe
					dup2(p[1],1);//duplicamos la entrada del pipe en la salida estandar (1), redireccionamos stdout al in del pipe
					//close(p[1]) esto lo podemos hacer porque ya hemos puesto el descriptor del pipe en la salida
					execvp(line->commands[0].argv[0], line->commands[0].argv);
					exit(1);
				}
				pid2 = fork(); 
				if(pid2 == 0){//aqui habla el hijo 1
					close(p[1]);//Cerramos el extremo de escritura porque el hijo solo escribe en el pipe
					dup2(p[0],0);//duplicamos la salida del pipe en la entrada estandar (1), redireccionamos stdout al out del pipe
					//close(p[1]) esto lo podemos hacer porque ya hemos puesto el descriptor del pipe en la salida
					execvp(line->commands[1].argv[0], line->commands[1].argv);
					printf("Error en el execvp\n");
					exit(1);
				}

				//El padre sigue teniendo el pipe
				close(p[0]);
				close(p[1]);
				//si no hacemos estos close nunca va a acabar el proceso

				wait(NULL);//para el primer hijo
				printf("Un hijo terminó\n");
				wait(NULL);//LO SUYO ES PONER UN WAIT STATUS
				//Para saber cómo ha acabado
				printf("Los dos hijos han terminado\n");
				}
							/*for (j=0; j<line->commands[i].argc; j++) {
								printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
							}*/

		printf("msh>$ ");	
	}
	return 0;
}