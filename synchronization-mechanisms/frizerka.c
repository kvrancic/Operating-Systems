#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>

int N; // kapacitet cekaonice
int M; // broj klijenata koji ce se generirati

// kako ne bi trebali koristiti pointer aritmetiku, najlakse je sve sto je dijeljeno pohraniti u strukturu kako bi mogao dodavati jos varijabli kasnije 
typedef struct{
    sem_t* frizerka; 
    sem_t* klijent; 
    sem_t* mutex; //medusobno iskljucivanje - osigurava da samo jedan proces istovremeno pristupa zajednickim resursima, zakljucan = 0, otkljucan = 1
    int curr_capacity; //trenutni broj klijenata u cekaonici
    int next_client; //indeks sljedeceg po redu 
    int otvoreno; 
    int unsuccessful[10]; //OPREZ - RUCNO DODANA VELICINA DA NE KOMPLICIRAM, trebalo bi ici M ali u structu mora biti unaprijed definirana velicina
    int unsuccessful_index; 
} cloud;

cloud* shared;

void frizerka(); 
void klijent(int id);

int main(){
    // MAC SPECIFIC NAREDBE, DA SE MAKNE IZ MEMORIJE AKO JE SLUCAJNO OSTAO U PROSLIM RUNOVIMA
    sem_unlink("/frizerka_sem");
    sem_unlink("/klijent_sem");
    sem_unlink("/mutex_sem");
    // stvori zajednički prostor
    int ID = shmget(IPC_PRIVATE, sizeof(cloud), 0600);
    //poveži se na "oblak" 
    shared = (cloud*)shmat(ID, NULL, 0); 

    //inicijaliziraj semafore
    shared->frizerka = sem_open("/frizerka_sem", O_CREAT | O_EXCL, 0600, 0);
    shared->klijent = sem_open("/klijent_sem", O_CREAT | O_EXCL, 0600, 0);
    shared->mutex = sem_open("/mutex_sem", O_CREAT | O_EXCL, 0600, 1);

    //inicijaliziraj varijable 
    shared->curr_capacity = 0; 
    shared->next_client = 1; 
    shared->otvoreno = 1; 
    shared->unsuccessful_index = 0; 
    for(int i = 0; i < M; i++){
        shared->unsuccessful[i] = 0; 
    }

    printf("Koliki je kapacitet cekaonice? "); 
    scanf("%d", &N);
    printf("Koliko klijenata zelis generirati? "); 
    scanf("%d", &M);
    printf("Krecemo s radom...\n\n");
    printf("Frizerka: Otvaram salon\n");

    //pokreni frizerku 
    if(fork() == 0){
        frizerka();
        exit(0);
    }

    srand(time(NULL));

    //pokreni klijente
    for(int i = 0; i < M; i++){
        if(fork() == 0){
            klijent(i+1);
            exit(0);
        }
        sleep(1 + (rand() % 2)); 
    }
    //neka radno vrijeme bude M + 14 sekundi pošto će maksimalno nakon prestanka ulazaka klijenata raditi još 12 sek
    sleep(14);

    sem_wait(shared->mutex);
    shared->otvoreno = 0; 
    sem_post(shared->klijent); //odblokiraj da bi se zavrsio proces
    sem_post(shared->mutex);

    //čekaj kraj procesa 
    for(int i = 0; i < M + 1; i++){
        wait(NULL);
    }

    printf("Frizerka: Zatvaram salon\n");

    //unisti semafore 
    sem_close(shared->frizerka);
    sem_close(shared->klijent);
    sem_close(shared->mutex);
    sem_unlink("/frizerka_sem");
    sem_unlink("/klijent_sem");
    sem_unlink("/mutex_sem");

    //oslobodi memoriju 
    shmdt(shared); 
    shmctl(ID, IPC_RMID, NULL);

    return 0;

}

void frizerka(){
    if(shared -> otvoreno == 1) printf("Frizerka: Postavljam znak OTVORENO\n");

    while(1){
        // spavanje 
        if(shared->curr_capacity == 0){
            printf("Frizerka: Spavam dok klijenti ne uđu\n"); 
        }
        // cekaj pojavu klijenta i dozvolu za ulazak u ko
        sem_wait(shared->klijent); // na ovom se zablokira na kraju, pa ce posluzit da ga na kraju podignemo da zavrsimo sve
        // je li gotovo 
        if(shared -> otvoreno == 0) {
            printf("Frizerka: Postavljam znak ZATVORENO\n"); 
            return; 
        }
        sem_wait(shared->mutex);
        
        // zovi sljedeceg klijenta 
        //pomakni indekse po potrebi 
        for(int i = 0; i <= shared->unsuccessful_index; i++){
            if(shared->unsuccessful[i] == shared->next_client){ // ako je klijent koji bi trebao biti sljedeci otisao
                shared->next_client++; //pomakni sve dalje za 1
            } 
        }
        int klijent = shared->next_client; 
        shared->next_client = shared->next_client + 1; 
        // oslobodi mjesto u cekaonici 
        shared->curr_capacity = shared->curr_capacity - 1; 
        // izadi iz ko 
        sem_post(shared->mutex); 

        // odradi posao 
        printf("Frizerka: Idem raditi na klijentu %d\n", klijent);
        sleep(2 + (rand() % 3));
        printf("Frizerka: Klijent %d gotov\n", klijent); 

        //frizerka opet spremna za rad
        sem_post(shared->frizerka);
    }


}

void klijent(int id){
    if(shared->otvoreno == 0){ // s obzirom kakav je main uvijek ce se izgenerirati svi, ali nema veze
        return; 
    }
    printf("\tKlijent(%d): Želim na frizuru\n", id);
    // pricekaj mogucnost ulaska u ko 
    sem_wait(shared->mutex); 
    if(shared->curr_capacity < N){
        shared->curr_capacity++; 
        printf("\tKlijent(%d): Ulazim u čekaonicu (%d)\n", id, shared->curr_capacity); 
        // obnovi semafor klijenata i izadi iz ko 
        sem_post(shared->klijent); 
        sem_post(shared->mutex);
        //cekaj poziv na sisanje
        sem_wait(shared->frizerka); 
    }
    else {
        //cekaonica puna, klijent odlazi 
        printf("\tKlijent(%d): Odlazim, vratit ću se sutra\n", id);
        // dodaj u listu 
        shared->unsuccessful[shared->unsuccessful_index] = id;
        shared->unsuccessful_index++;
        sem_post(shared->mutex);
    }
}


