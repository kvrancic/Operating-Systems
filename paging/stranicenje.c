#include <stdio.h>
#include <time.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include<string.h>

#define N 3 // broj procesa
#define M 4 // broj okvira
#define PAGE_SIZE 64
#define FRAME_SIZE 64
#define OFFSET_MASK 0x03f
#define PRESENT_MASK 0x20
#define LRU_MASK 0x1f
#define INDEX_MASK 0xffc0
#define LRU_REMOVE_MASK 0xffe0

char disk[N][PAGE_SIZE]; // n procesa, indeksi 0-15, velicina stranice/okvira 64
short int tablice_prevodenja[N][16]; // Tablica prevođenja za svaki od N procesa a indeksi idu od 0-15 jer u logadr ima 4 bita
char radni_spremnik[M][FRAME_SIZE]; // M okvira veličine 64 okteta
int zauzeti_okviri[M];
int t = 0;
int popunjenost = 0; // predstavlja poziciju u radnom spremniku
// radi samo u idealiziranom slucaju gdje proces nikad nece sam otici iz radnog spremnika 
// tj samo pod uvjetom da ce se linearno popunjavati i kad se jednom popuni da ce uvijek sve biti puno 
// lako se izmijeni ali otezava pracenje pseudokoda u uputama 
int zastavica = 1; 
int offset; 
int addr;
int indeks;

void inicijalizacija(){
    memset(disk, 0, sizeof(disk));
    memset(tablice_prevodenja, 0, sizeof(tablice_prevodenja));
    memset(radni_spremnik, 0, sizeof(radni_spremnik));
    memset(zauzeti_okviri, 0, sizeof(zauzeti_okviri));
}

int dohvati_fizicku_adresu(int p, int x){
    offset = x & OFFSET_MASK;
    indeks = x >> 6; // pazi - pretpostavka da je nekoristise 0 
    int present_bit = tablice_prevodenja[p][indeks] & PRESENT_MASK; 

    if(!present_bit){
        printf("\tPromasaj!\n");
        if(popunjenost < M){ // ima slobodnog mjesta
            printf("\t\tdodijeljen okvir: 0x%04x\n", indeks);
            tablice_prevodenja[p][indeks] = popunjenost << 6 | 0x20 | t; // dodaj u tablicu
            for(int i = 0; i < 64; i++){
                radni_spremnik[popunjenost][i] = disk[p][i];
            }
            addr = popunjenost;
            popunjenost += 1; 
        }
        else{ //treba nesto ukloniti
            //printf("Usli ovdje\n");
            int minlru = 31; //krecemo od najveceg moguceg
            int min_addr = 0;
            int ind_proc_zamjena;
            int ind_str_zamjena; 
            // klasicni naivni approach za pronalazak najmanjeg lru koji cemo mijenjati 
            for(int i = 0; i < N; i++){
                for(int j = 0; j < 16; j++){
                    if(tablice_prevodenja[i][j] & PRESENT_MASK){ //ako je bit pris = 1
                        if((tablice_prevodenja[i][j] & LRU_MASK) < minlru){
                            minlru = tablice_prevodenja[i][j] & LRU_MASK; 
                            ind_proc_zamjena = i; 
                            ind_str_zamjena = j;
                        }
                    }
                }
            }
            //printf("Sad smo ovdje \n");
            min_addr = (tablice_prevodenja[ind_proc_zamjena][ind_str_zamjena] & INDEX_MASK) >> 6;  
            //printf("A ovdje?\n");
            printf("\t\tIzbacujem stranicu 0x%04x iz procesa %d\n", ind_str_zamjena<<6, ind_proc_zamjena);
            printf("\t\tlru izbacene stranice: 0x%04x\n", minlru);
            printf("\t\tdodijeljen okvir 0x%04x\n", min_addr);

            // spusti bit prisutnosti najmanjem
            tablice_prevodenja[ind_proc_zamjena][ind_str_zamjena] &= 0xffdf;

            //postavi vrijendost trenutnom
            tablice_prevodenja[p][indeks] = min_addr << 6 | 0x20 | t;

            // ucitavanje s diska 
            for (int i = 0; i < 64; ++i) { 
                disk[p][i] = radni_spremnik[min_addr][i];
                radni_spremnik[min_addr][i] = disk[p][i];
            }
            addr = min_addr;
        }
        
    }
    int fizadr = (tablice_prevodenja[p][indeks] & INDEX_MASK) | offset;
    if(zastavica){
        printf("\tfiz. adresa: 0x%04x\n", fizadr);
        printf("\tzapis tablice: 0x%04x\n", tablice_prevodenja[p][indeks]);
        printf("\tsadrzaj adrese: %d\n", radni_spremnik[addr][offset]);
        
    }
    return fizadr; 
}

int dohvati_sadrzaj(int p, int x){
    zastavica = 1; 
    int y = dohvati_fizicku_adresu(p,x); 
    zastavica = 0;
    int i = radni_spremnik[y >> 6][offset]; 
    return i; 
}

void zapisi_sadrzaj(int p, int x, int i){
    int y = dohvati_fizicku_adresu(p,x);
    radni_spremnik[y >> 6][offset] = i;
}

int main(){
    srand(time(NULL));
    inicijalizacija();

    while(1){

        for(int p = 0; p < N; p++){ // za svaki proces
            printf("\n---------------------------\n");
            printf("proces: %d\n", p); 
            printf("\tt: %d\n", t);

            //x ← nasumična logička adresa;
            int x = rand() & 0x03fe;
            printf("\tlog. adresa: 0x%04x\n", x);
            int i = dohvati_sadrzaj(p, x); 
            i++;
            zapisi_sadrzaj(p,x,i); 

            t++;
            if (t >= 31){
                t = 0;
                for (int i = 0; i < N; ++i) {
                    for (int j = 0; j < 16; ++j) {
                        tablice_prevodenja[i][j] = tablice_prevodenja[i][j] & LRU_REMOVE_MASK;
                    }
                }
                tablice_prevodenja[p][indeks] = tablice_prevodenja[p][indeks] & LRU_REMOVE_MASK;
                tablice_prevodenja[p][indeks] |= 1;
            }

            sleep(1);
        }
    }
    return 0; 
}