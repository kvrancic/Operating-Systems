// Napisati program koji stvara N dretvi. Svaka dretva povećava zajedničku varijablu A za 1 u petlji M  puta. Parametre N i M zadati kao argument iz komandne linije. Sinkronizirati dretve Lamportovim algoritmom. 

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#define _POSIX_C_SOURCE 200809L 

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>

int n; //broj dretvi
int m; //broj iteracija
int a = 0; //rezultat 
atomic_int *broj; //broj ticketa dok se ceka u redu, velicina n
atomic_int *ulaz; // zastavica koja govori je li i-ta dretva u fazi dodjele broja, velicina n

int find_max(atomic_int *broj){
    int max = broj[0]; 
    for(int i = 1; i < n; i++){
        if(broj[i] > max) max = broj[i];
    }
    return max;
}

void ulazKO(int i){
    ulaz[i] = 1; // blokiraj ko
    broj[i] = find_max(broj) + 1; // pridijeli sljedeći broj u waitlisti
    ulaz[i] = 0; // odblokiraj ko 

    for(int j = 0; j < n; j++){
        while(ulaz[j] != 0); 
        while(broj[j] != 0 && ( //dok postoji neobradeni broj na waitlisti...
                broj[j] < broj[i] || ( // ... koji je manji od izdanog ili ...
                    broj[j] == broj[i] && j < i // ... jednak uz uvjet manjeg indeksa
                    )
                )
             );
    }
}

void izlazKO(int i){
    broj[i] = 0; // oznaci kao obraden
}

void *dretva (void *rbr){
    int *param = (int *)rbr;
    for(int i = 0; i < m; i++){
        ulazKO(*param);
        a++; 
        izlazKO(*param);
    }
    return NULL;
}

int main(int argc, char *argv[]){

    if(argc < 3){
        printf("Neispravan unos argumenata u pozivu programa"); 
        return 1; 
    }

    n = atoi(argv[1]);
    m = atoi(argv[2]); 

    // n intova za ulaze + n intova za brojeve + n dretvi + n intova za BR
    int *memorija = malloc(sizeof(atomic_int) * 2 * n + sizeof(pthread_t) * n + sizeof(int) * n);
    ulaz = memorija; 
    broj = ulaz + n; 
    pthread_t *t = (pthread_t *) (broj + n); 
    int *BR = (int *) (t + n);

    //kopija koda iz prezentacije 
    for(int i = 0; i < n; i++) {
        BR[i] = i;
        if(pthread_create(&t[i], NULL, dretva, (void*)&BR[i])) {
            printf("Ne mogu stvoriti novu dretvu!\n"); 
            exit(1); 
        }
    }
    for(int i = 0; i < n; i++) pthread_join(t[i],NULL);

    free(memorija); 
    printf("Konacna vrijednost zajednicke varijable A je %d\n", a); 

    return 0;
}