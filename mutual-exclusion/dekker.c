//  Napisati program koji stvara dva procesa. Svaki proces povećava zajedničku varijablu A za 1 u petlji M  puta. Parametar M zadati kao argument iz komandne linije. Sinkronizirati ta dva procesa Dekkerovim algoritmom
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _POSIX_C_SOURCE 200809L  // Required for clock_gettime()
#define BROJ_PROCESA 2

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <sys/wait.h>

atomic_int *zastavica; // 2 elementa: zastavica[i] = 0 -> i moze u ko, = 1 -> ne moze u ko
atomic_int *pravo; // 1 element: govori tko ima pravo na ko
atomic_int *A; // rezultat
atomic_int M, segment_id; 

void ulazKO(int i, int j){
    zastavica[i] = 1; 
    while(zastavica[j] != 0){
        if(*pravo == j){
            zastavica[i] = 0;
            while(*pravo == j);
            // j vise nema pravo 
            zastavica[i] = 1;
        }
    }
}

void izlazKO(int i, int j){
    *pravo = j; 
    zastavica[i] = 0; 
}

int main(int argc, char *argv[]){
    if (argc < 2) {
        printf("Potrebno je navesti broj ponavljanja petlje kao argument!\n");
        return 1;
    }
    int M = atoi(argv[1]);
    printf("Kontrolni ispis unosa: %d\n", M); 

    // Stvaranje zajedničke memorije za varijable 
    // size = int pravo + 2 int zastavice + int A = 4 * int
    segment_id = shmget(IPC_PRIVATE, sizeof(int) * 4, 0600); // sablona iz video uputa prof Jelenkovica
    if(segment_id == -1){
        fprintf(stderr, "Nema memorije!\n");
        exit(1); 
    }

    pravo = (atomic_int*) shmat(segment_id, NULL, 0); 
    zastavica = pravo + 1; 
    A = pravo + 3; 

    //inicjalizacija vrijednosti
    *pravo = 0; 
    *A = 0; 
    zastavica[0] = 0; 
    zastavica[1] = 0; 

   // Stvaranje procesa
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr, "Ne mogu stvoriti proces!\n");
        return 1;
    } else if (pid == 0) {  // dijete
        for (int i = 0; i < M; i++) {
            ulazKO(0, 1);
            printf("Dijete: A=%d\n", ++(*A));
            izlazKO(0, 1);
            sleep(1);
        }
        exit(0);
    } else {  // roditelj
        for (int i = 0; i < M; i++) {
            ulazKO(1, 0);
            printf("Roditelj: A=%d\n", ++(*A));
            izlazKO(1, 0);
            sleep(1);
        }
    }

    wait(NULL);

    printf("Konacna vrijednost parametra iznosi %d\n", *A); 
    shmdt((atomic_int *) pravo); //ChatGPT kaze da sustav zna sto sve treba pocistiti, da mu samo treba dati pointer na pocetak
    shmctl(segment_id, IPC_RMID, NULL);

    return 0;
}