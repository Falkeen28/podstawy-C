#define _WITH_GETLINE
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

char buf[MY_MSG_SIZE];

void desbloquear() //z hiszpańskiego "odblokować"
{
    /* oblokowanie semafora */
    sb.sem_op = 1;
    if (semop(semid, &sb, 1) == -1)
    {
        printf("Blad semop!\n");
        exit(1);
    }
}

void sgnhandle(int signal)
{
    printf("\n[KLIENT]: dostalem SIGINT => koncze i odblokowyeuje semafor...");
    desbloquear();
    printf("GOTOWE!\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    char key;
    int counter = 1;           // counter ktory pomaga wpisywac i wypisywac wpisy
    int counter2 = 1;          // counter ktory pomaga zorientowac sie ile jest wpisow dodanych
    signal(SIGINT, sgnhandle); /* obsluga sygnalu SIGINT */
    /* obsługa błędow czy dobra ilosc argumentow i czy dobre argumenty */
    if (argc != 3)
    {
        printf("zła liczba argumentow\n");
        exit(1);
    }

    /* tworzenie klucza */
    if ((shmkey = ftok(argv[1], 1)) == -1)
    {
        perror("Blad shmkey");
        exit(1);
    }

    /* obsługa błędow czy została stworzona pamiec wspołdzielona */
    if ((shmid = shmget(shmkey, 0, 0)) == -1)
    {
        printf(" blad shmget\n");
        exit(1);
    }

    /* przygotowanie do zablokowania zasobu semafora */
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if ((semid = semget(shmkey, 1, 0)) == -1)
    {
        printf("Blad semget\n");
        exit(1);
    }
    printf("Twitter 2.0 wita! (wersja B)\nSprawdzam czy moge komunikowac sie z serwerem... \n");

    /* blokujemy semafor */
    if (semop(semid, &sb, 1) == -1)
    {
        printf("Blad semop!\n");
        exit(1);
    }
    printf("OK!\n");

    /* obsługa błędow czy zostałą podłączona pamiec wspoldzielona */
    shared_data = (struct my_data *)shmat(shmid, (void *)0, 0);
    if (shared_data == (struct my_data *)-1)
    {
        printf(" blad shmat!\n");
        exit(1);
    }

    /* pętla do sprawdzenia ile mamy wpisow i ile wolengo miejsca */
    for (int i = 1; i <= shared_data[0].quantity; i++)
    {
        if (shared_data[i].type != 0)
        {
            counter2++;
        }
    }
    printf("[Wolnych %d wpisow (na %d)]\n", (shared_data[0].quantity + 1) - counter2, shared_data[0].quantity);

    /* pętla do wypisania wpisow */
    for (int i = 1; i <= shared_data[0].quantity; i++)
    {
        if (shared_data[i].type != 0)
        {
            printf("%d. [%s]: %s", i, shared_data[i].name, shared_data[i].msg);
            printf(" [Polubienia: %d]\n", shared_data[i].like);
            counter++;
        }
    }
    printf("Podaj akcje (N)owy wpis, (L)ike\n");
    scanf("%c", &key);

    /* taka sztuczka zeby działało :D */
    fflush(stdin);
    if (key == 'N' || key == 'n')
    {
        if (counter2 == (shared_data[0].quantity + 1))
        {
            printf("Pamiec pełna, przepraszamy!\n");
            desbloquear();
            exit(1);
        }
        printf("Napisz co ci chodzi po głowie:\n");

        /* taka sztuczka zeby działało :D */
        while (getchar() != '\n')
        {
            fflush(stdin);
        }
        fgets(buf, MY_MSG_SIZE, stdin);
        shared_data[counter].type = 1;
        shared_data[counter].like = 0;
        strcpy(shared_data[counter].name, argv[2]);
        /* techniczne: usuwam koniec linii */
        buf[strlen(buf) - 1] = '\0';
        strcpy(shared_data[counter].msg, buf);
        shmdt(shared_data); // odłączamy się od pamięci współdzielonej
    }
    else if (key == 'L' || key == 'l')
    {
        printf("Podaj numer wpisu, ktory chcesz polubic:\n");
        scanf("%d", &counter);
        if (counter >= counter2 || counter < 1)
        {
            printf("NIE MA TAKIEGO WPISU!\n");
            desbloquear();
            exit(1);
        }
        shared_data[counter].like++;
        shmdt(shared_data);
    }
    else
    {
        printf("NIE MA TAKIEJ OPCJI!\n");
        desbloquear();
        exit(1);
    }
    desbloquear();
    printf("Dziekuje za dokonanie wpisu do Twitter 2.0\n");

    return 0;
}