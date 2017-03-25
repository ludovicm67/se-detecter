#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

/**
 * @brief Longeur d'une chaîne de caractère
 * @details Retourne la longueur d'une chaîne de caractères
 *
 * @param str chaîne de caractères dont l'on souhaite connaître la longueur
 * @return longueur de la chaine de caractères passée en argument (str)
 */
unsigned int str_length(char * str) {
    int i = 0;
    while (str[i] != '\0') i++;
    return i;
}

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
    char usage[] = "Usage: ";
    char usage_opts[] = " [-t format][-i intervalle][-l limite][-c]";
    char usage_args[] = " prog arg...arg\n";

    check_error(write(2, usage, sizeof(usage)), "write");
    check_error(write(2, program_name, str_length(program_name)), "write");
    check_error(write(2, usage_opts, sizeof(usage_opts)), "write");
    check_error(write(2, usage_args, sizeof(usage_args)), "write");

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

    if(write(1, outstr, sizeof(outstr)) == -1){
      perror("write");
      exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    int i, c, errflg = 0, raison, nb_args;
    char* args[10];
    pid_t pid;

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

    // S'il y a eu un problème d'options ou d'argument
    if (errflg) usage(argv[0]);
    nb_args = argc - optind;
    if (nb_args <= 0) usage(argv[0]);

    // On initialise le tableau d'arguments pour le exec
    for (i = optind; i < argc; i++) args[i-optind] = argv[i];
    args[nb_args] = NULL;

    while (limit >= 0) {
        if (time_format) print_time(time_format);

        check_error(pid = fork(), "fork");
        if (!pid) { // fils
            execvp(argv[optind], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        check_error(wait(&raison), "wait");

        if (limit == 1) break;
        if (limit > 0) limit--;
        check_error(usleep(interval * 1000), "usleep");
    }

    (void) time_format;
    (void) interval;
    (void) limit;
    (void) code_change;

    return EXIT_SUCCESS;
}
