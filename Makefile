-include Makefile.rules
-include var.mk

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:.c=.o)

LIBRARY_NAME = isohelper
SRC_DIR = src
TEST_DIR = tests
FILE_OPTION_PROGRAMM = options
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Wvla

all: compil test

$(LIBRARY_NAME)_lib_static: $(OBJ_FILES)
	@echo "Création de la bibliothèque statique"
	ar rcs $(LIBRARY_NAME).a $(OBJ_FILES)

$(LIBRARY_NAME)_lib_dynamic: $(OBJ_FILES)
	@echo "Création de la bibliothèque dynamique"
	$(CC) -shared -o $(LIBRARY_NAME).so $(OBJ_FILES) $(LDFLAGS)

compil: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o programme.out $(OBJ_FILES) $(LDFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

check: compil
	@if [ -f option ]; then \
		echo "Exécution de programme.out avec les options du fichier 'option'"; \
		OPTIONS=`cat option`; \
		./programme.out $$OPTIONS; \
	else \
		echo "Fichier 'option' non trouvé."; \
	fi

dev:
	@gnome-terminal -- bash -c 'while true; do \
        $(MAKE) -s compil > ./$(TEST_DIR)/compile_output.txt 2>&1; \
        ERR=$$?; \
		clear; \
        if [ $$ERR -ne 0 ]; then \
            FILE_CONTENT=$$(cat ./$(TEST_DIR)/compile_output.txt); \
			echo "$$FILE_CONTENT"; \
        else \
			OPTIONS=$$(cat $(TEST_DIR)/$(FILE_OPTION_PROGRAMM)); \
            ./programme.out $$OPTIONS; \
			ERR=$$?; \
			echo "---------------------------------------"; \
			echo "Retour du programme : $$ERR"; \
         fi; \
        sleep 2; \
    done & \
    read -p ""; \
    kill $$!; \
	$(MAKE) -s clean'

clean:
	@rm -rf $(OBJ_FILES) $(LIBRARY_NAME).a $(LIBRARY_NAME).so programme.out
