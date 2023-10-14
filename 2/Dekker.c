#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdatomic.h>

/* zajednicke varijable */
typedef struct {
	int pravo;
	int a;
	atomic_int zastavica[2];
} spremnik;

spremnik* buf;

void udi_u_kriticni_odsjecak(int i, int j) {
	buf->zastavica[i] = 1;
	while (buf->zastavica[j] != 0) {
		if (buf->pravo == j) {
			buf->zastavica[i] = 0;
			while (buf->pravo == j) {};
			buf->zastavica[i] = 1;
		}
	}
}

void izadi_iz_kriticnog_odsjecka(int i, int j) {
	buf->pravo = j;
	buf->zastavica[i] = 0;
}


void posao_roditelja(int M) {
	for (int i = 0; i < M; i++) {
		/* funkcija za postavljanje zastavica prije ulaska u k.o. */
		udi_u_kriticni_odsjecak(0, 1);
		/* k.o. */
		buf->a++;
		/* funkcija za postavljanje zastavica nakon izlaska iz k.o. */
		izadi_iz_kriticnog_odsjecka(0, 1);
		// printf("Roditelj uvecao za 1, a = %d\n", buf->a);
	}
}

void posao_djeteta(int M) {
	printf("Stvoreno dijete s PID-om: %lu\n", (long)getpid());
	
	for (int i = 0; i < M; i++) {
		/* funkcija za postavljanje zastavica prije ulaska u k.o. */
		udi_u_kriticni_odsjecak(1, 0);
		/* k.o. */
		buf->a++;
		/* funkcija za postavljanje zastavica nakon izlaska iz k.o. */
		izadi_iz_kriticnog_odsjecka(1, 0);
		// printf("Dijete uvecalo za 1, a = %d\n", buf->a);
	}
}

int main(int argc, char *argv[]) {
	printf("Stvoren roditelj s PID-om: %lu\n", (long)getpid());
	int ID = shmget(IPC_PRIVATE, sizeof(spremnik), IPC_CREAT | 0640);
	if (ID == -1) {
		printf("Nisam uspio!\n"); /* nismo uspjeli dodijeliti zajednicku memoriju */
		exit(0);
	}
	
	buf = (spremnik *)shmat(ID, NULL, 0);
	int M = atoi(argv[1]);
	buf->pravo = 0;
	buf->a = 0;
	
	switch(fork()) {
		case -1: printf("Nismo uspjeli.\n"); break;
		case 0: 
			posao_djeteta(M);
			exit(0);
		default:
			posao_roditelja(M);
	}

	
	wait(NULL);
	printf("%d\n", buf->a);
	
	/* brisanje zajednicke memorije */
	shmdt((char *) buf);
	shmctl(ID, IPC_RMID, NULL);
	
	printf("Glavni program je gotov.\n");
}
