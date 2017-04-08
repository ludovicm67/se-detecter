#ifndef __DETECTER_H__
#define __DETECTER_H__

#define BUFFER_SIZE 1024

// Affiche un message sur la sortie d'erreur standard si status == value
// et quitte le programme avec un EXIT_FAILURE
#define CHECK_ERRVALUE(status, value, msg, b)           \
    if(status == value) {                               \
        fprintf(stderr, "%s\n", msg);                   \
        free_buffer(b);                                 \
        exit(EXIT_FAILURE);                             \
    }

// Affiche un message sur la sortie d'erreur standard si status == NULL,
// et quitte le programme avec un EXIT_FAILURE
#define CHECK_NULL(status, msg, b) CHECK_ERRVALUE(status, NULL, msg, b)

// Affiche un perror et quitte le programme avec un EXIT_FAILURE si status==-1
#define CHECK_ERR(status, msg, b)                       \
    if(status == -1) {                                  \
        perror(msg);                                    \
        free_buffer(b);                                 \
        exit(EXIT_FAILURE);                             \
    }

// Gère les erreurs après un exec
#define CHECK_EXEC(b)                                   \
    perror("execvp");                                   \
    free_buffer(b);                                     \
    CHECK_ERR(kill(getpid(), SIGUSR1), "kill", NULL);   \
    exit(EXIT_FAILURE)

// Structure pour un buffer chaîné
typedef struct buffer {
    char content[BUFFER_SIZE]; // contenu du buffer
    struct buffer* next; // pointeur sur le buffer suivant
    unsigned int size;
} * Buffer;

/**
 * @brief Initialise un nouveau buffer
 * @details Alloue un nouveau buffer et l'initialise
 * @return Le nouveau buffer
 */
Buffer new_buffer();

/**
 * @brief Libère la mémoire utilisée par le buffer
 * @details Libère la mémoire utilisée par une suite de buffer chaînées
 *
 * @param b Le buffer que l'on ne souhaite plus utiliser
 */
void free_buffer(Buffer b);

/**
 * @brief Affiche le contenu du buffer
 * @details Affiche le contenu stocké dans les buffer chaînés sur la sortie
 *          standard
 *
 * @param b Le buffer à afficher
 */
void print_buffer(Buffer b);

/**
 * @brief Rmplit le buffer b avec le ceontenu de fd
 * @details Lit le contenu de fd et l'inscrit dans le buffer b
 *
 * @param fd Descripteur de fichier où on doit effectuer la lecture
 * @param b Buffer dans lequel on doit écrire
 *
 * @return 1 si le contenu du buffer a changé, 0 sinon
 */
unsigned int read_buffer(int fd, Buffer b);

/**
 * @brief Affiche la manière dont on doit utiliser le programme
 * @details Affiche les différentes options et arguments pour ce programme
 *
 * @param program_name Le nom du programme
 */
void usage(char * program_name);

/**
 * @brief Affiche le temps courant
 * @details Affiche le temps actuel dans le format time_format spécifié
 *
 * @param time_format Format dans lequel on souhaite afficher le temps
 * @param b Le buffer (pour le free en cas d'erreur)
 */
void print_time(char * time_format, Buffer b);

#endif
