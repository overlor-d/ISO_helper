-include Makefile.rules
-include var.mk

SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:.c=.o)

all: isohelper

isohelper: $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $(LIBRARY_NAME) $(OBJ_FILES)

test: isohelper
	@if [ -f $(TEST_DIR)/$(FILE_OPTION_PROGRAMM) ]; then \
		echo "Exécution de $(LIBRARY_NAME) avec les options du fichier '$(FILE_OPTION_PROGRAMM)'"; \
		OPTIONS=`cat $(TEST_DIR)/$(FILE_OPTION_PROGRAMM)`; \
		./$(LIBRARY_NAME) $$OPTIONS; \
	else \
		echo "Fichier '$(FILE_OPTION_PROGRAMM)' non trouvé."; \
	fi

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	@rm -rf $(OBJ_FILES) $(LIBRARY_NAME) $(LIBRARY_NAME).a $(LIBRARY_NAME).so $(SRC_DIR)/*.o

.PHONY: all isohelper test clean
