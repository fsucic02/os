#include <stdio.h>
#include <stdbool.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <stdlib.h>
#define N 3

/* globalne varijable */
sem_t *otvoreno; 
sem_t *br_mjesta;
sem_t *cekam_klijenta;
sem_t *stolac;
sem_t *broj_klijenta;
sem_t *ispis;

/* zajednicke varijable */
typedef struct {
	bool zatvori;
	int br_preostalih_mjesta;
	int klijent;
} spremnik;

spremnik* buf;

/* postavljanje zajedničke memorije */
void inicijaliziraj() {
	int ID = shmget (IPC_PRIVATE, sizeof(sem_t), 0600);
	otvoreno = (sem_t*)shmat (ID, NULL, 0);
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)
	sem_init(otvoreno, 1, 1); //početna vrijednost = 1, 1=>za procese

	ID = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
	br_mjesta = (sem_t*)shmat(ID, NULL, 0);
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)
	sem_init(br_mjesta, 1, 1); //početna vrijednost = 1, 1=>za procese
	
	ID = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
	cekam_klijenta = (sem_t*)shmat(ID, NULL, 0);
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)
	sem_init(cekam_klijenta, 1, 0); //početna vrijednost = 0, 1=>za procese
	
	ID = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
	stolac = (sem_t*)shmat(ID, NULL, 0);
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)
	sem_init(stolac, 1, 0); //početna vrijednost = 0, 1=>za procese
	
	ID = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
	ispis = (sem_t*)shmat(ID, NULL, 0);
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)
	sem_init(ispis, 1, 0); //početna vrijednost = 0, 1=>za procese
	
	ID = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
	broj_klijenta = (sem_t*)shmat(ID, NULL, 0);
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)
	sem_init(broj_klijenta, 1, 0); //početna vrijednost = 0, 1=>za procese

	ID = shmget(IPC_PRIVATE, sizeof(spremnik), 0600);
	buf = (spremnik*)shmat(ID, NULL, 0);
	buf->zatvori = 0;
	buf->br_preostalih_mjesta = N;
	shmctl(ID, IPC_RMID, NULL); //moze odmah ovdje, nakon shmat, ili na kraju nakon shmdt jer IPC_RMID oznacava da segment treba izbrisati nakon sto se zadnji proces odijeli od tog segmenta (detach)	
	
}

void frizerka() {
	printf("Frizerka: Otvaram salon\n");
	printf("Frizerka: Postavljam znak OTVORENO\n");
	
	while(1) {
		sem_wait(otvoreno);
		if (buf->zatvori) {
			printf("Frizerka: Postavljam znak ZATVORENO\n");
		}
		sem_post(otvoreno);
		sem_wait(br_mjesta);
		if (buf->br_preostalih_mjesta < N) { // ima klijenata u cekaonici
			sem_post(br_mjesta);
			sem_post(stolac); // pustamo jednog klijenta iz cekaonicu na stolac
			sem_wait(broj_klijenta);
			printf("Frizerka: Idem raditi na klijentu %i\n", buf->klijent); 
			sem_post(ispis);
			sleep(3); // radimo na frizuri... npr 3 sekunde
			printf("Frizerka: Klijent %i gotov\n", buf->klijent);
		} else if (!buf->zatvori) {
			sem_post(br_mjesta);			
			printf("Frizerka: Spavam dok klijenti ne dođu\n");
			int temp = 1;
			while (temp > 0) {
				sem_getvalue(cekam_klijenta, &temp);
				sem_wait(cekam_klijenta);
			}
		} else { // kraj radnog vremena i salon je prazan
			sem_post(br_mjesta);
			printf("Frizerka: Zatvaram salon\n");
			break;
		}
	}	
}

void klijent(int i) {
	printf("	Klijent(%i): Želim na frizuru\n", i);
	sem_wait(otvoreno);
	sem_wait(br_mjesta);
	
	if (!buf->zatvori && buf->br_preostalih_mjesta > 0) { // ako nije zatvoreno i ima mjesta u cekaonici
		sem_post(otvoreno);
		printf("	Klijent(%i): Ulazim u čekaonicu (%d)\n", i, N - buf->br_preostalih_mjesta + 1);
		buf->br_preostalih_mjesta--; // ulazimo u cekaonicu i smanjujemo broj preostalih mjesta
		sem_post(br_mjesta);
		sem_post(cekam_klijenta); // budimo frizerku
		sem_wait(stolac); // cekamo red da sjednemo za stolac
		buf->klijent = i;
		sem_post(broj_klijenta);
		sem_wait(ispis);
		sem_wait(br_mjesta);
		buf->br_preostalih_mjesta++; // frizerka nam radi frizuru tako da povecavamo broj mjesta u cekaonici
		sem_post(br_mjesta);
		printf("	Klijent(%i): Frizerka mi radi frizuru\n", i);
	} else {
		printf("	Klijent(%i): Nema mjesta u čekaoni, vratit ću se sutra\n", i);
		sem_post(otvoreno);
		sem_post(br_mjesta);
	}
}

int main() {
	inicijaliziraj();
	
	// stvaranje procesa frizerka
	switch(fork()) {
		case 0:
			frizerka();
			exit(0);
	}
	
	for (int i = 1; i <= 5; i++) {
		switch(fork()) {
			case 0: 
				klijent(i);
				exit(0);
		}
	}
	
	sleep(15);
	for (int i = 6; i <= 7; i++) {
		switch(fork()) {
			case 0: 
				klijent(i);
				exit(0);
		}
	
	}
		
	sleep(20); // simuliranje radnog vremena
	sem_post(cekam_klijenta); // budimo frizerku ako je potrebno da bi mogla zatvoriti
	sem_wait(otvoreno);
	buf->zatvori = 1;
	sem_post(otvoreno);
	for (int i = 0; i < 8; i++) {
		wait(NULL); 
	}
	
	sem_destroy(otvoreno);
	sem_destroy(br_mjesta);
	shmdt(br_mjesta);
	shmdt((spremnik*)buf);
	shmdt(otvoreno);
	sem_destroy(cekam_klijenta);
	shmdt(cekam_klijenta);
	sem_destroy(stolac);
	shmdt(stolac);
	sem_destroy(broj_klijenta);
	shmdt(broj_klijenta);
	sem_destroy(ispis);
	shmdt(ispis);
}
