#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdatomic.h>

int N, M;
int a = 0; /* zajednicka varijabla */
atomic_int *ulaz, *broj;

int find_max() {
	int max = broj[0];
	for (int i = 1; i < N; i++) {
		if (broj[i] > max)
			max = broj[i];
	}
			
	return max; 
}
void udi_u_kriticni_odsjecak(int i) {
	ulaz[i] = 1;
	broj[i] = find_max() + 1;
	ulaz[i] = 0;
	
	for (int j = 0; j < N; j++) {
		while (ulaz[j] != 0) {}
		while (broj[j] != 0 && (broj[j] < broj[i] || (broj[j] == broj[i] && j < i))) {}
	}
}

void izadi_iz_kriticnog_odsjecka(int i) {
	broj[i] = 0;
}


/* funkcija koju ce dretve obavljati */
void *posao_dretve(void *arg) {
	int *i = (int *)arg;
	/* funkcija kojom ulazimo u k.o. */
	for (int j = 0; j < M; j++) {
		udi_u_kriticni_odsjecak(*i);
		a++;
		izadi_iz_kriticnog_odsjecka(*i);
	}
}

int main(int argc, char *argv[]) {
	N = atoi(argv[1]);
	M = atoi(argv[2]);
	ulaz = (atomic_int *)calloc(N, sizeof(int));
	broj = (atomic_int *)calloc(N, sizeof(int));
	
	pthread_t dretve[N];
	int broj_dretvi[N];
	
	for (int i = 0; i < N; i++) {
		broj_dretvi[i] = i;
		pthread_create(&dretve[i], NULL, posao_dretve, &broj_dretvi[i]);
	}
	
	for (int i = 0; i < N; i++) {
		pthread_join(dretve[i], NULL);
	}
	
	printf("%d\n", a);
	free(ulaz);
	free(broj);
}	
