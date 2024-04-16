#include <stdio.h>
#include <pthread.h> 
#include <unistd.h> 
#include <string.h> 
#include <time.h> 
#include <stdlib.h>
#include <errno.h>
#define BROJ_LJUDI 20

#define string char*

//monitor 
pthread_mutex_t m; 
pthread_cond_t red[2]; //red cekanja na svakoj od obala 
pthread_cond_t camacSpremnost; 

int rbrK = 0; 
int rbrM = 0; 
int brdr = 0; // samo broj dretvi misionara i kanibala 
int gotovo = 0; 
int strana = 0; // 0 desna, 1 lijeva
int uCamcu = 0; // broj ljudi u camcu 
int KuCamcu = 0; // broj kanibala u camcu 
int MuCamcu = 0; // broj misionara u camcu
char putnici[7][4]; 
char cekanjeDesno[BROJ_LJUDI][4]; // Zasto 4? pocetno slovo (1) + broj znamenki (1 + (floor(BROJ_LJUDI/10) = 2) + null-terminator (1) 
char cekanjeLijevo[BROJ_LJUDI][4];
int rbrCekanjeL = 0; 
int rbrCekanjeR = 0; 
int plovidba = 0; // indicira vozi li brod trenutno, 0 - ne vozi, 1 - vozi

void print_format() {
    if(!strana){
        printf("C[D]={");
    }
    else {
        printf("C[L]={");
    }
    for(int i = 0; i < uCamcu; i++){
        printf("%s", putnici[i]);
        if(i != uCamcu - 1){
            printf(" ");
        }
    }
    printf("} ");

    printf("LO ={");
    for(int i = 0; i < rbrCekanjeL; i++) {
        printf("%s", cekanjeLijevo[i]);
        if(i != rbrCekanjeL - 1){
            printf(" ");
        }
    }
    printf("}");
    printf(" DO = {");
    for(int i = 0; i < rbrCekanjeR; i++) {
        printf("%s", cekanjeDesno[i]);
        if(i != rbrCekanjeR - 1){
            printf(" ");
        }
    }
    printf("}\n");
}

void ukloniCekanje(char array[][4], string id, int size){
    int i, j;

    // Pronalazak indeksa stringa s u polju
    for (i = 0; i < size; i++) {
        if (strcmp(array[i], id) == 0) {
            // Pomak svih elemenata iza stringa s ulijevo
            for (j = i; j < size - 1; j++) {
                strcpy(array[j], array[j + 1]);
            }
            break;
        }
    }
}

void sleepMilliseconds(unsigned int milliseconds) {
    struct timespec req, rem;

    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000;

    while (nanosleep(&req, &rem) == -1 && errno == EINTR) {
        req = rem;
    }
}

void *kanibal(){
    int stranaK = rand() & 1; // 0 - desna, 1 - lijeva
    pthread_mutex_lock(&m); // moramo zakljucati kad manipuliramo dijeljenim varijablama
    ++rbrK; 
    char id[4];
    sprintf(id, "K%d", rbrK);
    //printf("Usao u KO %s\n", id);
    if(!stranaK){
        printf("%s: došao na desnu obalu \n", id);
        // njegov id moramo appendati u red cekanja na desnoj strani
        // koristimo strcpy kako bi predali vrijednost, a ne adresu 
        strcpy(cekanjeDesno[rbrCekanjeR], id); 
        rbrCekanjeR++;
    }
    else{
        printf("%s: došao na lijevu obalu \n", id);
        // njegov id moramo appendati u red cekanja na lijevoj strani
        // koristimo strcpy kako bi predali vrijednost, a ne adresu 
        strcpy(cekanjeLijevo[rbrCekanjeL], id); 
        rbrCekanjeL++;
    }
    print_format();
    pthread_cond_signal(&camacSpremnost); 
    while(plovidba == 1 || strana != stranaK || (uCamcu + 1) > 7 || ((KuCamcu + 1) > MuCamcu && MuCamcu != 0)){
        // provjera moze li u camac
        //printf("Izasao iz KO %s\n", id);
        pthread_cond_wait(&red[stranaK], &m); 
        //printf("Usao u KO %s\n", id);
    }
    // sada je spreman za ulazak u camac
    KuCamcu++; 
    uCamcu++; 
    strcpy(putnici[uCamcu - 1], id); 
    printf("%s: ušao u čamac\n", id); 

    // brisanje iz reda cekanja
    if(!stranaK){
        ukloniCekanje(cekanjeDesno, id, BROJ_LJUDI);
        rbrCekanjeR--; 
    }
    else{
        ukloniCekanje(cekanjeLijevo, id, BROJ_LJUDI);
        rbrCekanjeL--; 
    }
    print_format();
    // dojavi da je doslo do promjene onima u redu cekanja 
    pthread_cond_signal(&camacSpremnost); // vidi je li camac spreman za polazak
    //sleepMilliseconds(50); 
    pthread_cond_broadcast(&red[stranaK]); // mozda sada netko u redu moze uci 
    // otkljucaj monitor 
    pthread_mutex_unlock(&m);
    //printf("Izasao iz KO %s\n", id);
    brdr--;
    //printf("%s gotov\n", id);
    return NULL;
}

void *misionar(){
    int stranaM = rand() & 1; // 0 - desna, 1 - lijeva
    pthread_mutex_lock(&m); // moramo zakljucati kad manipuliramo dijeljenim varijablama
    ++rbrM;
    char id[4];
    sprintf(id, "M%d", rbrM);
    //printf("Usao u KO %s\n", id);
    if(!stranaM){
        printf("%s: došao na desnu obalu \n", id);
        // njegov id moramo appendati u red cekanja na desnoj strani
        // koristimo strcpy kako bi predali vrijednost, a ne adresu 
        strcpy(cekanjeDesno[rbrCekanjeR], id); 
        rbrCekanjeR++;
    }
    else{
        printf("%s: došao na lijevu obalu \n", id);
        // njegov id moramo appendati u red cekanja na lijevoj strani
        // koristimo strcpy kako bi predali vrijednost, a ne adresu 
        strcpy(cekanjeLijevo[rbrCekanjeL], id); 
        rbrCekanjeL++;
    }
    print_format();
    pthread_cond_signal(&camacSpremnost);  // novo
    while(plovidba == 1 || strana != stranaM || ((uCamcu + 1) > 7) || (KuCamcu > (MuCamcu + 1))){
        // provjera moze li u camac
        //printf("Izasao iz KO %s\n", id);
        pthread_cond_wait(&red[stranaM], &m); 
        //printf("Usao u KO %s\n", id);
    } 
    // sada je spreman za ulazak u camac
    MuCamcu++; 
    uCamcu++; 
    strcpy(putnici[uCamcu - 1], id); 
    printf("%s: ušao u čamac\n", id); 

    // brisanje iz reda cekanja
    if(!stranaM){
        ukloniCekanje(cekanjeDesno, id, BROJ_LJUDI);
        rbrCekanjeR--; 
    }
    else{
        ukloniCekanje(cekanjeLijevo, id, BROJ_LJUDI);
        rbrCekanjeL--; 
    }
    print_format();
    // dojavi da je doslo do promjene onima u redu cekanja 
    pthread_cond_signal(&camacSpremnost); // vidi je li camac spreman za polazak
    //sleepMilliseconds(50); 
    pthread_cond_broadcast(&red[stranaM]); // mozda sada netko u redu moze uci 
    // otkljucaj monitor 
    pthread_mutex_unlock(&m);
    //printf("Izasao iz KO %s\n", id);
    brdr--;
    //printf("%s gotov\n", id);
    return NULL;
}

void *camac(){  // jedan prolaz kroz fju = jedna voznja camca 
    // dretva koja konstantno radi 
    while(!gotovo){
        // u startu camac uvijek prazan jer ili pocinje ili je upravo iskrcao putnike 
        if(!strana){
            printf("C: prazan na D obali.\n");
        } else {
            printf("C: prazan na L obali.\n");
        }
        print_format(); 

        pthread_mutex_lock(&m); // zakljucaj, ukrcaj putnika 
        //printf("Usao u KO camac\n");
        //javi svima na strani na kojoj je dosao da je stigao 
        pthread_cond_broadcast(&red[strana]);

        //printf("Izasao iz KO camac\n");
        //pthread_cond_wait(&camacSpremnost, &m);
        //pthread_cond_wait(&camacSpremnost, &m);
        //pthread_cond_wait(&camacSpremnost, &m);
        while(uCamcu < 3){
            pthread_cond_wait(&camacSpremnost, &m); // bitno da je ovako a ne 3 zaredom jer nekad saljem signal i ako nije nitko usao 
        }
        
        //printf("Usao u KO camac\n");
        printf("C: dovoljno putnika ukrcano, polazim za jednu sekundu\n");
        pthread_mutex_unlock(&m); // otkljucaj da netko stigne uletiti
        //printf("Izasao iz KO camac\n");

        sleep(1); 

        // ponovno zakljucaj - priprema za voznju
        pthread_mutex_lock(&m);
        //printf("Usao u KO camac\n");
        if(!strana){
            printf("C: prevozim s desne na lijevu obalu: ");
        } else {
            printf("C: prevozim s lijeve na desnu obalu: ");
        }

        for(int i = 0; i < uCamcu; i++){
            printf("%s", putnici[i]); 
            if(i != uCamcu - 1){
                printf(", ");
            }
        }
        printf("\n");
        printf("U camcu bilo ljudi %d \n", uCamcu);

        // polazak - otkljucaj jer tada se moze dolaziti 
        plovidba = 1; // bitno da onemogucimo ukrcavanje 
        pthread_mutex_unlock(&m);
        //printf("Izasao iz KO camac\n");
        sleep(2);

        // stigli smo - zakljucavamo sve za vrijeme iskrcavanja da nitko nebi usao - NISAM SIGURAN JE LI POTREBNO 
        //printf("Usao u KO camac\n");
        pthread_mutex_lock(&m);
        if(!strana){
            printf("C: preveo s desne na lijevu obalu: ");
        } else {
            printf("C: preveo s lijeve na desnu obalu: ");
        }
        for(int i = 0; i < uCamcu; i++){
            printf("%s", putnici[i]); 
            if(i != uCamcu - 1){
                printf(", ");
            }
        }
        printf("\n\n");

        // kraj voznje
        memset(putnici, '\0', sizeof(putnici)); 
        plovidba = 0; 
        uCamcu = 0; 
        KuCamcu = 0;
        MuCamcu = 0;
        strana = (++strana) % 2;
        pthread_mutex_unlock(&m); // ako maknes gornji lock makni i ovaj unlock 
        //printf("Izasao iz KO camac\n");
    }
    return NULL;
}

void *pomocna_dretva(){
    int pom = 0; 
    pthread_t thr_id; // ne treba nam handle za dretve pa dovoljno samo ovo slati za argument
    pthread_attr_t attr; 

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    // kad dretva zavrsi s radom obrisi sve njene resurse, da je ne moramo cekati

    while(rbrK + rbrM < BROJ_LJUDI){ // NE ZABORAVI UPDATEATI U FUNKCIJAMA GORE 
        if(!(pom % 2)){
            pthread_create(&thr_id, &attr, kanibal, NULL);
            brdr = brdr + 1; 
        } else {
            pthread_create(&thr_id, &attr, kanibal, NULL);
            pthread_create(&thr_id, &attr, misionar, NULL);
            brdr = brdr + 2; 
        }
        sleep(1);
        pom++;
    }
    return NULL;
}

int main(){
    printf("Legenda: M-misionar, K-kanibal, C-čamac\nLO-lijeva obala, DO-desna obala\nL-lijevo, D-desno\n\n");
    // copy paste iz video upute za labos
    srand(time(NULL));

    pthread_mutex_init (&m, NULL); // m pokazuje na varijablu zaključavanja, attr definira karakteristike stvorene varijable - NULL je default
    pthread_cond_init (&red[0], NULL); 
    pthread_cond_init (&red[1], NULL); 
    
    pthread_t thr_camac;
    pthread_t thr_pomocna;
    
    pthread_create(&thr_camac, NULL, camac, NULL); 

    pthread_create(&thr_pomocna, NULL, pomocna_dretva, NULL); 

    pthread_join(thr_pomocna, NULL);

    sleep(2);
   
    while(brdr > 0){ // NE ZABORAVI UPDATEATI brdr U FUNKCIJAMA GORE 
        sleep(1);
    } // ovako cekamo zavrsetak svih dretvi, nema potrebe za joinom jer su tako postavljene

    gotovo = 1; 

    pthread_join(thr_camac, NULL); 
    printf("Svi su se putnici prevezli\n"); 

    pthread_mutex_destroy(&m);
    
    pthread_cond_destroy(&camacSpremnost);
    pthread_cond_destroy(&red[0]);
    pthread_cond_destroy(&red[1]);

    return 0;
}




