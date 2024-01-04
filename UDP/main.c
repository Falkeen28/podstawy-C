#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>

// dla uproszczenia (dla mnie) w obłudze sygnału i sprzątaniu po sobie :)
struct sockaddr_in my_addr, to_addr;
int sockfd, send_len, pid, shmid;

typedef enum
{
    WRITE,
    JOIN,
    WIN,
    QUIT,
    WAIT
} chat_status;
// tryb wyliczenowy pomaga w obsłudze wiadomości wysyłanych pomiędzy klientami
// choc nie jest to konieczne

struct moja_wiadomosc
{
    chat_status status;
    int wartosc;
    int wynik;
    char nick[20];
    char wiadomosc[1024];
    char ostatni_gracz[20];
    char przeciwnik[20];
} msg;
struct fork_str
{
    int ile;
    int moj_wynik;
    int peer_wynik;
    char peer_nick[20];
    char ostatni_gracz[20];
    char ja[20];
} *wspolna_pamiec;
// struktura wspolna_pamiec jest wspoldzielona pomiędzy procesami rodzica i dziecka

void nagle_wysjcie(int sig)
{
    printf("\nOdebrano sygnal SIGINT\n");
    printf("Wysylam wiadomosc QUIT i sprzątam\n");
    msg.status = QUIT;
    shmdt(wspolna_pamiec);
    shmctl(shmid, IPC_RMID, 0);
    send_len = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&to_addr, (socklen_t)sizeof(my_addr));
    kill(pid, SIGTERM);
    exit(0);
}
// funkcja obsługi sygnału SIGINT - ctrl+c - wysyła wiadomość QUIT i zabija proces dziecka oraz usuwa segment pamięci wspólnej
// dziecko też wysyła wiadomość QUIT i powiadamia przeciwnika o rozłączeniu

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
// funkcja sprawdzająca czy podany ciąg znaków jest liczbą
int is_valid_ipv4(const char *addr)
{
    int segment = 0;  // ilosc segmentow
    int znaki = 0;    // ilosc znakow w segmencie
    int znak_seg = 0; // wartosc segmentu

    // sprawdzanie czy adres nie jest pusty
    if (addr == NULL)
        return 0;

    // sprawdzanie czy adres nie jest za dlugi
    while (*addr != '\0')
    {
        // sprawdzanie czy znak jest cyfra
        if (*addr == '.')
        {
            if (znaki == 0)
                return 0;
            // sprawdzanie czy segment nie jest za duzy
            if (++segment == 4)
                return 0;
            // resetowanie wartosci segmentu i znakow
            znaki = znak_seg = 0;
            addr++;
            continue;
        }
        if ((*addr < '0') || (*addr > '9'))
            return 0;
        // sprawdzanie czy wartosc segmentu nie jest za duzy
        if ((znak_seg = znak_seg * 10 + *addr - '0') > 255)
            return 0;
        // sprawdzanie czy nie ma za duzo znakow w segmencie
        addr++;
        znaki++;
    }
    if (segment != 3)
        return 0;
    if (znaki == 0)
        return 0;
    return 1;
}

int main(int argc, char *argv[])
{
    extern int errno;
    struct sockaddr_in from_addr;
    int rec_len;
    char nick[20];
    socklen_t from_len = sizeof(from_addr);
    u_short PORT;
    key_t key;

    char temp[1024];
    strcpy(temp, argv[1]);

    // obsluga sygnalu SIGINT - ctrl+c
    signal(SIGINT, nagle_wysjcie);
    // obsługa błedow - za mało argumentów, lub ich błędny typ
    if (argc < 3)
    {
        printf("Usage: %s <IP> <PORT> [nick]\n", argv[0]);
        exit(1);
    }
    if (isNumber(argv[2]) == 0)
    {
        printf("Port musi byc liczba\n");
        exit(1);
    }
    if (is_valid_ipv4(temp) == 0)
    {
        printf("takie ip nie istnieje, podaj cos w foramiec xxx.xxx.xxx.xxx gdzie xxx to lczna w zakresie 0-255\n");
        exit(1);
    }

    PORT = atoi(argv[2]);
    // inicializacja klucza
    key = ftok("main.c", 69);
    if (key == -1)
    {
        perror(" problem z kluczem");
        exit(1);
    }
    // shmget - inicializacja pamieci wspoldzielonej
    if ((shmid = shmget(key, sizeof(struct fork_str), 0755 | IPC_CREAT | IPC_EXCL)) == -1)
    {
        perror(" problem z pamiecia wspoldzielona: ");
        exit(1);
    }
    // shmat - zeby dolaczyc wspolna_pamiec
    if ((wspolna_pamiec = (struct fork_str *)shmat(shmid, (void *)0, 0)) == NULL)
    {
        perror(" problem z dolaczeniem pamiesci: ");
        shmctl(shmid, IPC_RMID, 0);
    }

    wspolna_pamiec->ile = 0;
    strcpy(wspolna_pamiec->ostatni_gracz, "");
    strcpy(msg.ostatni_gracz, "");

    if (argc < 4)
    {
        strcpy(nick, "NN");
        strcpy(msg.nick, "NN");
        strcpy(wspolna_pamiec->ja, "NN");
        strcpy(msg.przeciwnik, argv[1]);
        strcpy(wspolna_pamiec->peer_nick, argv[1]);
    }
    else
    {
        strcpy(nick, argv[3]);
        strcpy(msg.nick, argv[3]);
        strcpy(wspolna_pamiec->ja, argv[3]);
        strcpy(msg.przeciwnik, argv[1]);
        strcpy(wspolna_pamiec->peer_nick, argv[1]);
    }
    // walka o to zeby wyswietlic prawidlowy nick w wiadomosciach xDD

    msg.wartosc = 0;
    msg.wynik = 0;
    wspolna_pamiec->moj_wynik = 0;
    wspolna_pamiec->peer_wynik = 0;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("Socket error: ");
        return 1;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(PORT);

    to_addr.sin_family = AF_INET;
    inet_aton(argv[1], &(to_addr.sin_addr));
    to_addr.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Bind error: ");
        return 1;
    }

    if ((pid = fork()) == 0)
    {
        while (1)
        {
            // proces potomny obługuje odbieranie wiadomości i wyświetlanie komunikatów
            rec_len = recvfrom(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&from_addr, &from_len);
            if (rec_len != -1)
            {
                if (strcmp(wspolna_pamiec->ja, "NN") == 0)
                {
                    strcpy(nick, msg.przeciwnik);
                    strcpy(msg.nick, msg.przeciwnik);
                    strcpy(wspolna_pamiec->ja, msg.przeciwnik);
                }
                strcpy(wspolna_pamiec->peer_nick, msg.nick);
                strcpy(wspolna_pamiec->ostatni_gracz, msg.nick);
            }
            if (msg.status == JOIN)
            {
                strcpy(wspolna_pamiec->peer_nick, msg.nick);
                strcpy(wspolna_pamiec->ostatni_gracz, msg.nick);
                wspolna_pamiec->ile = 0;
                printf("Rozpoczynam czat z %s (%s)\n", msg.nick, inet_ntoa(from_addr.sin_addr));
                printf("Podaj nastepna wartość: ");
                fflush(stdout);
            }
            else if (msg.status == QUIT)
            {
                printf("\nUzytkownik %s (%s) wyszedl \n", msg.nick, inet_ntoa(from_addr.sin_addr));
                memset(&msg, 0, sizeof(msg));
                strcpy(wspolna_pamiec->ostatni_gracz, "");
                wspolna_pamiec->ile = 0;
                printf("Oczekiwanie na przeciwnika...\n");
                fflush(stdout);
            }
            else if (msg.status == WIN)
            {
                printf("Uzytkownik %s Wygral \n", msg.nick);
                printf("Podaj nastepna wartość aby zaczac gre od nowa: ");
                wspolna_pamiec->ile = 0;
                wspolna_pamiec->peer_wynik++;
                strcpy(wspolna_pamiec->ostatni_gracz, "");
                strcpy(msg.ostatni_gracz, "");
                fflush(stdout);
            }
            else if (msg.status == WRITE)
            {
                strcpy(wspolna_pamiec->ostatni_gracz, msg.nick);
                wspolna_pamiec->ile += msg.wartosc;
                printf("Uzytkownik [%s] podal liczbe: %d\n", msg.nick, wspolna_pamiec->ile);
                printf("Podaj nastepna wartość: ");
                fflush(stdout);
            }
        }
    }
    else
    {
        // proces rodzica obługuje wysyłanie wiadomości
        msg.status = JOIN;
        strcpy(wspolna_pamiec->ostatni_gracz, nick);
        send_len = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&to_addr, (socklen_t)sizeof(my_addr));
        if (send_len == -1)
        {
            perror("Send error: ");
            return 1;
        }
        printf("Wysłano prpozyce gry...\n");
        printf("Oczekiwanie na przeciwnika...\n");
        while (1)
        {
            if (strcmp(nick, wspolna_pamiec->ja) != 0)
            {
                strcpy(nick, wspolna_pamiec->ja);
                strcpy(msg.nick, wspolna_pamiec->ja);
            }
            // ciag dalszy walki o dobry nick w postaci IP xDD

            fgets(msg.wiadomosc, 256, stdin);
            msg.wiadomosc[strlen(msg.wiadomosc) - 1] = '\0';
            if (strcmp(msg.wiadomosc, "<koniec>") == 0)
            {
                msg.status = QUIT;
                send_len = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&to_addr, (socklen_t)sizeof(my_addr));
                kill(pid, SIGTERM);
                break;
            }
            else if (strcmp(msg.wiadomosc, "<wynik>") == 0)
            {
                printf("Ty %d : %d %s\n", wspolna_pamiec->moj_wynik, wspolna_pamiec->peer_wynik, wspolna_pamiec->peer_nick);
                continue;
            }
            else
            {
                if (strcmp(wspolna_pamiec->ostatni_gracz, nick) == 0)
                {
                    printf("Teraz nie twoja kolej\n");
                    continue;
                }
                else
                {
                    msg.status = WRITE;
                    msg.wartosc = atoi(msg.wiadomosc);
                    if (msg.wartosc < 1 || msg.wartosc > 10)
                    {
                        printf("Podaj liczbe z zakresu 1-10\n");
                        continue;
                    }
                    msg.wynik += msg.wartosc;
                    wspolna_pamiec->ile += msg.wartosc;
                    if (wspolna_pamiec->ile >= 50)
                    {
                        printf("Wygrana!!!\nOczekiwanie na nastepnego przeciwnika...\n");
                        msg.status = WIN;
                        wspolna_pamiec->moj_wynik++;
                        wspolna_pamiec->ile = 0;
                        strcpy(wspolna_pamiec->ostatni_gracz, "");
                        strcpy(msg.ostatni_gracz, "");
                    }
                    send_len = sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr *)&to_addr, (socklen_t)sizeof(my_addr));
                    strcpy(wspolna_pamiec->ostatni_gracz, nick);
                }
            }
        }
    }
    close(sockfd);
    shmctl(shmid, IPC_RMID, 0);
    return 0;
}
