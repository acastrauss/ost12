#include <iostream>
#include <vector>
#include <thread>
#include <fstream>
#include <string>
#include <string.h>
#include <mutex>

#define VEL_VECTORA2 256
#define EXIT_CODE -1

using namespace std;

mutex mx;

typedef vector<char>::iterator vci;
typedef vector<char>::const_iterator vcci;

void ucitajBajte(char* nazivFajla,vci it); // opis svake fje je tad kod tela iste
void ucitajN(char* nazivFajla, unsigned*n);
void uradiXOR(vcci v1begin, vcci v1end, vcci v2begin, vci v3);
vector<char> fNiti(vector<char>& v1, vector<char>& v2, const unsigned brojNiti);
void zapisiuFajl(char* nazivFajla, vcci vStart, vcci vKraj);
void skalirajKljuc(vector<char>& kljuc, unsigned ulazniSize);
void dodajVelV3(vector<char>& izlazniV, char* nazivFajla);

int main(int argsNum, char** args)
{

    char* nazivPrvog = args[1];
    char* nazivDrugog = args[2];
    char* nazivTreceg = args[3];
    const unsigned numOfThreads = atoi(args[4]);
    //cout<<numOfThreads<<endl;
    unsigned n = 0; // broj karaktera koji su dati kao prva 4 bajta u prvom fajlu

    ucitajN(nazivPrvog,&n);
    //cout<<n<<endl;

    vector<char> prvi(n);
    vector<char> drugi(VEL_VECTORA2); // objasnjeno upotrebom fja u nastavku zasto VEL_VECTORA*2

    vci itPrvi = prvi.begin();
    vci itDrugi = drugi.begin();

    ucitajBajte(nazivPrvog,itPrvi);

    ucitajBajte(nazivDrugog,itDrugi);

    vector<char> treci = fNiti(prvi,drugi,numOfThreads);
    vci itTreci;

    dodajVelV3(treci,nazivPrvog);

    zapisiuFajl(nazivTreceg,treci.begin(),treci.end());
    //cout<<drugi.size()<<endl;
    //cout<<treci.size()<<endl;
    //ovde su ispisivani karakteri redom vektora radi provere onih koji su citljivi
    /*
    for(itPrvi = prvi.begin(); itPrvi!=prvi.end(); itPrvi++){
        cout<<*itPrvi<<endl;
    }
    *//*
    int i = 0;
    for(itDrugi = drugi.begin(); itDrugi!=drugi.end(); itDrugi++){
        cout<<*itDrugi<<endl;
        i++;
    }
    cout<<i<<endl;
    *//*
    for(itTreci = treci.begin(); itTreci!=treci.end(); itTreci++){
        cout<<*itTreci<<endl;
    }
    */

    return 0;
}

void ucitajN(char* nazivFajla, unsigned*n){
    /*ova fja ucitava prva 4 bajta iz fajla da bi pronasla unsigned broj koji predstavlja broj
    ostalih bajtova
    */
    ifstream fajl;
    char buffer[4];

    fajl.open(nazivFajla,ios::in|ios::binary);

    if(fajl.is_open()){
        fajl.read(buffer,4);
        *n = *(unsigned*)buffer;
    }
    else{cout<<"Couldn't open the file!\n"<<endl; exit(EXIT_CODE);}
    fajl.close();
}

void ucitajBajte(char* nazivFajla, vci it){
    /*ucitava bajte iz fajla, i smesta u odredjeni vektor,
    odbacuje prva 4 bajta jer se oni ne koriste za xor vec predstavljaju broj ostalih
    */
    ifstream fajl;
    char temp;
    char garbage[4];

    fajl.open(nazivFajla,ios::in|ios::binary);

    if(fajl.is_open()){
        fajl.read(garbage,4); // da se preskoci prvih nekoliko bajta

        while(!fajl.eof()){
            fajl.read(&temp,1);
            if(!fajl.eof()){
                *it++ = temp;
                /* razlog zbog kog sam ovde stavio proveru da nije doslo do kraja fajla
                je taj sto ce u prolazu u kom ucita poslednji karakter u fajlu(verovatno EOF), on ce upisati
                i taj karakter u vektor iako ne bi trebalo, jer ce tek u sledecem prolazu da uradi proveru
                da li je fajl stigao do kraja
                */
            }
        }
    }
    else{cout<<"Couldn't open the file!\n"<<endl; exit(EXIT_CODE);}

    fajl.close();
}

void uradiXOR(vcci v1begin, vcci v1end, vcci v2begin, vci v3){
    // ova fja opisuje aktivnost svake niti
    unique_lock<mutex> lock(mx); // mutex za medjusobnu iskljucivost niti pri radu sa vektoraom
    for(; v1begin!=v1end ; v1begin++, v2begin++, v3++){
        *v3 = (char)((*v1begin) ^ (*v2begin));
        //cout<<*v3<<endl;
    }
}

vector<char> fNiti(vector<char>& v1, vector<char>& v2,const unsigned brojNiti){
    /* ova fja pravi i poziva niti, koje opisuje fja uradiXOR, poceci i krajevi obradjenih delova vektora se smenjuju,
    a vektor kljuca je povecan ciklicno sve do velicine koju ima ulazni vektor, ukoliko je on veci od kljuca. U suprotnom
    kljuc ostaje isti, i radice se xor sve dok ima elemenata u ulaznom vektoru.
    */
    unsigned velUlaznog = v1.size();
    unsigned velKljuca = v2.size();

    vector<char> v3(v1.size());
    unsigned deoUlaznog = velUlaznog / brojNiti; // ovaj deo se dodaje svakoj niti osim poslednje jer se njoj
    //daje potencijalno vise bajtova
    //cout<<deoUlaznog<<endl;
    //cout<<v2.size()<<endl;
    if(velUlaznog > v2.size()) // povecace drugi da bude iste velicine kao prvi
        skalirajKljuc(v2,velUlaznog);

    thread nizT[brojNiti];

    vcci pocetakPrvi = v1.begin();
    vcci krajPrvi = v1.begin() + deoUlaznog;
    vcci pocetakDrugi = v2.begin();
    vci pocetakTreci = v3.begin();

    //cout<<(v2.size()==velUlaznog/brojNiti)<<endl;
    for(int i = 0 ; i < brojNiti-1; i++){
        nizT[i] = thread(uradiXOR,pocetakPrvi,krajPrvi,pocetakDrugi,pocetakTreci);
        pocetakPrvi = krajPrvi;
        krajPrvi += deoUlaznog;
        pocetakDrugi += deoUlaznog; // i drugi se pomera za deo koji se dodaje nitima jer je drugi iste velicine kao prvi
        pocetakTreci += deoUlaznog;
    }
        nizT[brojNiti-1] = thread(uradiXOR, pocetakPrvi, v1.end(), pocetakDrugi, pocetakTreci);

    for(int i = 0; i < brojNiti ; i++){
        nizT[i].join();
    }

    return v3;
}

void zapisiuFajl(char* nazivFajla, vcci vStart, vcci vKraj){
    //fja koja zapisuje u dati vektor u dati fajl
    ofstream fajl;
    fajl.open(nazivFajla);

    if(fajl.is_open()){
        for(;vStart != vKraj; vStart++)
            fajl<<*vStart;
    }
    else{cout<<"Couldn't open the file!\n"<<endl; exit(EXIT_CODE);}

    fajl.close();

}

void skalirajKljuc(vector<char>& kljuc, unsigned ulazniSize){ // radi se samo kada je deo veci od kljuca
    /* ova fja ce dopuniti kljuc, tj povecace ga da bi imao isti broj bajtova kao i ulazni
    tako sto ce kruzno dodeljivati bajtove prvobitnog kljuca
    */
    int povecanje = ulazniSize - kljuc.size(); // ovde je broj bajtova koji treba da se dodaju
    int i = 0;
    int s = kljuc.size();
    while(povecanje--){
        kljuc.push_back(kljuc[i++]);
        i = (i == s ? 0 : i); // zbog kruznog dodavanja
    }
}

void dodajVelV3(vector<char>& izlazniV, char* nazivFajla){
    /*fja koja dodaje broj ostalih bajtova na pocetak izlaznog vektora, dole sam taj broj nazvao zaglavlje
    */
    ifstream fajl;
    fajl.open(nazivFajla,ios::in|ios::binary);
    char buffer;
    vector<char> temp1;
    vector<char> temp2;
    int i = 0;

    if(fajl.is_open()){
        while(i<4){ // jer 4 bajta je zaglavlje
            fajl.read(&buffer,1);
            temp1.push_back(buffer);
            i++;
        }
    }
    else{cout<<"Couldn't open the file!\n"<<endl; exit(EXIT_CODE);}
    // ovde se spajaju izlazni i vektor gde je zaglavlje
    temp2 = temp1;
    temp2.insert(temp2.end(),izlazniV.begin(),izlazniV.end());
    izlazniV = temp2;

    fajl.close();
}
