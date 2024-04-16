#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _GNU_SOURCE
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <signal.h>
#include <errno.h>

// u K_Z upisujemo +1 u indeks i ako je signal razine i+1 na "waitlistu" i nije jos niti poceo s izvodenjem
// ovdje postoji odredena dilema -> je li svaki element K_Z fiksiran na 0/1 ili moze imati i vecu vrijednost, jer u tom slucaju bi mogli staviti vise istih signala u waitlist zaredom, ja sam ga tretirao kao bool (moze samo biti 0/1)
int K_Z[3] = {0, 0, 0};
// T_P ima vrijednost i za i-tu razinu tekuceg prioriteta (gl program 0) 
int T_P = 0;
// stog je zapravo troclano polje stanja koja cekaju da se vrate u fazu prioriteta 
// stog[visi_novi_signal - 1] = stari_nizi_signal 
// ako je 1 aktivan i prekine ga 2 -> stog[2 - 1] = 1
int stog[3] = {-1, -1, -1}; 

// deklaracija funkcija za obradu signala 
void dogadaj(int kod); 
void obradi(int razina); // ako je doslo iz dogadaja
void nastavak_stare_obrade(int razina); // ako postoji nesto u stogu 
void ispis(); 

#pragma region vrijeme_sablona 
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

#define PRINTF(format, ...)       \
do {                              \
  vrijeme();                      \
  printf(format, ##__VA_ARGS__);  \
}                                 \
while(0)
#pragma endregion vrijeme_sablona

int main()
{
	struct sigaction act;
	act.sa_handler = dogadaj;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	sigaction(SIGUSR1, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	PRINTF("Program s PID=%ld krenuo s radom\n", (long) getpid());
    ispis();
	while(true) {
		sleep(1);
	}
	return 0;
}

void ispis(){
    PRINTF("K_Z=%d%d%d, T_P=%d, stog: ", K_Z[0], K_Z[1], K_Z[2], T_P);
    bool prazan_stog = true; 
    for(int i = 2; i >= 0; i--){
        if(stog[i] != -1){
            prazan_stog = false; 
            printf("%d, reg[%d] ", stog[i], stog[i]);
        }
    }
    if(prazan_stog) {
        printf("-"); 
    }
    printf("\n");
    printf("\n");
}

void dogadaj(int kod){
    int razina = 0;
    switch(kod){
        case SIGTERM:
            razina = 1;
            break; 
        case SIGUSR1:
            razina = 2;
            break; 
        case SIGINT:
            razina = 3; 
            break; 
    }
    K_Z[razina - 1] = 1;  // ne zaboravi obrisati u trenutku pocetka obrade
    if(razina > T_P){
        PRINTF("SKLOP: Dogodio se prekid razine %d i prosljeđuje se procesoru\n", razina);
        ispis(); 
        // sigurni smo da u K_Z ne ceka nista prioritetnije od T_P pa mozemo pisati sljedece
        stog[razina - 1] = T_P; // spremi staro stanje
        T_P = razina; 
        K_Z[razina - 1] = 0;
        obradi(razina);
    }
    else {
        PRINTF("SKLOP: Dogodio se prekid razine %d ali se on pamti i ne prosljeđuje procesoru\n", razina);
		ispis();
    }
}

void obradi(int razina){
    PRINTF("Počela obrada razine %d\n", razina);
    ispis();
    for(int i = 0; i < 15; i++){
        sleep(1);
    }
    PRINTF("Završila obrada prekida razine %d\n", razina);	
    nastavak_stare_obrade(razina);
}

void nastavak_stare_obrade(int razina){
    T_P = stog[razina - 1]; // vracamo se na ono sto smo krenuli raditi
    if(stog[razina - 1] >= 1 && stog[razina - 1] <= 3){ // ako smo dosli iz nekog drugog prekida
        PRINTF("Nastavlja se obrada prekida razine %d\n", T_P);
        stog[razina - 1] = -1; 
        ispis();
    }
    else{ // ako smo dosli iz glavnog
        PRINTF("Nastavlja se izvođenje glavnog programa\n");
        stog[razina - 1] = -1;
        ispis();
        for(int i = 2; i >= 0; i--){
            if(K_Z[i] == 1){
                PRINTF("SKLOP: promijenio se T_P, prosljeđuje prekid razine %d procesoru\n", i + 1);
                stog[i] = T_P; //ovo ce uvijek biti 0 jer idemo iz glavnog
                T_P = i + 1; 
                K_Z[i] = 0; 
                obradi(i + 1);
                break;
            }
        }
    }
}