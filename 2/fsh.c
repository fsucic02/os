#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <limits.h>

void obradi_sigint(int sig);
pid_t child_id = -1;

int main(int argc, char *argv[]) {
	bool trebam_prekinuti = 0; // bool kojim pratimo je li korisnik unio "exit"
	char naredba[90];
	memset(&naredba,0,sizeof(naredba));
	
	// maskiranje signala sigint
	struct sigaction act;
	act.sa_flags = 0;
	act.sa_handler = obradi_sigint;
	sigaction(SIGINT, &act, NULL);
	
	while (!trebam_prekinuti) {
		memset(naredba,0,sizeof(naredba));
		printf("fsh> ");
		
		fgets(naredba, 90, stdin);
		if (naredba[0] == 0) continue;
		naredba[strlen(naredba)-1] = '\0'; // makivam \n
		
		if (strcmp(naredba, "exit") == 0 || strcmp(naredba, "\0") == 0) {
			// korisnik je unio prazan niz ili upisao exit, moramo prekinuti shell
			trebam_prekinuti = 1;
		} else {
			// korisnik je unio neku naredbu, moramo ju obraditi
			char* args[40]; 
			char *token = strtok(naredba, " ");
			int idx = 0;
			
			while (token != NULL) {
				args[idx++] = token;
				token = strtok(NULL, " ");
			}
			
			args[idx] = NULL; // posljednji argument mora biti NULL (man execv)
			
			if (strcmp(args[0], "cd") == 0) {
				// korisnik je unio cd, tu naredbu moramo posebno obraditi pomocu chdir-a
				if (chdir(args[1]) != 0) {
					fprintf(stderr, "cd: The directory '%s' does not exist\n", args[1]);
				}
			} else if (strcmp(args[0], "..") == 0) {
				chdir(args[0]);
			} else {
				// korisnik je unio naredbu koja nije cd, moramo ju obraditi tako da pretrazimo var okoline...
				child_id = fork();
				if (child_id == 0) {
					// proces dijete
					if (access(args[0], F_OK) == 0) {
						execv(args[0], args);
					} else {
						char *sadrzaj = getenv("PATH");
						char *split = strtok(sadrzaj, ":");
						while (split != NULL) {
							char *novanaredba = malloc(strlen(split) + strlen(args[0]) + 2);
							sprintf(novanaredba, "%s/%s", split, args[0]);
							if (access(novanaredba, F_OK) == 0) {
								execv(novanaredba, args);
								break;
							}
							split = strtok(NULL, ":");
						}
						printf("Unknown command: %s\n", args[0]);
						exit(0);	
					}
				} else {
					wait(NULL);
				}
			}
		}
	}
}


void obradi_sigint(int sig) {
	if (child_id == 0) {
		exit(1);
	} else if (child_id > 0) {
		kill(child_id, SIGINT);
		wait(NULL);	
		child_id = -1;
	}
	
	printf("\n");
}
