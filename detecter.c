#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define BUFFER_SIZE 1024

// Affiche un message sur la sortie d'erreur standard si status == value
// et quitte le programme avec un EXIT_FAILURE
#define CHECK_ERRVALUE(status, value, msg)  \
    if(status == value) {                   \
        fprintf(stderr, "%s\n", msg);       \
        exit(EXIT_FAILURE);                 \
    }

// Affiche un message sur la sortie d'erreur standard si status == NULL,
// et quitte le programme avec un EXIT_FAILURE
#define CHECK_NULL(status, msg) CHECK_ERRVALUE(status, NULL, msg)

// Affiche un perror et quitte le programme avec un EXIT_FAILURE si status==-1
#define CHECK_ERR(status, msg)              \
    if(status == -1) {                      \
        perror(msg);                        \
        exit(EXIT_FAILURE);                 \
    }

// Permet de gérer les erreurs après un exec
#define CHECK_EXEC()                        \
    perror("execvp");                       \
    kill(getpid(), SIGUSR1);                \
    exit(EXIT_FAILURE)


// Structure pour un buffer chaîné
typedef struct buffer {
  char content[BUFFER_SIZE]; // contenu du buffer
  struct buffer* next; // pointeur sur le buffer suivant
  unsigned int size;
} *Buffer;

// Permet de
Buffer new_buffer() {
  Buffer b = malloc(sizeof(struct buffer));
  CHECK_NULL(b, "buffer non alloué");
  b->next = NULL;
  b->size = 0;
  return b;
}

void free_buffer(Buffer b) {
  if(!b) return;
  if(b->next) free_buffer(b->next);
  free(b);
}

void print_buffer(Buffer b){
  if(!b) return;
  for(; b->next != NULL; b = b->next){
    write(1, b->content, b->size);
  }
}

unsigned int read_buffer(int fd, Buffer b) {
    unsigned int has_changed = 0, size;
    char tmp[BUFFER_SIZE];
    while((size = read(fd, tmp, BUFFER_SIZE)) > 0) {
        if(size != b->size || memcmp(tmp, b->content, size) != 0) {
            has_changed = 1;
            memcpy(b->content, tmp, size);
            b->size = size;
        }
        if(!b->next) {
            b->next = new_buffer();
        }
        b = b->next;
    }
    if (b && b->next) free_buffer(b->next);
    return has_changed;
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

    CHECK_NULL(tmp, "localtime a une valeur NULL");

    CHECK_ERRVALUE(
        (nbc = strftime(outstr, sizeof(outstr)-1, time_format, tmp)),
        0, "strftime a retourné 0"
    );

    outstr[nbc++] = '\n';
    outstr[nbc] = '\0';

    CHECK_ERR(write(1, outstr, nbc), "write");
}

int main(int argc, char *argv[]) {
    int c, i, raison, nb_args, tube[2];
    unsigned int errflg = 0, first = 1, last_code = 0, code, affiche;
    char * args[argc];
    pid_t pid;
    Buffer b = new_buffer();

    char * time_format = NULL; // option -t
    int interval = 10000; // option -i
    int limit = 0; // option -l
    int code_change = 0; // option -c

    while((c = getopt(argc, argv, "+t:i:l:c")) != EOF) {
        switch(c) {
            case 't':
                time_format = optarg;
                break;
            case 'i':
                if((interval = atoi(optarg)) <= 0) {
                    fprintf(stderr, "-i doit être supérieur à 0\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'l':
                if((limit = atoi(optarg)) < 0) {
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
    if(errflg) usage(argv[0]);
    nb_args = argc - optind;
    if(nb_args <= 0) usage(argv[0]);

    // On initialise le tableau d'arguments pour le exec
    for(i = optind; i < argc; i++) args[i-optind] = argv[i];
    args[nb_args] = NULL;

    while(limit >= 0) {
        if(time_format) print_time(time_format);
        CHECK_ERR(pipe(tube), "pipe");
        CHECK_ERR((pid = fork()), "fork");
        if(!pid) { // fils
            close(tube[0]);
            dup2(tube[1], 1);
            close(tube[1]);
            execvp(argv[optind], args);
            CHECK_EXEC();
        } else { // père
            close(tube[1]);
            affiche = read_buffer(tube[0], b);
            if(first || affiche) print_buffer(b);
            close(tube[0]);
        }
        CHECK_ERR(wait(&raison), "wait");

        if(WIFEXITED(raison)) code = WEXITSTATUS(raison);
        else {
            fprintf(
                stderr,
                "Le programme a été quitté d'une autre manière qu'un exit.\n"
            );
            exit(EXIT_FAILURE);
        }

        // teste s'il y a un changment de code de retour
        if(code_change && (last_code != code || first)) {
            printf("exit %d\n", code);
            fflush(stdout);
            last_code = code;
        }


        if(first) first = 0;
        if(limit == 1) break;
        if(limit > 0) limit--;

        CHECK_ERR(usleep(interval * 1000), "usleep");
    }

    free_buffer(b);

    return EXIT_SUCCESS;
}
