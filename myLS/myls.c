#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

struct plik
{
    char *uprawnienia;
    int czyLink;
    int dowiozania;
    char user[32];
    char grupa[32];
    int rozmiar;
    time_t mon;
    time_t mday;
    time_t hour;
    time_t min;
    char linkname[256];
    char d_name[256];
};
typedef struct plik PLIK;
char *month(int a)
{
    /* prosta funkcja zamieniająca wartość int ktora na string zeby spełnić wymagania 2 trybu w wyswietlaniu daty :) */
    char *miesiac = NULL;
    switch (a)
    {
    case 1:
        miesiac = "stycznia";
        break;
    case 2:
        miesiac = "lutego";
        break;
    case 3:
        miesiac = "marca";
        break;
    case 4:
        miesiac = "kwietnia";
        break;
    case 5:
        miesiac = "maja";
        break;
    case 6:
        miesiac = "czerwca";
        break;
    case 7:
        miesiac = "lipca";
        break;
    case 8:
        miesiac = "sierpnia";
        break;
    case 9:
        miesiac = "września";
        break;
    case 10:
        miesiac = "pazdziernika";
        break;
    case 11:
        miesiac = "listopada";
        break;
    case 12:
        miesiac = "grudnia";
        break;
    }
    return miesiac;
}

void cat(char nazwa[])
{
    /* prosty "cat" ktory wypisuje na sztywno ustawione 77 znakow z pliku tekstowego w 2 trybie */
    printf("Pierwsze 77 znakow:\n");
    FILE *plik;
    plik = fopen(nazwa, "r");
    int ile = 0;
    char znak;
    while ((znak = fgetc(plik)) != EOF && ile < 77)
    {
        printf("%c", znak);
        ile++;
    }
    fclose(plik);
}
void rwx(int a)
{
    /* w tej funkcji jest sprawdzane jaka grupa i jakie ma prawa dostępu do "pliku" w trybie 2  */
    switch (a)
    {
    case 7:
        printf("rwx");
        break;
    case 6:
        printf("rw-");
        break;
    case 5:
        printf("r-x");
        break;
    case 4:
        printf("r--");
        break;
    case 3:
        printf("-wx");
        break;
    case 2:
        printf("-w-");
        break;
    case 1:
        printf("--x");
        break;
    case 0:
        printf("---");
        break;
    }
}
char *Que_es(int a)
{
    /* funkcja zwraca sformatowany string z danymi co to za plik i jakie ma uprawnienia */
    char *x = malloc(11);

    /* sprawdza czy plik czy katalog czy link */
    switch (a & S_IFMT)
    {
    case S_IFDIR:
        x[0] = 'd';
        break;
    case S_IFLNK:
        x[0] = 'l';
        break;
    default:
        x[0] = '-';
        break;
    }
    /* sprawdza uprawnienia */
    if (a & S_IRUSR)
        x[1] = 'r';
    else
        x[1] = '-';
    if (a & S_IWUSR)
        x[2] = 'w';
    else
        x[2] = '-';
    if (a & S_IXUSR)
        x[3] = 'x';
    else
        x[3] = '-';
    if (a & S_IRGRP)
        x[4] = 'r';
    else
        x[4] = '-';
    if (a & S_IWGRP)
        x[5] = 'w';
    else
        x[5] = '-';
    if (a & S_IXGRP)
        x[6] = 'x';
    else
        x[6] = '-';
    if (a & S_IROTH)
        x[7] = 'r';
    else
        x[7] = '-';
    if (a & S_IWOTH)
        x[8] = 'w';
    else
        x[8] = '-';
    if (a & S_IXOTH)
        x[9] = 'x';
    else
        x[9] = '-';

    x[10] = '\0';
    return x;
}
void ordenamiento_de_burbuja(PLIK *a, int n)
{
    /* najprosztsze i nasze ulubione sortowanie :) */
    int i, j;
    PLIK t;
    for (i = 2; i < n; i++)
    {
        for (j = 2; j < n; j++)
        {
            if (strcmp(a[i].d_name, a[j].d_name) < 0)
            {
                t = a[i];
                a[i] = a[j];
                a[j] = t;
            }
        }
    }
}
int main(int argc, char *argv[])
{
    extern int errno;
    DIR *dirp = NULL;
    struct dirent *direntp;
    struct stat statbuf;
    struct passwd *pwd;
    struct group *grupa;
    struct tm *times, *times1, *times2, *times3;
    struct stat link;
    char charBuff[1024] = {'\0'};
    int i = 0;

    /* sprawdzenie czy program ma działać w 1 czy 2 trybie */
    if (argc == 1)
    {
        dirp = opendir(".");

        /* alokujemy pamiec na strukture do ktorej zapiszemy informacje do pliku */
        PLIK *pliczek = malloc(sizeof(PLIK)); 

        /* obługa błędów */
        if (dirp == NULL)
        {
            perror("brak folderu ");
            printf("Numer bledu: %d\n", errno);
            exit(1);
        }

        /* w pętli która wykonuje się póki nie przejdzie po całej zawartości katalogu wyciagane
            są wszytkie informacje o danym pliku/podkatalogu */
        while (1)
        {
            direntp = readdir(dirp);
            if (direntp == NULL)
                break;

            /* realokujemy pamiec zeby miec miejsce na kolejne rzeczy ktore beda w folderze */
            pliczek = (PLIK *)realloc(pliczek, (i + 2) * sizeof(PLIK)); 

            lstat(direntp->d_name, &statbuf);
            lstat(direntp->d_name, &link);

            pwd = getpwuid(statbuf.st_uid);
            grupa = getgrgid(statbuf.st_gid);
            times = localtime(&(statbuf.st_mtime));

            /* wrzucamy rzeczy do struktury */
            pliczek[i].uprawnienia = Que_es(statbuf.st_mode);
            pliczek[i].dowiozania = statbuf.st_nlink;
            strcpy(pliczek[i].user, pwd->pw_name);
            strcpy(pliczek[i].grupa, grupa->gr_name);
            pliczek[i].rozmiar = statbuf.st_size;
            pliczek[i].mon = (times->tm_mon) + 1;
            pliczek[i].mday = times->tm_mday;
            pliczek[i].hour = times->tm_hour;
            pliczek[i].min = times->tm_min;
            strcpy(pliczek[i].d_name, direntp->d_name);
            if ((statbuf.st_mode & S_IFMT) == S_IFLNK)
            {
                readlink(direntp->d_name, charBuff, sizeof(charBuff));
                strcpy(pliczek[i].linkname, charBuff);
                pliczek[i].czyLink = 1;
            }
            else
            {
                pliczek[i].czyLink = 0;
            }
            i++;
        }

        ordenamiento_de_burbuja(pliczek, i);

        for (int j = 0; j < i; j++)
        {
            /* drukujemy rzeczy ze struktury juz po po sortowaniu */
            printf("%s", pliczek[j].uprawnienia);
            printf(" %2d ", pliczek[j].dowiozania);
            printf("%s  ", pliczek[j].user);
            printf("%s  ", pliczek[j].grupa);
            printf("%5d  ", pliczek[j].rozmiar);
            printf("%02ld-", pliczek[j].mon);
            printf("%02ld ", pliczek[j].mday);
            printf("%02ld:", pliczek[j].hour);
            printf("%02ld ", pliczek[j].min);
            printf("%s ", pliczek[j].d_name);
            if (pliczek[j].czyLink == 1)
            {
                printf("-> %s ", pliczek[j].linkname);
            }
            printf("\n");
         }  
        /* zwalnianie pamieci */
        pliczek = NULL;
        free(pliczek);
        closedir(dirp);
    }
    /* sprawdzenie czy program ma działać w 1 czy 2 trybie */
    else
    {
        if (argc > 2)
        {
            perror("Za duzo argumentow ");
            printf("Numer bledu: %d\n", errno);
            exit(1);
        }
        if (lstat(argv[1], &statbuf) == 0)
        {
            char cwd[256];
            times1 = localtime(&(statbuf.st_atime));
            times2 = localtime(&(statbuf.st_mtime));
            times3 = localtime(&(statbuf.st_ctime));
            int ile_podkatalogow = 0;
            printf("Informacje o: %s\n", argv[1]);

            if ((statbuf.st_mode & S_IFMT) == S_IFDIR)
            {
                printf("Typ pliku: katalog \n");
                printf("Sciezka: %s/%s\n", getcwd(cwd, sizeof(cwd)), argv[1]);

                /* funkcja zlicza ile jest podkatalogow w badanym pliku jesli okaze sie on katalogiem */
                dirp = opendir(argv[1]);
                int zadzialaj_pls_jpg = dirfd(dirp);
                while (1)
                {
                    direntp = readdir(dirp);
                    if (direntp == NULL)
                    {
                        break;
                    }
                    fstatat(zadzialaj_pls_jpg, direntp->d_name, &statbuf, 0);
                    if ((statbuf.st_mode & S_IFMT) == S_IFDIR && strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0)
                    {
                        ile_podkatalogow++;
                    }
                }
                closedir(dirp);

                printf("Liczba podkatalogow: %d\n", ile_podkatalogow);

                printf("Uprawnienia: Uzytkownik: ");
                rwx((statbuf.st_mode & S_IRWXU) >> 6);
                printf(", Grupa: ");
                rwx((statbuf.st_mode & S_IRWXG) >> 3);
                printf(", Innni: ");
                rwx((statbuf.st_mode & S_IRWXO));
                printf("\n");

                printf("Ostatnio uzywany:        %d %s %d roku o %02d:%02d:%02d\n", times1->tm_mday, month(times1->tm_mon + 1), times1->tm_year + 1900, times1->tm_hour, times1->tm_min, times1->tm_sec);
                printf("Ostatnio modyfikowany:   %d %s %d roku o %02d:%02d:%02d\n", times2->tm_mday, month(times2->tm_mon + 1), times2->tm_year + 1900, times2->tm_hour, times2->tm_min, times2->tm_sec);
                printf("Ostatnio zmieniany stan: %d %s %d roku o %02d:%02d:%02d\n", times3->tm_mday, month(times3->tm_mon + 1), times3->tm_year + 1900, times3->tm_hour, times3->tm_min, times3->tm_sec);
            }
            else if ((statbuf.st_mode & S_IFMT) == S_IFREG)
            {
                printf("Typ pliku: plik \n");
                printf("Sciezka: %s/%s\n", getcwd(cwd, sizeof(cwd)), argv[1]);
                printf("Rozmair: %ld bajty\n", statbuf.st_size);

                printf("Uprawnienia: Uzytkownik: ");
                rwx((statbuf.st_mode & S_IRWXU) >> 6);
                printf(", Grupa: ");
                rwx((statbuf.st_mode & S_IRWXG) >> 3);
                printf(", Innni: ");
                rwx((statbuf.st_mode & S_IRWXO));
                printf("\n");

                printf("Ostatnio uzywany:        %d %s %d roku o %02d:%02d:%02d\n", times1->tm_mday, month(times1->tm_mon + 1), times1->tm_year + 1900, times1->tm_hour, times1->tm_min, times1->tm_sec);
                printf("Ostatnio modyfikowany:   %d %s %d roku o %02d:%02d:%02d\n", times2->tm_mday, month(times2->tm_mon + 1), times2->tm_year + 1900, times2->tm_hour, times2->tm_min, times2->tm_sec);
                printf("Ostatnio zmieniany stan: %d %s %d roku o %02d:%02d:%02d\n", times3->tm_mday, month(times3->tm_mon + 1), times3->tm_year + 1900, times3->tm_hour, times3->tm_min, times3->tm_sec);

                /* naiwny "cat" */
                cat(argv[1]);
            }
            else if ((statbuf.st_mode & S_IFMT) == S_IFLNK)
            {
                printf("Typ pliku: link symboloiczny \n");
                printf("Sciezka: %s/%s\n", getcwd(cwd, sizeof(cwd)), argv[1]);
                char link[255];
                realpath(argv[1], link);
                printf("Wskazuje na: %s\n", link);
                printf("Rozmair: %ld bajty\n", statbuf.st_size);

                printf("Uprawnienia: Uzytkownik: ");
                rwx((statbuf.st_mode & S_IRWXU) >> 6);
                printf(", Grupa: ");
                rwx((statbuf.st_mode & S_IRWXG) >> 3);
                printf(", Innni: ");
                rwx((statbuf.st_mode & S_IRWXO));
                printf("\n");

                printf("Ostatnio uzywany:        %d %s %d roku o %02d:%02d:%02d\n", times1->tm_mday, month(times1->tm_mon + 1), times1->tm_year + 1900, times1->tm_hour, times1->tm_min, times1->tm_sec);
                printf("Ostatnio modyfikowany:   %d %s %d roku o %02d:%02d:%02d\n", times2->tm_mday, month(times2->tm_mon + 1), times2->tm_year + 1900, times2->tm_hour, times2->tm_min, times2->tm_sec);
                printf("Ostatnio zmieniany stan: %d %s %d roku o %02d:%02d:%02d\n", times3->tm_mday, month(times3->tm_mon + 1), times3->tm_year + 1900, times3->tm_hour, times3->tm_min, times3->tm_sec);
            }
        }
        else
        {
            perror("Blad nie istnieje taka rzecz ");
            printf("Numer bledu: %d\n", errno);
            exit(1);
        }
    }
    return 0;
}
