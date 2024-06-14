#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "iso9660.h"

struct iso_prim_voldesc *descripteur_iso = NULL;
struct iso_dir *repertoire_actuel = NULL;
char pwd[4096] = "/";
int fichier_iso_fd = -1;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <fichier_iso>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!verifier_fichier_iso(argv[1]))
    {
        fprintf(stderr, "Fichier ISO invalide: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    repertoire_actuel = &descripteur_iso->root_dir;

    char commande[4096];
    while (1)
    {
        if (isatty(STDIN_FILENO))
        {
            printf("\033[0;32misohelper\033[0;34m: %s\033[0;37m > \033[0m",
                   pwd);
        }
        if (fgets(commande, sizeof(commande), stdin) == NULL)
        {
            break;
        }
        traiter_commande(commande);
    }

    if (fichier_iso_fd != -1)
    {
        close(fichier_iso_fd);
    }

    free(descripteur_iso);

    return 0;
}

void traiter_commande(char *commande)
{
    char *cmd = strtok(commande, " \t\n");
    if (!cmd)
        return;

    if (strcmp(cmd, "help") == 0)
    {
        afficher_help();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        afficher_info();
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        lister_repertoire();
    }
    else if (strcmp(cmd, "cd") == 0)
    {
        char *arg = strtok(NULL, " \t\n");
        changer_repertoire(arg);
    }
    else if (strcmp(cmd, "tree") == 0)
    {
        afficher_tree();
    }
    else if (strcmp(cmd, "get") == 0)
    {
        char *arg = strtok(NULL, " \t\n");
        get_fichier(arg);
    }
    else if (strcmp(cmd, "cat") == 0)
    {
        char *arg = strtok(NULL, " \t\n");
        cat(arg);
    }
    else if (strcmp(cmd, "pwd") == 0)
    {
        afficher_pwd();
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        quit_programme();
    }
    else
    {
        fprintf(stderr, "Commande inconnue: %s\n", cmd);
    }
}

bool verifier_fichier_iso(const char *nom_fichier)
{
    fichier_iso_fd = open(nom_fichier, O_RDONLY);
    if (fichier_iso_fd < 0)
        return false;

    struct stat st;
    if (fstat(fichier_iso_fd, &st) < 0)
    {
        close(fichier_iso_fd);
        return false;
    }

    if (st.st_size < 16 * ISO_BLOCK_SIZE)
    {
        close(fichier_iso_fd);
        return false;
    }

    descripteur_iso = malloc(sizeof(struct iso_prim_voldesc));
    if (!descripteur_iso)
    {
        close(fichier_iso_fd);
        return false;
    }

    lire_bloc(fichier_iso_fd, ISO_PRIM_VOLDESC_BLOCK, descripteur_iso,
              sizeof(struct iso_prim_voldesc));
    const char *identifier = "CD001";
    bool valide = memcmp(descripteur_iso->std_identifier, identifier, 5) == 0;

    if (!valide)
    {
        close(fichier_iso_fd);
        free(descripteur_iso);
        return false;
    }

    return true;
}

void lire_bloc(int fd, uint32_t num_bloc, void *buffer, size_t taille)
{
    lseek(fd, num_bloc * ISO_BLOCK_SIZE, SEEK_SET);
    read(fd, buffer, taille);
}

void afficher_help(void)
{
    printf("help: afficher l'help des commandes\n");
    printf("info: afficher les informations du volume\n");
    printf("ls: lister le cat du répertoire\n");
    printf("cd: changer de répertoire\n");
    printf("tree: afficher l'tree du répertoire\n");
    printf("get: copier un fichier vers le répertoire local\n");
    printf("cat: afficher le cat d'un fichier\n");
    printf("pwd: afficher le pwd actuel\n");
    printf("quit: quit le programme\n");
}

void afficher_info(void)
{
    if (!descripteur_iso)
    {
        fprintf(stderr, "Erreur: descripteur ISO non chargé.\n");
        return;
    }

    char date_creation[20];
    snprintf(date_creation, sizeof(date_creation), "%.4s-%.2s-%.2s %.2s:%.2s",
             descripteur_iso->date_creat, descripteur_iso->date_creat + 4,
             descripteur_iso->date_creat + 6, descripteur_iso->date_creat + 8,
             descripteur_iso->date_creat + 10);

    printf("Identifiant du système: %.32s\n", descripteur_iso->syidf);
    printf("Identifiant du volume: %.32s\n", descripteur_iso->vol_idf);
    printf("Nombre de blocs: %u\n", descripteur_iso->vol_blk_count.le);
    printf("Taille d'un bloc: %u\n", descripteur_iso->vol_blk_size.le);
    printf("Date de création: %s\n", date_creation);
    printf("Identifiant de l'application: %.128s\n", descripteur_iso->app_idf);
}

void lister_repertoire(void)
{
    if (!repertoire_actuel)
    {
        fprintf(stderr, "Erreur: répertoire actuel non défini.\n");
        return;
    }

    uint32_t taille = repertoire_actuel->file_size.le;
    uint8_t buffer[ISO_BLOCK_SIZE];
    lire_bloc(fichier_iso_fd, repertoire_actuel->data_blk.le, buffer,
              ISO_BLOCK_SIZE);

    const uint8_t *ptr = buffer;

    while ((ptr - buffer) < taille)
    {
        const struct iso_dir *entree = (const struct iso_dir *)ptr;

        if (entree->dir_size == 0)
        {
            break;
        }

        char *nom_fichier = malloc(entree->idf_len + 1);
        if (nom_fichier == NULL)
        {
            fprintf(stderr, "Erreur: échec de l'allocation mémoire.\n");
            return;
        }

        strncpy(nom_fichier, (char *)(entree + 1), entree->idf_len);
        nom_fichier[entree->idf_len] = '\0';

        char *point_virgule = strchr(nom_fichier, ';');
        if (point_virgule != NULL)
        {
            *point_virgule = '\0';
        }

        char type = (entree->type & ISO_FILE_ISDIR) ? 'd' : '-';
        char cache = (entree->type & ISO_FILE_HIDDEN) ? 'h' : '-';

        int annee = entree->date[0] + 1900;
        int mois = entree->date[1];
        int jour = entree->date[2];
        int heure = entree->date[3];
        int minute = entree->date[4];

        if (entree->idf_len == 1 && *((char *)(entree + 1)) == 0x00)
        {
            printf("%c%c %9u %04d/%02d/%02d %02d:%02d .\n", type, cache,
                   entree->file_size.le, annee, mois, jour, heure, minute);
        }
        else if (entree->idf_len == 1 && *((char *)(entree + 1)) == 0x01)
        {
            printf("%c%c %9u %04d/%02d/%02d %02d:%02d ..\n", type, cache,
                   entree->file_size.le, annee, mois, jour, heure, minute);
        }
        else
        {
            printf("%c%c %9u %04d/%02d/%02d %02d:%02d %s\n", type, cache,
                   entree->file_size.le, annee, mois, jour, heure, minute,
                   nom_fichier);
        }

        free(nom_fichier);
        ptr += entree->dir_size;
    }
}

void changer_repertoire(char *nom_repertoire)
{
    if (!nom_repertoire)
    {
        repertoire_actuel = &descripteur_iso->root_dir;
        strcpy(pwd, "/");
        return;
    }

    if (strcmp(nom_repertoire, "..") == 0)
    {
        if (strcmp(pwd, "/") == 0)
        {
            return;
        }

        char *last_slash = strrchr(pwd, '/');
        if (last_slash != NULL && last_slash != pwd)
        {
            *last_slash = '\0';
        }
        else
        {
            strcpy(pwd, "/");
        }

        uint8_t buffer[ISO_BLOCK_SIZE];
        lire_bloc(fichier_iso_fd, descripteur_iso->le_path_table_blk, buffer, ISO_BLOCK_SIZE);

        struct iso_dir *parent_repertoire = NULL;
        for (struct iso_path_table_le *entry = (struct iso_path_table_le *)buffer;
             (char *)entry < (char *)buffer + ISO_BLOCK_SIZE && entry->idf_len != 0;
             entry = (struct iso_path_table_le *)((char *)entry + 8 + entry->idf_len + (entry->idf_len % 2 == 0 ? 0 : 1)))
        {
            if (entry->parent_dir == repertoire_actuel->vol_seq.le)
            {
                parent_repertoire = (struct iso_dir *)((uint8_t *)descripteur_iso + entry->data_blk * ISO_BLOCK_SIZE);
                break;
            }
        }

        if (parent_repertoire)
        {
            repertoire_actuel = parent_repertoire;
        }
        else
        {
            fprintf(stderr, "isohelper: impossible de trouver le répertoire parent\n");
        }

        return;
    }

    struct iso_dir *entree = trouver_entree_repertoire(nom_repertoire);
    if (entree && (entree->type & ISO_FILE_ISDIR))
    {
        repertoire_actuel = entree;
        if (strcmp(pwd, "/") != 0)
        {
            strcat(pwd, "/");
        }
        strcat(pwd, nom_repertoire);
    }
    else
    {
        fprintf(stderr, "isohelper: impossible de trouver le répertoire '%s'\n", nom_repertoire);
    }
}


void afficher_arborescence_recurse(struct iso_dir *dir, int profondeur,
                                   const char *prefixe)
{
    uint8_t buffer[ISO_BLOCK_SIZE];
    lire_bloc(fichier_iso_fd, dir->data_blk.le, buffer, ISO_BLOCK_SIZE);

    struct iso_dir *entree = (struct iso_dir *)buffer;
    while ((char *)entree < (char *)buffer + ISO_BLOCK_SIZE
           && entree->dir_size != 0)
    {
        if (entree->idf_len == 1
            && (*(char *)(entree + 1) == 0x00 || *(char *)(entree + 1) == 0x01))
        {
            entree = (struct iso_dir *)((char *)entree + entree->dir_size);
            continue;
        }

        char *nom_fichier = malloc(entree->idf_len + 1);
        if (nom_fichier == NULL)
        {
            fprintf(stderr, "Erreur: échec de l'allocation mémoire.\n");
            return;
        }

        strncpy(nom_fichier, (char *)(entree + 1), entree->idf_len);
        nom_fichier[entree->idf_len] = '\0';

        char *point_virgule = strchr(nom_fichier, ';');
        if (point_virgule != NULL)
        {
            *point_virgule = '\0';
        }

        printf("%s", prefixe);
        if ((char *)entree + entree->dir_size
            >= (char *)buffer + ISO_BLOCK_SIZE)
        {
            printf("└── %s%s\n", (entree->type & ISO_FILE_ISDIR) ? "/" : "",
                   nom_fichier);
        }
        else
        {
            printf("├── %s%s\n", (entree->type & ISO_FILE_ISDIR) ? "/" : "",
                   nom_fichier);
        }

        if (entree->type & ISO_FILE_ISDIR)
        {
            char nouveau_prefixe[4096];
            snprintf(nouveau_prefixe, sizeof(nouveau_prefixe), "%s%s", prefixe,
                     (char *)entree + entree->dir_size
                             >= (char *)buffer + ISO_BLOCK_SIZE
                         ? "    "
                         : "│   ");
            struct iso_dir sous_repertoire;
            memcpy(&sous_repertoire, entree, sizeof(struct iso_dir));
            afficher_arborescence_recurse(&sous_repertoire, profondeur + 1,
                                          nouveau_prefixe);
        }

        free(nom_fichier);
        entree = (struct iso_dir *)((char *)entree + entree->dir_size);
    }
}

void afficher_tree(void)
{
    if (!repertoire_actuel)
    {
        fprintf(stderr, "Erreur: répertoire actuel non défini.\n");
        return;
    }

    printf(".\n");
    afficher_arborescence_recurse(repertoire_actuel, 1, "");
}

void get_fichier(char *nom_fichier)
{
    if (!nom_fichier)
    {
        fprintf(stderr, "Erreur: nom de fichier non fourni.\n");
        return;
    }

    char *base_nom = strrchr(nom_fichier, '/');
    if (base_nom)
    {
        base_nom++;
    }
    else
    {
        base_nom = nom_fichier;
    }

    struct iso_dir *entree = trouver_entree_repertoire(base_nom);
    if (!entree)
    {
        char *nom_fichier_avec_version = malloc(strlen(base_nom) + 3);
        if (nom_fichier_avec_version)
        {
            snprintf(nom_fichier_avec_version, strlen(base_nom) + 3, "%s;1",
                     base_nom);
            entree = trouver_entree_repertoire(nom_fichier_avec_version);
            free(nom_fichier_avec_version);
        }
    }

    if (entree && !(entree->type & ISO_FILE_ISDIR))
    {
        char *buffer = (char *)malloc(entree->file_size.le);
        lire_bloc(fichier_iso_fd, entree->data_blk.le, buffer,
                  entree->file_size.le);

        FILE *fd = fopen(base_nom, "wb");
        if (fd == NULL)
        {
            fprintf(stderr, "Erreur: impossible de créer le fichier '%s'\n",
                    base_nom);
            free(buffer);
            return;
        }

        fwrite(buffer, 1, entree->file_size.le, fd);
        fclose(fd);
        free(buffer);
    }
    else
    {
        fprintf(stderr, "isohelper: impossible de trouver le fichier '%s'\n",
                nom_fichier);
    }
}

void cat(char *nom_fichier)
{
    if (!nom_fichier)
    {
        fprintf(stderr, "Erreur: nom de fichier non fourni.\n");
        return;
    }

    struct iso_dir *entree = trouver_entree_repertoire(nom_fichier);
    if (!entree)
    {
        char *nom_fichier_avec_version = malloc(strlen(nom_fichier) + 3);
        if (nom_fichier_avec_version)
        {
            snprintf(nom_fichier_avec_version, strlen(nom_fichier) + 3, "%s;1",
                     nom_fichier);
            entree = trouver_entree_repertoire(nom_fichier_avec_version);
            free(nom_fichier_avec_version);
        }
    }

    if (entree && !(entree->type & ISO_FILE_ISDIR))
    {
        char *buffer = (char *)malloc(entree->file_size.le);
        lire_bloc(fichier_iso_fd, entree->data_blk.le, buffer,
                  entree->file_size.le);
        fwrite(buffer, 1, entree->file_size.le, stdout);
        free(buffer);
    }
    else
    {
        fprintf(stderr, "isohelper: impossible de trouver le fichier '%s'\n",
                nom_fichier);
    }
}

void afficher_pwd(void)
{
    printf("%s\n", pwd);
}

void quit_programme(void)
{
    if (fichier_iso_fd != -1)
    {
        close(fichier_iso_fd);
    }
    if (descripteur_iso)
    {
        free(descripteur_iso);
    }
    exit(0);
}

struct iso_dir *trouver_entree_repertoire(const char *nom)
{
    uint8_t buffer[ISO_BLOCK_SIZE];
    lire_bloc(fichier_iso_fd, repertoire_actuel->data_blk.le, buffer,
              ISO_BLOCK_SIZE);

    struct iso_dir *entree = (struct iso_dir *)buffer;
    while ((char *)entree < (char *)buffer + ISO_BLOCK_SIZE
           && entree->dir_size != 0)
    {
        char *nom_fichier = malloc(entree->idf_len + 1);
        if (!nom_fichier)
        {
            fprintf(stderr, "Erreur: échec de l'allocation mémoire.\n");
            return NULL;
        }
        strncpy(nom_fichier, (char *)(entree + 1), entree->idf_len);
        nom_fichier[entree->idf_len] = '\0';

        char *point_virgule = strchr(nom_fichier, ';');
        if (point_virgule)
        {
            *point_virgule = '\0';
        }

        if (strcmp(nom, nom_fichier) == 0)
        {
            free(nom_fichier);
            return entree;
        }

        free(nom_fichier);
        entree = (struct iso_dir *)((char *)entree + entree->dir_size);
    }

    return NULL;
}
