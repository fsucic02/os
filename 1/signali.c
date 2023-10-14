#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

/* priprema stoga, K_Z, T_P */
int stack[10];
int top = -1;
int data;
int temp;
int kz[3] = {0, 0, 0}; // na pocetku 000
int tp = 0; // na pocetku programa je 0

/* ispis vremena - postavljanje */
struct timespec t0; /* vrijeme pocetka programa */

/* postavlja trenutno vrijeme u t0 */
void postavi_pocetno_vrijeme()
{
	clock_gettime(CLOCK_REALTIME, &t0);
}

/* dohvaca vrijeme proteklo od pokretanja programa */
void vrijeme(void)
{
	struct timespec t;

	clock_gettime(CLOCK_REALTIME, &t);

	t.tv_sec -= t0.tv_sec;
	t.tv_nsec -= t0.tv_nsec;
	if (t.tv_nsec < 0) {
		t.tv_nsec += 1000000000;
		t.tv_sec--;
	}

	printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

/* ispis kao i printf uz dodatak trenutnog vremena na pocetku */
#define PRINTF(format, ...)       \
do {                              \
  vrijeme();                      \
  printf(format, ##__VA_ARGS__);  \
}                                 \
while(0)

/*
 * spava zadani broj sekundi
 * ako se prekine signalom, kasnije nastavlja spavati neprospavano
 */
void spavaj(time_t sekundi)
{
	struct timespec koliko;
	koliko.tv_sec = sekundi;
	koliko.tv_nsec = 0;

	while (nanosleep(&koliko, &koliko) == -1 && errno == EINTR) {}
}

void obradi_sig(int sig);
void printaj_stog();
void inicijaliziraj();
int pop();
int push(int data);
int nije_kraj = 1;
void print();

int main() {
	inicijaliziraj();
	
	PRINTF("Program s PID=%ld krenuo s radom\n", (long) getpid());
	PRINTF("K_Z=000, T_P=0, stog: - \n\n");
	
	while (1) {
		// koristan posao 
		if (kz[2] != 0) obradi_sig(SIGINT);
		if (kz[1] != 0) obradi_sig(SIGUSR1); 
		if (kz[0] != 0) obradi_sig(SIGTERM);
	}
	return 0;
}


void inicijaliziraj() {
	struct sigaction act;
	
	/* maskiranje za sigusr1*/
	
	act.sa_handler = obradi_sig; /* kojom se funkcijom signal obrađuje */
	act.sa_flags = SA_NODEFER;
	sigaction(SIGUSR1, &act, NULL); /* maskiranje signala preko sučelja os-a */
	sigemptyset(&act.sa_mask);
	/* 2. maskiranje signala sigterm */
	act.sa_handler = obradi_sig;
	sigaction(SIGTERM, &act, NULL);
	sigemptyset(&act.sa_mask);
	/* 3. maskiranje signala sigint */
	act.sa_handler = obradi_sig;
	sigaction(SIGINT, &act, NULL);
	sigemptyset(&act.sa_mask);
	postavi_pocetno_vrijeme();
}

/* sigint -> 2, sigterm -> 15, sigusr1 -> 10 */
void obradi_sig(int sig) {
	switch (sig) {
		case SIGINT: // obrada siginta, najveci prioritet
				kz[2] = 1;
				if (tp == 3) {
					PRINTF("SKLOP: Dogodio se prekid razine 3 ali se on pamti i ne prosljeduje procesoru\n");
					print();
					return;
				}
				
				PRINTF("SKLOP: dogodio se prekid razine 3 i prosljeđuje se procesoru\n");
				print();
				
				push(tp);
				PRINTF("Počela obrada prekida razine 3\n");
				kz[2] = 0; tp = 3;
				print();
				
				spavaj(5);
				
				
				PRINTF("Završila obrada prekida razine 3\n");
				tp = pop();
				
				if (tp == 0) PRINTF("Nastavlja se izvođenje glavnog programa\n");
				else PRINTF("Nastavlja se obrada prekida razine %d\n", tp);
				print();
				
				if (kz[2] != 0) obradi_sig(SIGINT);
				if (kz[1] != 0 && kz[2] == 0) obradi_sig(SIGUSR1);
				if (kz[0] != 0 && tp != 2) obradi_sig(SIGTERM);
				break;
				
		case SIGUSR1:
				kz[1] = 1;

				if (tp == 2) {
					PRINTF("SKLOP: Dogodio se prekid razine 2 ali se on pamti i ne prosljeduje procesoru\n");
					print();
					return;
				}
				
				if (tp == 3) {// trenutno se obraduje prekid najvise razine, samo spremimo u kz info da moramo obraditi ovaj prekid
				 	PRINTF("SKLOP: Dogodio se prekid razine 2 ali se on pamti i ne prosljeduje procesoru\n");
				 	print();
				 } else {
				 	PRINTF("SKLOP: Dogodio se prekid razine 2 i prosljeduje se procesoru\n");
					print();
					 
					push(tp);
					PRINTF("Počela obrada prekida razine 2\n");
					kz[1] = 0; tp = 2;
					print();
					
					spavaj(5);
					
					PRINTF("Završila obrada prekida razine 2\n");
					tp = pop();
					
					if (tp == 0) PRINTF("Nastavlja se izvođenje glavnog programa\n");
					else PRINTF("Nastavlja se obrada prekida razine %d\n", tp);
					print();
					
					if (kz[1] != 0) obradi_sig(SIGUSR1);
					if (kz[0] != 0 && tp != 1) obradi_sig(SIGTERM);
					
				 }
				
				break;
				 
		case SIGTERM: kz[0] = 1;
				
				if (tp == 1 && kz[0] == 0) {
					PRINTF("SKLOP: Dogodio se prekid razine 1 ali se on pamti i ne prosljeduje procesoru\n");
					print();
					return;
				}
				
				 if (tp != 0) {
				 	PRINTF("SKLOP: Dogodio se prekid razine 1 ali se on pamti i ne prosljeduje procesoru\n");
				 	print();
				 } else {
				 	PRINTF("SKLOP: Dogodio se prekid razine 1 i prosljeduje se procesoru\n");
					print();
					 
					push(tp);
					PRINTF("Počela obrada prekida razine 1\n");
					kz[0] = 0; tp = 1;
					print();
					
					spavaj(5);
					
					PRINTF("Završila obrada prekida razine 1\n");
					tp = pop();
					
					PRINTF("Nastavlja se izvođenje glavnog programa\n");
					print();
					
					if (kz[0] != 0) obradi_sig(SIGTERM);
					break;
				 }
				 
	}
}

void print() {
	PRINTF("K_Z=%d%d%d, T_P=%d, ", kz[0], kz[1], kz[2], tp);
	printaj_stog();
}

void printaj_stog() {
	printf("stog: "); 
	if (top == -1) printf("-\n"); // ako je prazan
	else if (top == 0) printf("%d, reg[%d]\n", stack[top], stack[top]);
	else {
		// ima barem dva elementa, moram namjestit ispis
		for (int i = 0; i < top; i++) {
			printf("%d, reg[%d]; ", stack[top - i], stack[top - i]);
		}
		printf("%d, reg[%d]\n", stack[0], stack[0]);
	}
	printf("\n");
}

int pop() {
	if (top == -1) return -1;
	else return stack[top--];
}

int push(int data) {
  	stack[++top] = data;
}
