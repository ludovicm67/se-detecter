#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 1024

typedef struct buffer{
  char content[BUFFER_SIZE]; // contenu du buffer
  struct buffer* next; // pointeur sur le buffer suivant
  unsigned int size;
} *Buffer;

Buffer new_buffer(){
  Buffer b = malloc(sizeof(struct buffer));
  b->next = NULL;
  b->size = 0;
  return b;
}

void free_buffer(Buffer b){
  if(!b) return;
  if (b->next) free_buffer(b->next);
  free(b);
}

void print_buffer(Buffer b){
  if(!b) return;
  for(; b->next != NULL; b = b->next){
    write(1, b->content, b->size);
  }
}

int compare_buffer(Buffer b1, Buffer b2) {
  if (b1 == b2) return 1;
  if (!b1 || !b2) return 0;
  if (b1->size != b2->size) return 0;
  if (!memcmp(b1, b2, b1->size))
    return 1 && compare_buffer(b1->next, b2->next);
  else return 0;
}

void read_buffer(int fd, Buffer b){
  if (!b) return;
  Buffer bc = b;
  while((bc->size = read(fd, bc->content, BUFFER_SIZE)) > 0){
    bc->next = new_buffer();
    bc = bc->next;
  }
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
    fprintf(stderr, "Usage: %s ", program_name);
    fprintf(stderr, "[-t format][-i intervalle][-l limite][-c] ");
    fprintf(stderr, " %s prog arg...arg\n", program_name);

    exit(EXIT_FAILURE);
}


void print_time(char * time_format) {
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
        fprintf(stderr, "strftime returned 0\n");
        exit(EXIT_FAILURE);
    }
    outstr[nbc++] = '\n';
    outstr[nbc] = '\0';

    check_error(write(1, outstr, nbc), "write");
}

int main(int argc, char *argv[]) {
    int c, i, raison, nb_args, tube[2];
    unsigned int errflg = 0, first = 1, last_code = 0, code;
    char * args[argc];
    pid_t pid;
    Buffer b = malloc(sizeof(struct buffer));
    Buffer ba = malloc(sizeof(struct buffer)); // ancien buffer

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
          read_buffer(tube[0], b);
printf("\n\n\n\n\n===========\nDEBUG b=%p\n", b);
print_buffer(b);
printf("\n\n===========\nMID ba=%p\n", ba);
print_buffer(ba);
printf("\n\n\n\n\n===========\nFIN DEBUG\n\n\n\n\n\n");
          if (first || !compare_buffer(b, ba)) print_buffer(b);
          close(tube[0]);
          ba = b;

          free_buffer(b);
          b = new_buffer();
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
