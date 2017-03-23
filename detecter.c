#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

/* TODO :
 - remplacer les printf par des write
 - dans print_time voir en détail pour vérifier les erreurs

*/


/**
 * @brief Vérifie s'il y a une erreur
 * @details Vérifie si la commande (qui doit nécéssairement retourner un int,
 *          avec -1 en cas d'erreur) dans status a échouée ou non, ce qui est
 *          très pratique pour les primitives système par exemple. Lors d'une
 *          erreur, on affiche le message 'msg' et on quitte le programme.
 *
 * @param status un entier, en général le code de retour d'une fonction
 * @param msg le message à afficher en cas d'erreur
 */
int check_error(int status, char * msg) {
    if (status == -1) {
        perror(msg);
        exit(EXIT_FAILURE);
    }
    return status;
}

void usage(char * program_name) {
    printf("Usage: %s ", program_name);
    printf("[-t format] [-i intervalle] [-l limite] [-c] prog arg ... arg\n");
    exit(EXIT_FAILURE);
}

void print_time(char * time_format) {
    char outstr[200];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
       perror("localtime");
       exit(EXIT_FAILURE);
    }

    if (strftime(outstr, sizeof(outstr), time_format, tmp) == 0) {
       fprintf(stderr, "strftime returned 0");
       exit(EXIT_FAILURE);
    }

    printf("%s\n", outstr);
}

int main(int argc, char *argv[]) {
    int c, errflg = 0;

    char * time_format = NULL; // option -t
    int interval = 10000; // option -i
    int limit = 0; // option -l
    int code_change = 0; // option -c

    while ((c = getopt(argc, argv, "+t:i:l:c")) != EOF) {
        switch (c) {
            case 't':
                time_format = optarg;
                break;
            case 'i':
                interval = atoi(optarg);
                break;
            case 'l':
                limit = atoi(optarg);
                break;
            case 'c':
                code_change = 1;
                break;
            case '?': /*option non reconnue */
                errflg++;
                break;
        }
    }
    if (errflg) usage(argv[0]);
    if ((argc - optind) <= 0) usage(argv[0]);

    // Debug des options :
    printf("OPTION t = %s\n", time_format);
    printf("OPTION i = %d\n", interval);
    printf("OPTION l = %d\n", limit);
    if (code_change) printf("OPTION c :)\n");

    // Debug des arguments
    int i;
    for (i = 0; i < argc; i++) {
        printf("ARG %d = %s\n", i, argv[i]);
    }

    for (i = optind; i < argc; i++) {
        printf("COMMANDE (arg[%d]) = %s\n", i-optind, argv[i]);
    }

    while (limit >= 0) {
        if (time_format) print_time(time_format);

        if (limit == 1) limit = -1;
        if (limit > 0) limit--;
        check_error(usleep(interval * 1000), "usleep");
    }

    (void) time_format;
    (void) interval;
    (void) limit;
    (void) code_change;

    return EXIT_SUCCESS;
}
