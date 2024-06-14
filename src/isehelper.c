#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iso9660.h"

void help();
void info();
void ls();
void cd(char* dir_name);
void tree();
void get(char* file_name);
void cat(char* file_name);
void pwd();
void quit();
void handle_command(char* command);
bool is_iso_file(const char* filename);
void print_dir(struct iso_dir* dir, int depth);
void read_block(FILE* fd, uint32_t block_num, void* buffer, size_t size);
struct iso_dir* find_dir_entry(const char* name);
void extract_file(struct iso_dir* entry, const char* filename);

struct iso_prim_voldesc* iso_descriptor = NULL;
struct iso_dir* current_dir = NULL;
char current_path[4096] = "/";
FILE* iso_fd = NULL;

#define GREEN "\033[0;32m"
#define BLUE "\033[0;34m"
#define WHITE "\033[0;37m"
#define RESET "\033[0m"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <iso_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!is_iso_file(argv[1]))
    {
        fprintf(stderr, "Fichier ISO invalide: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    current_dir = &iso_descriptor->root_dir;

    char command[4096];
    while (1)
    {
        if (isatty(STDIN_FILENO)) {
            printf(GREEN "isohelper" BLUE ": %s" WHITE " > " RESET, current_path);
        }
        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }
        handle_command(command);
    }

    if (iso_fd != NULL)
    {
        fclose(iso_fd);
    }

    free(iso_descriptor);

    return 0;
}

void handle_command(char* command)
{
    char* cmd = strtok(command, " \t\n");
    if (!cmd)
        return;

    if (strcmp(cmd, "help") == 0)
    {
        help();
    }
    else if (strcmp(cmd, "info") == 0)
    {
        info();
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        ls();
    }
    else if (strcmp(cmd, "cd") == 0)
    {
        char* arg = strtok(NULL, " \t\n");
        cd(arg);
    }
    else if (strcmp(cmd, "tree") == 0)
    {
        tree();
    }
    else if (strcmp(cmd, "get") == 0)
    {
        char* arg = strtok(NULL, " \t\n");
        get(arg);
    }
    else if (strcmp(cmd, "cat") == 0)
    {
        char* arg = strtok(NULL, " \t\n");
        cat(arg);
    }
    else if (strcmp(cmd, "pwd") == 0)
    {
        pwd();
    }
    else if (strcmp(cmd, "quit") == 0)
    {
        quit();
    }
    else
    {
        fprintf(stderr, "Commande inconnue: %s\n", cmd);
    }
}

bool is_iso_file(const char* filename)
{
    iso_fd = fopen(filename, "rb");
    if (iso_fd == NULL)
        return false;

    fseek(iso_fd, 0, SEEK_END);
    long size = ftell(iso_fd);
    fseek(iso_fd, 0, SEEK_SET);

    if (size < 16 * ISO_BLOCK_SIZE)
    {
        fclose(iso_fd);
        return false;
    }

    iso_descriptor = (struct iso_prim_voldesc*)malloc(sizeof(struct iso_prim_voldesc));
    if (!iso_descriptor)
    {
        fclose(iso_fd);
        return false;
    }

    read_block(iso_fd, ISO_PRIM_VOLDESC_BLOCK, iso_descriptor,
               sizeof(struct iso_prim_voldesc));
    const char* identifier = "CD001";
    bool valid = memcmp(iso_descriptor->std_identifier, identifier, 5) == 0;

    if (!valid)
    {
        fclose(iso_fd);
        free(iso_descriptor);
        return false;
    }

    return true;
}

void read_block(FILE* fd, uint32_t block_num, void* buffer, size_t size)
{
    fseek(fd, block_num * ISO_BLOCK_SIZE, SEEK_SET);
    fread(buffer, 1, size, fd);
}

void help(void)
{
    printf("help: afficher l'aide des commandes\n");
    printf("info: afficher les informations du volume\n");
    printf("ls: lister le contenu du répertoire\n");
    printf("cd: changer de répertoire\n");
    printf("tree: afficher l'arborescence du répertoire\n");
    printf("get: copier un fichier vers le répertoire local\n");
    printf("cat: afficher le contenu d'un fichier\n");
    printf("pwd: afficher le chemin actuel\n");
    printf("quit: quitter le programme\n");
}

void info(void)
{
    if (!iso_descriptor)
    {
        fprintf(stderr, "Erreur: descripteur ISO non chargé.\n");
        return;
    }

    char creation_date[20];
    sprintf(creation_date, "%.4s-%.2s-%.2s %.2s:%.2s:%.2s", 
             iso_descriptor->date_creat, iso_descriptor->date_creat + 4, iso_descriptor->date_creat + 6, 
             iso_descriptor->date_creat + 8, iso_descriptor->date_creat + 10, iso_descriptor->date_creat + 12);

    printf("Identifiant du système: %.32s\n", iso_descriptor->syidf);
    printf("Identifiant du volume: %.32s\n", iso_descriptor->vol_idf);
    printf("Nombre de blocs: %u\n", iso_descriptor->vol_blk_count.le);
    printf("Taille d'un bloc: %u\n", iso_descriptor->vol_blk_size.le);
    printf("Date de création: %s\n", creation_date);
    printf("Identifiant de l'application: %.128s\n", iso_descriptor->app_idf);
}

void ls(void)
{
    if (!current_dir)
    {
        fprintf(stderr, "Erreur: répertoire actuel non défini.\n");
        return;
    }

    uint32_t size = current_dir->file_size.le;
    uint8_t buffer[ISO_BLOCK_SIZE];
    read_block(iso_fd, current_dir->data_blk.le, buffer, ISO_BLOCK_SIZE);

    const uint8_t* ptr = buffer;

    while (ptr < (buffer + size))
    {
        const struct iso_dir* entry = (const struct iso_dir*)ptr;

        if (entry->dir_size == 0)
        {
            break;
        }

        char* file_identifier = malloc(entry->idf_len + 1);
        if (file_identifier == NULL)
        {
            fprintf(stderr, "[ls.c:ls] Malloc failed.\n");
            return;
        }

        strncpy(file_identifier, (char*)(entry + 1), entry->idf_len);
        file_identifier[entry->idf_len] = '\0';

        // remove ; from filename
        char* semicolon = strchr(file_identifier, ';');
        if (semicolon != NULL)
        {
            *semicolon = '\0';
        }

        char type = (entry->type & ISO_FILE_ISDIR) ? 'd' : '-';
        char hidden = (entry->type & ISO_FILE_HIDDEN) ? 'h' : '-';

        int year = entry->date[0] + 1900;
        int month = entry->date[1];
        int day = entry->date[2];
        int hour = entry->date[3];
        int minute = entry->date[4];

        printf("%c%c %9u %04d/%02d/%02d %02d:%02d %s\n", type, hidden,
               entry->file_size.le, year, month, day, hour,
               minute, file_identifier);

        free(file_identifier);
        ptr += entry->dir_size;
    }
}

void cd(char* dir_name)
{
    if (!dir_name || strcmp(dir_name, "/") == 0)
    {
        current_dir = &iso_descriptor->root_dir;
        strcpy(current_path, "/");
        return;
    }

    if (strcmp(dir_name, "..") == 0)
    {
        if (strcmp(current_path, "/") == 0)
        {
            // already at root
            return;
        }
        // Remove the last directory from the current path
        char* last_slash = strrchr(current_path, '/');
        if (last_slash)
        {
            *last_slash = '\0';
            last_slash = strrchr(current_path, '/');
            if (last_slash)
            {
                *(last_slash + 1) = '\0';
            }
            else
            {
                strcpy(current_path, "/");
            }
        }
        // Need to update current_dir to reflect the new path
        // This is a simplified version and may need adjustments for nested directories
        current_dir = &iso_descriptor->root_dir;
        char* token = strtok(current_path, "/");
        while (token)
        {
            current_dir = find_dir_entry(token);
            token = strtok(NULL, "/");
        }
        return;
    }

    struct iso_dir* entry = find_dir_entry(dir_name);
    if (entry && (entry->type & ISO_FILE_ISDIR))
    {
        current_dir = entry;
        strcat(current_path, dir_name);
        strcat(current_path, "/");
    }
    else
    {
        fprintf(stderr, "isohelper: impossible de trouver le répertoire '%s'\n",
                dir_name);
    }
}

void tree(void)
{
    if (!current_dir)
    {
        fprintf(stderr, "Erreur: répertoire actuel non défini.\n");
        return;
    }

    print_dir(current_dir, 0);
}

void get(char* file_name)
{
    if (!file_name)
    {
        fprintf(stderr, "Erreur: nom de fichier non fourni.\n");
        return;
    }

    struct iso_dir* entry = find_dir_entry(file_name);
    if (entry && !(entry->type & ISO_FILE_ISDIR))
    {
        extract_file(entry, file_name);
    }
    else
    {
        fprintf(stderr, "isohelper: impossible de trouver le fichier '%s'\n",
                file_name);
    }
}

void cat(char* file_name)
{
    if (!file_name)
    {
        fprintf(stderr, "Erreur: nom de fichier non fourni.\n");
        return;
    }

    struct iso_dir* entry = find_dir_entry(file_name);
    if (entry && !(entry->type & ISO_FILE_ISDIR))
    {
        char* buffer = (char*)malloc(entry->file_size.le);
        read_block(iso_fd, entry->data_blk.le, buffer, entry->file_size.le);
        fwrite(buffer, 1, entry->file_size.le, stdout);
        free(buffer);
    }
    else
    {
        fprintf(stderr, "isohelper: impossible de trouver le fichier '%s'\n",
                file_name);
    }
}

void pwd(void)
{
    printf("%s\n", current_path);
}

void quit(void)
{
    if (iso_fd != NULL)
    {
        fclose(iso_fd);
    }
    if (iso_descriptor)
    {
        free(iso_descriptor);
    }
    exit(0);
}

void print_dir(struct iso_dir* dir, int depth)
{
    uint8_t buffer[ISO_BLOCK_SIZE];
    read_block(iso_fd, dir->data_blk.le, buffer, ISO_BLOCK_SIZE);

    struct iso_dir* entry = (struct iso_dir*)buffer;

    while ((char*)entry < (char*)buffer + ISO_BLOCK_SIZE && entry->dir_size != 0)
    {
        if (entry->idf_len == 1 && (entry->date[0] == 0 || entry->date[0] == 1))
        {
            entry = (struct iso_dir*)((char*)entry + entry->dir_size);
            continue;
        }

        for (int i = 0; i < depth; i++)
        {
            printf("  ");
        }

        printf("%c %.*s\n", (entry->type & ISO_FILE_ISDIR) ? 'd' : '-', entry->idf_len, (char*)(entry + 1));

        if (entry->type & ISO_FILE_ISDIR)
        {
            struct iso_dir subdir;
            memcpy(&subdir, entry, sizeof(struct iso_dir));
            print_dir(&subdir, depth + 1);
        }

        entry = (struct iso_dir*)((char*)entry + entry->dir_size);
    }
}

struct iso_dir* find_dir_entry(const char* name)
{
    uint8_t buffer[ISO_BLOCK_SIZE];
    read_block(iso_fd, current_dir->data_blk.le, buffer, ISO_BLOCK_SIZE);

    struct iso_dir* entry = (struct iso_dir*)buffer;
    while ((char*)entry < (char*)buffer + ISO_BLOCK_SIZE
           && entry->dir_size != 0)
    {
        if (strncmp(name, (char*)(entry + 1), entry->idf_len) == 0)
        {
            return entry;
        }
        entry = (struct iso_dir*)((char*)entry + entry->dir_size);
    }

    return NULL;
}

void extract_file(struct iso_dir* entry, const char* filename)
{
    char* buffer = (char*)malloc(entry->file_size.le);
    read_block(iso_fd, entry->data_blk.le, buffer, entry->file_size.le);

    FILE* fd = fopen(filename, "wb");
    if (fd == NULL)
    {
        fprintf(stderr, "Erreur: impossible de créer le fichier '%s'\n",
                filename);
        free(buffer);
        return;
    }

    fwrite(buffer, 1, entry->file_size.le, fd);
    fclose(fd);
    free(buffer);
}
