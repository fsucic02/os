#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <stdlib.h>

#define DESNA 1
#define LIJEVA 0

/* globalne varijable - monitori, zajednicke varijable, ... */
pthread_mutex_t m;
pthread_cond_t lijeva_obala_red;
pthread_cond_t desna_obala_red;
pthread_cond_t camac_red;
int trenutni_idx = 0, broj_putnika = 0, broj_kanibala = 0, broj_misionara = 0;
int camac[7] = {0};
int desna_obala[201] = {0}, lijeva_obala[201] = {0};
int trenutna_obala = LIJEVA; // 1 je desna obala, 0 je lijeva
void print();
void print_camac();

void *camac_d(void *arg) {
	while (1) {
		pthread_mutex_lock(&m);
		trenutna_obala ^= 1;
		trenutni_idx = 0; broj_putnika = 0; broj_kanibala = 0; broj_misionara = 0;
		for (int i = 0; i < 7; i++) { camac[i] = 0; }
		
		if (trenutna_obala == DESNA) {
			printf("C: prazan na desnoj obali\n");
		} else {
			printf("C: prazan na lijevoj obali\n");
		}
	
		print();
		
		while (broj_putnika < 3) {
			pthread_cond_wait(&camac_red, &m); // ako nisu barem 3 putnika u camcu, ne mozemo krenuti
		}
		
		// ako smo do ovdje dosli, spremni smo za polazak
		printf("C: tri putnika ukrcana, polazim za jednu sekundu\n");
		print();
		
		pthread_mutex_unlock(&m);
		sleep(1); // spavamo jednu sekundu
		pthread_mutex_lock(&m);
		
		if (trenutna_obala == DESNA) {
			printf("C: prevozim s desne na lijevu obalu:");
		} else {
			printf("C: prevozim s lijeve na desnu obalu:");
		}
		
		print_camac();
		pthread_mutex_unlock(&m);
		
		if (trenutna_obala == DESNA) {
			pthread_cond_broadcast(&lijeva_obala_red);
		} else {
			pthread_cond_broadcast(&desna_obala_red);
		}
	}
}


void *kanibal(void *arg_kanibal) {
	pthread_mutex_lock(&m);
	int idx = *(int *)arg_kanibal;
	int obala = rand() % 2;
	if (obala == DESNA) {
		printf("K%d: došao na desnu obalu\n", idx);
		desna_obala[idx] += 2;
	} else {
		printf("K%d: došao na lijevu obalu\n", idx);
		lijeva_obala[idx] += 2;
	}
	
	print();
	
	while (!((broj_kanibala < broj_misionara || (broj_kanibala >= 0 && broj_kanibala == broj_putnika)) && obala == trenutna_obala) || broj_putnika == 7) {
		if (obala == DESNA) { 
			pthread_cond_wait(&desna_obala_red, &m); 
		} else { 
			pthread_cond_wait(&lijeva_obala_red, &m);
		}
	}
	
	broj_kanibala++;
	broj_putnika++;
	camac[trenutni_idx++] = 2*idx; // kanibale kodiramo parnim brojevima
	printf("K%d: ušao u čamac\n", idx);
	
	if (obala == DESNA) { 
		desna_obala[idx] -= 2;
	} else { 
		lijeva_obala[idx] -= 2; 
	}
    
	print();
	pthread_mutex_unlock(&m);
	
	if (broj_putnika >= 3) {
    	pthread_cond_signal(&camac_red);
    }
    
	if (obala == DESNA) { 
		pthread_cond_broadcast(&desna_obala_red);
	} else { 
		pthread_cond_broadcast(&lijeva_obala_red);
	}
	
}


void *misionar(void *arg_misionar) {
    pthread_mutex_lock(&m);
    int idx = *(int *)arg_misionar;
    int obala = rand() % 2;
    if (obala == DESNA) {
        printf("M%d: došao na desnu obalu\n", idx);
        desna_obala[idx] += 1;
    } else {
        printf("M%d: došao na lijevu obalu\n", idx);
        lijeva_obala[idx] += 1;
    }
    
    print();
    
    while ((broj_kanibala >= 2 && broj_kanibala == broj_putnika) || obala != trenutna_obala || broj_putnika == 7) {
        if (obala == DESNA) { 
            pthread_cond_wait(&desna_obala_red, &m); 
        } else { 
            pthread_cond_wait(&lijeva_obala_red, &m);
        }
    
    }

    broj_misionara++;
    broj_putnika++;
    camac[trenutni_idx++] = 2*idx + 1; // misionare kodiramo neparnim brojevima
    printf("M%d: ušao u čamac\n", idx);    
    
	if (obala == DESNA) { 
		desna_obala[idx] -= 1;
	} else { 
		lijeva_obala[idx] -= 1; 
	}
    
    print();
    pthread_mutex_unlock(&m);
    
    if (broj_putnika >= 3) {
    	pthread_cond_signal(&camac_red);
    }
    
	if (obala == DESNA) { 
		pthread_cond_broadcast(&desna_obala_red);
	} else { 
		pthread_cond_broadcast(&lijeva_obala_red);
	}
	
}


void *stvaraj_misionare() {
	pthread_t dretva_misionar;
	for (int i = 1;; i++) {
		pthread_create(&dretva_misionar, NULL, misionar, &i);
		sleep(2);
	}	
}

void *stvaraj_kanibale() {
	pthread_t dretva_kanibal;
	int argumenti[201];
	for (int i = 1; i < 201; i++) argumenti[i] = i;
	for (int j = 1;; j++) {
		pthread_create(&dretva_kanibal, NULL, kanibal, &argumenti[j]);
		sleep(1);
	}
}

int main() {
	srand((unsigned)time(NULL));
	pthread_t dretve[3];
	int broj_dretvi[3];
	
	pthread_create(&dretve[0], NULL, camac_d, &broj_dretvi[0]);
	pthread_create(&dretve[1], NULL, stvaraj_misionare, &broj_dretvi[1]);
	pthread_create(&dretve[2], NULL, stvaraj_kanibale, &broj_dretvi[2]);
	
	for (int i = 0; i < 3; i++) {
		pthread_join(dretve[i], NULL);
	}
	
}

void print() {
	printf("C[%c]={", trenutna_obala == DESNA ? 68 : 76);
	
	for (int i = 0; i < 7; i++) {
		if (camac[i] == 0) { break; }
		else {
			if (camac[i] % 2 == 0) {
				// kanibal je
				printf(" K%d ", camac[i] / 2);
			} else {
				// misionar je
				printf(" M%d ", (camac[i] - 1) / 2);
			}
		}
	}
	
	printf("} LO={");
	
	for (int i = 1; i < 200; i++) {
		if (lijeva_obala[i] == 1) {
			// misionar
			printf(" M%d ", i);
		} else if (lijeva_obala[i] == 2) {
			printf(" K%d ", i);
		} else if (lijeva_obala[i] == 3) {
			printf(" M%d  K%d ", i, i);
		}
	}
	
	printf("} DO={");
	
	for (int i = 1; i < 200; i++) {
		if (desna_obala[i] == 1) {
			// misionar
			printf(" M%d ", i);
		} else if (desna_obala[i] == 2) {
			printf(" K%d ", i);
		} else if (desna_obala[i] == 3) {
			printf(" M%d  K%d ", i, i);
		}
	}
	
	printf("}\n\n");
}

void print_camac() {
	for (int i = 0; i < 7; i++) {
			if (camac[i] == 0) { break; }
			else {
				if (camac[i] % 2 == 0) {
					// kanibal je
					printf(" K%d ", camac[i] / 2);
				} else {
					// misionar je
					printf(" M%d ", (camac[i] - 1) / 2);
				}
			}
		}
		
	printf("\n\n");
	
	if (trenutna_obala == DESNA) {
		printf("C: preveo s desne na lijevu obalu:");
	} else {
		printf("C: preveo s lijeve na desnu obalu:");
	}
	
	for (int i = 0; i < 7; i++) {
		if (camac[i] == 0) { break; }
		else {
			if (camac[i] % 2 == 0) {
				// kanibal je
				printf(" K%d ", camac[i] / 2);
			} else {
				// misionar je
				printf(" M%d ", (camac[i] - 1) / 2);
			}
		}
	}
	
	printf("\n");
}
