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
		for (i=0; i<line->ncommands; i++) {
			printf("orden %d (%s):\n", i, line->commands[i].filename);
			//NO se si se crea el hijo aqui, a lo mejor tengo un bucle infinito
			
			pid_t pid;
			int status;
			if (line->commands->argc > 0){
				printf("se va a ejecutar el comando %s\n", line->commands[i].argv[0]);
				pid = fork();
				if (pid < 0) { /* Error */
                fprintf(stderr, "Falló el fork().\n%s\n", strerror(errno));
                exit(1);
				//continue;
				}
				else if (pid == 0) { /* Proceso Hijo */
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

			}

			/*for (j=0; j<line->commands[i].argc; j++) {
				printf("  argumento %d: %s\n", j, line->commands[i].argv[j]);
			}*/
		}
		printf("msh>$ ");	
	}
	return 0;
}