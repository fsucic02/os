#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <string.h>


char ***disk; // simulirani disk koji služi za pohranu sadržaja stranica
char **okvir; // simulirani radni spremnik od M okvira veličine 64 okteta
short **tablica; // tablica prevođenja za svaki od N procesa
char *disk_dijeljeni; // dijeljeni dio diska

int trenutni = 0;
short t = 0;
int N;
int M;
bool print = 0; // za manevriranje printanjem... 
int okvir_dijeljenog = -1;

void inicijaliziraj();
short dohvati_sadrzaj(int p, short x);
char* dohvati_fizicku_adresu(int p, short x);
void zapisi_sadrzaj(int p, short x, short i);
void stavi_poruke();
void procitaj_poruku(int p);

int main(int argc, char *argv[]) {
	srand((unsigned)time(NULL));
    N = atoi(argv[1]);
    M = atoi(argv[2]);

    inicijaliziraj();

    while (1) {
        for (int p = 0; p < N; p++) { // za svaki proces
            short x = 0x01fe; // nasumicna logicka adresa - (short)rand() & 0x3fe / 0x01fe
            
            while (0 <= x && x <= 63) {
           		x = rand() & 0x3fe;	
            }
            
            /* ispis... */
            printf("---------------------------\n");
            printf("proces: %d\n", p);
            printf("\tt: %hd\n", t);
            
        	if (p == 0) { // proizvodac
        		stavi_poruke();
        	} else { // potrosac
				procitaj_poruku(p);        		
        	}
        	
            printf("\tlog. adresa: 0x%04hx\n", x);

			short i = dohvati_sadrzaj(p, x);
			i++;
			zapisi_sadrzaj(p, x, i);
			
			t++;
			
		 	if (t == 31) {
		 		t = 0; // postavljamo t na 0
		 		
		 		for (int i = 0; i < N; i++) {
		 			for (int j = 0; j < 16; j++) {
		 				tablica[i][j] &= 0xffe0; // postavljamo LRU metapodatke na 0
		 			}
		 		}
		 		
		 		tablica[p][x >> 6] &= 0xffe1; // postavljamo LRU metapodatak trenutne stranice na 1
	 		}

			sleep(1);
        }
	}
}

void inicijaliziraj() {
    disk = (char***)calloc(N, sizeof(char**));
    for (int i = 0; i < N; i++) {
        disk[i] = (char**)calloc(16, sizeof(char*));
        for (int j = 0; j < 16; j++) {
            disk[i][j] = (char*)calloc(64, sizeof(char));
        }
    }

    tablica = (short **)calloc(N, sizeof(short*));
    for (int i = 0; i < N; i++) {
        tablica[i] = (short *)calloc(16, sizeof(short));
    }

    okvir = (char **)calloc(M, sizeof(char*));
    for (int i = 0; i < M; i++) {
        okvir[i] = (char *)calloc(64, sizeof(char));
    }
    
    disk_dijeljeni = (char*)calloc(N, sizeof(char));
}

short dohvati_sadrzaj(int p, short x) {
	char *y = dohvati_fizicku_adresu(p, x);
	bool zajednicki = (0 <= x && x <= 63);
	short i;
	if (zajednicki) {
		i = ((*y & 0x00ff) << 8) | (*(y-1) & 0x00ff);
	} else {
		i = *y;
	}
	return i;	
}

void zapisi_sadrzaj(int p, short x, short i) {
	char *y = dohvati_fizicku_adresu(p, x);
	bool zajednicki = (0 <= x && x <= 63); // pisemo u zajednicki spremnik - 4567
	if (zajednicki) {
		*y = i >> 8;
		y--;
	} 
	
	*y = i;
}

char* dohvati_fizicku_adresu(int p, short x) {
	bool zajednicki = (0 <= x && x <= 63); // prva stranica svakog procesa
	
	if (zajednicki) {
		// dohvacamo zajednicki spremnik
		if (okvir_dijeljenog == -1) { // dijeljeni spremnik nije u okviru
			printf("\tpromasaj!\n");
			if (trenutni < M) {
				// smjestamo zajednicki sprmenik u okvir _trenutni_
				printf("\t\tdodijeljen okvir 0x%04x\n", trenutni);
				okvir_dijeljenog = trenutni;
				
				for (int i = 0; i < N; i++) {
					tablica[i][0] = (trenutni << 6) | 32; // postavljamo bit prisutnosti u tablici stranicenja svakog procesa zato sto smo upravo stavili zajednicku stranicu u okvir
				}
				
				for (int i = 0; i < 64; i++) {
					okvir[trenutni][i] = disk_dijeljeni[i]; // s dijeljenog diska uzimamo podatke
				}
				
				trenutni++;

			} else {
				// moramo LRU algoritam koristiti
				int min = 32;
		 		int min_i, min_j;
		 		short adresa;
		 		for (int i = 0; i < N; i++) {
		 			for (int j = 0; j < 16; j++) {
		 				if (((tablica[i][j] >> 5) & 1 == 1) && (tablica[i][j] & 0x1f) < min) { // stranica je prisutna i LRU je najmanji dosad
		 					min_i = i; 
		 					min_j = j;
		 					adresa = (tablica[i][j] & 0xffc0); // adresa okvira
		 					min = tablica[i][j] & 0x1f;
		 					// ovo je j-ta stranica i-tog procesa
		 				}
		 			}
		 		}
		 		
		 		printf("\t\tizbacujem stranicu 0x%04hx iz procesa %d\n", min_j << 6, min_i);
		 		printf("\t\tlru izbacene stranice: 0x%04hx\n", tablica[min_i][min_j] & 0x1f);
		 		printf("\t\tdodijeljen okvir 0x%04hx\n", adresa >> 6);
		 		tablica[min_i][min_j] = tablica[min_i][min_j] & 0xffdf; // brisanje bita prisutnosti
		 		
		 		// moramo okvir pohraniti na disk
		 		for (int i = 0; i < 64; i++) {
		 			disk[min_i][min_j][i] = okvir[adresa >> 6][i];
		 		}
		 		
		 		for (int i = 0; i < N; i++) {
		 			tablica[i][0] = adresa | 32; // u tablicu stranicenja svakog procesa upisujemo u koji okvir ide zajednicki spremnik i postavljamo bit prisutnosti
		 		}
		 		
		 		// prebacujemo s zajednickog dijela diska u okvir
		 		for (int i = 0; i < 64; i++) {
		 			okvir[adresa >> 6][i] = disk_dijeljeni[i];
		 		}
		 		
		 		okvir_dijeljenog = adresa >> 6;
			}	
		}
		
		return &okvir[okvir_dijeljenog][2*p - 1]; // offset je 2p-1 jer "atomicki" citam / pisem

	} else {
		// ne zajednicki
		short redak = x >> 6;
		print ^= 1;

		if (print == 0) {
			// ne trebamo sve printat, samo vratimo fiz adresu, ovo je poziv iz zapisi sadrzaj
			return &okvir[tablica[p][redak] >> 6][x & 0x3f];
		}
		
		if (((tablica[p][redak] >> 5) & 1) == 0) { // adresa x nije prisutna
			printf("\tpromasaj!\n");

		 	if (trenutni < M) {
		 		// spremamo u okvir _trenutni_
		 		printf("\t\tdodijeljen okvir 0x%04x\n", trenutni);
		 		
		 		tablica[p][redak] = (trenutni << 6) | 32 | t; // u tablicu zapisujemo u kojem je okviru stranica, postavljamo bit prisutnosti i LRU metapodatak postavljamo na t
				
				for (int i = 0; i < 64; i++) {
					okvir[trenutni][i] = disk[p][redak][i]; // spremanje s diska u okvir
				}     		
				trenutni++;
		 	} else {
		 		// svi okviri su zauzeti, prvo provjeramo je li zajednicki spremnik u okviru
		 		
		 		if (okvir_dijeljenog != -1) {
		 			// prisutan je, njega izbacujemo
		 			printf("\t\tizbacujem okvir dijeljenog spremnika\n");
		 			printf("\t\tdodijeljen okvir 0x%04hx\n", okvir_dijeljenog);
		 			
		 			for (int i = 0; i < 64; i++) {
		 				disk_dijeljeni[i] = okvir[okvir_dijeljenog][i];
		 				okvir[okvir_dijeljenog][i] = disk[p][redak][i];
		 			}
		 			
		 			for (int i = 0; i < N; i++) {
		 				tablica[i][0] &= 0xffdf; // brisanje bita prisutnosti za svaki proces
		 			}
		 			
					int temp = okvir_dijeljenog;
					okvir_dijeljenog = -1; // izbacili smo ga
					tablica[p][redak] = (temp << 6) | 32 | t; // u tablicu upisujemo u koji okvir stavljamo stranicu, postavljamo bit prisutnosti i upisujemo LRU metapodatak
 		 		} else {
 		 			// zajednicki spremnik nije u okviru, koristimo LRU algoritam
 		 			int min = 32;
			 		int min_i, min_j;
			 		short adresa;
			 		for (int i = 0; i < N; i++) {
			 			for (int j = 0; j < 16; j++) {
			 				if (((tablica[i][j] >> 5) & 1 == 1) && (tablica[i][j] & 0x1f) < min) { // stranica je prisutna i LRU je najmanji dosad
			 					min_i = i; 
			 					min_j = j;
			 					adresa = (tablica[i][j] & 0xffc0); // adresa okvira
			 					min = tablica[i][j] & 0x1f;
			 					// ovo je j-ta stranica i-tog procesa
			 				}
			 			}
			 		}
			 		
			 		printf("\t\tizbacujem stranicu 0x%04hx iz procesa %d\n", min_j << 6, min_i);
			 		printf("\t\tlru izbacene stranice: 0x%04hx\n", tablica[min_i][min_j] & 0x1f);
			 		printf("\t\tdodijeljen okvir 0x%04hx\n", adresa >> 6);
			 		tablica[min_i][min_j] = tablica[min_i][min_j] & 0xffdf; // brisanje bita prisutnosti
			 		
			 		// moramo okvir pohraniti na disk
			 		for (int i = 0; i < 64; i++) {
			 			disk[min_i][min_j][i] = okvir[adresa >> 6][i];
			 		}
			 		
			 		tablica[p][redak] = adresa | 32 | t; // u tablicu upisujemo u koji okvir stavljamo stranicu, postavljamo bit prisutnosti i upisujemo LRU metapodatak
			 		
			 		// prebacujemo s diska u okvir
			 		for (int i = 0; i < 64; i++) {
			 			okvir[adresa >> 6][i] = disk[p][redak][i];
			 		}
 		 		}
			}
		}
		
		printf("\tfizicka adresa: 0x%04hx\n", (tablica[p][redak] & 0xffc0) | (x & 0x3f));
		printf("\tzapis tablice: 0x%04hx\n", tablica[p][redak]);
		printf("\tsadrzaj adrese: %hx\n\n", okvir[tablica[p][redak] >> 6][x & 0x3f]); 
		return &okvir[tablica[p][redak] >> 6][x & 0x3f];
	}
}

void stavi_poruke() {
	short poruka; // 4567
	for (int i = 0; i < N - 1; i++) {
		poruka = rand();
		zapisi_sadrzaj(i + 1, 0, poruka);
		printf("\tposlao poruku (%d): 0x%04hx\n\n", i+1, poruka);
	}
}

void procitaj_poruku(int p) {
	short poruka = dohvati_sadrzaj(p, 0); // pristupamo zajednickom spremniku
	
	printf("\tprimio poruku: 0x%04hx\n\n", poruka);
}
