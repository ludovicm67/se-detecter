#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include "detecter.h"

// Initialise un nouveau buffer
Buffer new_buffer() {
    Buffer b = malloc(sizeof(struct buffer));
    CHECK_NULL(b, "buffer non alloué", b);
    b->next = NULL;
    b->size = 0;
    return b;
}

// Libère la mémoire utilisée par le buffer chaîné
void free_buffer(Buffer b) {
    if(!b) return;
    if(b->next) free_buffer(b->next);
    free(b);
}

// Affiche le contenu stocké dans les buffer chaînés
void print_buffer(Buffer b) {
    if(!b) return;
    for(; b->next != NULL; b = b->next) {
        CHECK_ERR(write(1, b->content, b->size), "write buffer", b);
    }
}

// Lit le contenu de fd et l'inscrit dans le buffer b
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

// Affiche la manière dont on doit utiliser le programme
void usage(char * program_name) {
    fprintf(stderr, "Usage: %s ", program_name);
    fprintf(stderr, "[-t format][-i intervalle][-l limite][-c] ");
    fprintf(stderr, " %s prog arg...arg\n", program_name);

    exit(EXIT_FAILURE);
}

// Affiche le temps actuel dans le format time_format spécifié
void print_time(char * time_format, Buffer b) {
    char outstr[200];
    struct tm *tmp;
    time_t t;
    unsigned int nbc;

    t = time(NULL);
    tmp = localtime(&t);

    CHECK_NULL(tmp, "localtime a une valeur égale à NULL", b);

    CHECK_ERRVALUE(
        (nbc = strftime(outstr, sizeof(outstr)-1, time_format, tmp)),
        0,
        "strftime vaut 0 : le format pour le temps est-il bien renseigné ?", b
    );

    outstr[nbc++] = '\n';
    outstr[nbc] = '\0';

    CHECK_ERR(write(1, outstr, nbc), "write", b);
}

int main(int argc, char *argv[]) {
    int c, i, raison, nb_args, tube[2];
    unsigned int errflg = 0, first = 1, last_code = 0, code, affiche;
    char * args[argc];
    pid_t pid;
    Buffer b;

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

    b = new_buffer();

    while(limit >= 0) {
        if(time_format) print_time(time_format, b);
        CHECK_ERR(pipe(tube), "pipe", b);
        CHECK_ERR((pid = fork()), "fork", b);
        if(!pid) { // fils
            CHECK_ERR(close(tube[0]), "close tube[0] (fils)", b);
            CHECK_ERR(dup2(tube[1], 1), "dup2 (fils)", b);
            CHECK_ERR(close(tube[1]), "close tube[1] (fils)", b);
            execvp(argv[optind], args);
            CHECK_EXEC(b);
        } else { // père
            CHECK_ERR(close(tube[1]), "close tube[1] (parent)", b);
            affiche = read_buffer(tube[0], b);
            if(first || affiche) print_buffer(b);
            CHECK_ERR(close(tube[0]), "close tube[0] (parent)", b);
        }
        CHECK_ERR(wait(&raison), "wait", b);

        if(WIFEXITED(raison)) code = WEXITSTATUS(raison);
        else {
            fprintf(
                stderr,
                "Le programme a été quitté d'une autre manière qu'un exit.\n"
            );
            free_buffer(b);
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

        CHECK_ERR(usleep(interval * 1000), "usleep", b);
    }

    free_buffer(b);

    return EXIT_SUCCESS;
}
