#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#define MY_MSG_SIZE 128
#define MY_NAME_SIZE 32

key_t shmkey;
int shmid, semid;
struct my_data
{
    int type;
    int like;
    int quantity;
    char msg[MY_MSG_SIZE];
    char name[MY_NAME_SIZE];
} *shared_data;

struct sembuf sb;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} arg;

void sgnhandle(int signal)
{
    printf("\n[Serwer]: dostalem SIGINT => koncze i sprzatam...");
    printf(" (pamiec: odlaczenie: %s, usuniecie: %s )\n",
           (shmdt(shared_data) == 0) ? "OK" : "blad shmdt",
           (shmctl(shmid, IPC_RMID, 0) == 0) ? "OK" : "blad shmctl");
    printf(" (semafor: usuniecie: %s )\n",
           (semctl(semid, IPC_RMID, 0) == 0) ? "OK" : "blad semctl");
    exit(0);
}

/* wypisywanie po wciśnięciu CTRL^Z */
void wypisz(int signal)
{
    if (shared_data[1].type == 0)
        printf("\nBRAK WPSIOW\n");
    else
    {
        printf("\n___________  Twitter 2.0:  ___________\n");
        for (int i = 1; i <= shared_data[0].quantity; i++)
        {
            if (shared_data[i].type != 0)
            {
                printf("[%s]: %s", shared_data[i].name, shared_data[i].msg);
                printf(" [Polubienia: %d]\n", shared_data[i].like);
            }
        }
    }
}

int isNumber(char number[])
{
    int i = 0;
    // sprawdzanie czy liczba jest ujemna
    if (number[0] == '-')
        return 0;
    for (; number[i] != 0; i++)
    {
        // if (number[i] > '9' || number[i] < '0')
        if (!isdigit(number[i]))
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    struct shmid_ds buf;
    int n = atoi(argv[2]); /* ilosc wpisow */
    /* sprawdzanie czy podano odpowiednia ilosc argumentow oraz czy argumenty sa odpowiedniego typu */
    if (argc != 3 || isNumber(argv[2]) == 0) 
    {
        printf("zła liczba argumentow lub ich typ\n");
        exit(1);
    }
    signal(SIGINT, sgnhandle); /* obsluga sygnalu SIGINT */
    signal(SIGTSTP, wypisz);   /* obsluga sygnalu SIGTSTP */
    printf("[Serwer]: Twitter 2.0 (wersja B)\n");

    /* tworzenie klucza */
    printf("[Serwer]: tworze klucz na podstawie pliku %s...", argv[1]);
    if ((shmkey = ftok(argv[1], 1)) == -1)
	{
		perror("Blad shmkey");
        exit(1);
	}
    printf(" OK (klucz: %d)\n", shmkey);

    /* obsługa błędow czy została stworzona pamiec wspołdzielona */
    printf("[Serwer]: tworze segment pamieci wspolnej na %d wpisow po %db...\n", n, MY_MSG_SIZE);
    if ((shmid = shmget(shmkey, sizeof(struct my_data), 0600 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror(" blad shmget!\n");
        exit(1);
    }

    /* tworzenie zbioru semaforow */
    printf("[Serwer]: tworze zbior semaforow...\n");
    if ((semid = semget(shmkey, 1, 0600 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror(" blad semget!\n");
        exit(1);
    }
    shmctl(shmid, IPC_STAT, &buf); /* pobranie informacji o pamieci wspoldzielonej */
    printf("	  OK (id: %d, rozmiar: %zub)\n", shmid, buf.shm_segsz);
	printf("[Serwer]: dolaczam pamiec wspolna...");

    /* obsługa błędow czy zostałą podłączona pamiec wspoldzielona */
    shared_data = (struct my_data *)shmat(shmid, (void *)0, 0); 
    if (shared_data == (struct my_data *)-1)
    {
        perror(" blad shmat!\n");
        exit(1);
    }

    arg.val = 1; /* inicjalizacja semafora */
    if (semctl(semid, 0, SETVAL, arg) == -1)
    {
        perror(" blad semctl!\n"); 
        exit(1); 
    }
    printf("OK (adres: %ld)\n", (long int)shared_data);
    
    printf("[Serwer]: nacisnij Crtl^Z by wyswietlic stan serwisu\n"); 
    printf("[Serwer]: nacisnij Crtl^C by zakonczyc program\n"); 
    shared_data[0].quantity = atoi(argv[2]); 

    /* przykładowe dane do testow */
    shared_data[1].type = 1; // 1 - aktywny, 0 - nieaktywny
    shared_data[1].like = 3; // ilosc lajkow
    strcpy(shared_data[1].name, "Janek"); // nazwa uzytkownika
    strcpy(shared_data[1].msg, "Witam na Twitterze 2.0"); // wiadomosc

    while (8)
    {
        sleep(1);
    }

    return 0;
}
