#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

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
    char err_no_outstr[] = "strftime returned 0";
    char outstr[200];
    struct tm *tmp;
    time_t t;
    unsigned int nbc;

    t = time(NULL);
    tmp = localtime(&t);
    if (tmp == NULL) {
        perror("localtime");
        exit(EXIT_FAILURE);
    }

    if ((nbc = strftime(outstr, sizeof(outstr)-1, time_format, tmp)) == 0) {
        check_error(write(2, err_no_outstr, sizeof(err_no_outstr)), "write");
        exit(EXIT_FAILURE);
    }
    outstr[nbc++] = '\n';
    outstr[nbc] = '\0';

    check_error(write(1, outstr, nbc), "write");
}

int getchar_buff(int fd){ // getchar bufferisé
  static int nbAppels = 0;
  static int buff_size = 0;
  static unsigned char b[1024];
  if (buff_size == 0) {
      nbAppels = 0;
      buff_size = read(fd, b, sizeof(b));
      if (buff_size == -1 || buff_size == 0) return EOF;
  }
  buff_size--;
  return b[nbAppels++];
}

int main(int argc, char *argv[]) {
    int c, i, raison, nb_args, iteration = 0, affiche = 0, tube[2];
    int pos_buff = 0, pos_buffC = 0, taille_buff = 1024, taille_buffC = 1024;
    unsigned int errflg = 0, first = 1, last_code = 0, code;
    char * args[argc];
    char* buffer = malloc(taille_buff*sizeof(char));
    char* current_buff = malloc(taille_buffC*sizeof(char));
    char a;
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
                if ((interval = atoi(optarg)) <= 0) {
                    fprintf(stderr, "-i doit être supérieur à 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                if ((limit = atoi(optarg)) < 0) {
                    fprintf(stderr, "-l doit être supérieur ou égal à 0\n");
                    exit(EXIT_FAILURE);
                }
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
        check_error(pipe(tube), "pipe");
        check_error(pid = fork(), "fork");
        if (!pid) { // fils
            close(tube[0]);
            dup2(tube[1], 1);
            execvp(argv[optind], args);
            perror("execvp");
            close(tube[1]);
            exit(EXIT_FAILURE);
        } else { // père
          close(tube[1]);
          while (((a = getchar_buff(tube[0])) != EOF)) {
            if (!iteration) {
              affiche = 1;
              *(buffer + (pos_buff++)) = a;
              if (pos_buff == taille_buff) {
                taille_buff += 1024;
                buffer = realloc(buffer, taille_buff*sizeof(char));
                if (buffer == NULL) {
                  fprintf(stderr, "Allocation impossible\n");
                  exit(EXIT_FAILURE);
                }
              }
            } else affiche = 0;

            *(current_buff + (pos_buffC++)) = a;
            if (pos_buffC == taille_buffC) {
              taille_buffC += 1024;
              current_buff = realloc(current_buff, taille_buffC*sizeof(char));
              if (current_buff == NULL) {
                fprintf(stderr, "Allocation impossible\n");
                exit(EXIT_FAILURE);
              }
            }
            iteration ++;
          }
          if (strcmp(buffer, current_buff)) {
            memset(buffer, 0, taille_buff);
            strcpy(buffer, current_buff);
            affiche = 1;
          } else affiche = 0;
          pos_buff = 0;
          pos_buffC = 0;
          memset(current_buff, 0, taille_buff);
          if (affiche) printf("%s", buffer);
          close(tube[0]);
        }
        check_error(wait(&raison), "wait");

        if (WIFEXITED(raison)) code = WEXITSTATUS(raison);
        else {
            fprintf(
                stderr,
                "Le programme a été quitté d'une autre manière qu'un exit.\n"
            );
            exit(EXIT_FAILURE);
        }

        // teste s'il y a un changment de code de retour
        if (code_change && (last_code != code || first)) {

            printf("exit %d\n", code);

            last_code = code;
            first = 0;
        }

        if (limit == 1) break;
        if (limit > 0) limit--;

        check_error(usleep(interval * 1000), "usleep");
    }

    return EXIT_SUCCESS;
}
