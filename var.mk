# Nom de l'auteur
NAME = jean_lepeltier

# Nom du projet
NOM_PROJ = isohelper

# Dossier des fichiers à compiler en .c pour l'exécutable
SRC_DIR = src

# Nom que vous souhaitez donner à l'archive avec la commande donnée dans le README.md
ARCHIVE_NAME = isohelper_archive

# Fichiers pour l'archive targz
FILES_ARCH = $(SRC_DIR)/*.c $(SRC_DIR)/*.h README.md Makefile

# Nom de la bibliothèque
LIBRARY_NAME = isohelper

# Dossier pour afficher les comptes rendus des tests
TEST_DIR = tests

# Fichier qui permet de mettre les options que l'on souhaite dans le programme
FILE_OPTION_PROGRAMM = options
