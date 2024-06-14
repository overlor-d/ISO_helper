# ISO Helper

ISO Helper est un programme en C pour manipuler les fichiers ISO 9660. Il permet de lister, extraire et afficher le contenu des fichiers ISO.

## Prérequis

Avant de commencer, assurez-vous d'avoir les outils suivants installés sur votre machine :

- GCC (GNU Compiler Collection)
- Make

## Configuration

Pour initialiser les règles du Makefile, exécutez la commande suivante :

```sh
./configure
```

## Compilation

Pour compiler le projet, utilisez la commande suivante :

```sh
make
```

## Exécution

Après la compilation, vous pouvez exécuter le programme avec :

```sh
./isohelper <chemin_vers_fichier_iso>
```

## Utilisation

Le programme prend en charge plusieurs commandes interactives :

- `help` : Affiche l'aide des commandes
- `info` : Affiche les informations du volume
- `ls` : Liste le contenu du répertoire
- `cd <répertoire>` : Change de répertoire
- `tree` : Affiche l'arborescence du répertoire
- `get <fichier>` : Copie un fichier vers le répertoire local
- `cat <fichier>` : Affiche le contenu d'un fichier
- `pwd` : Affiche le chemin actuel
- `quit` : Quitte le programme

## Tests

Pour exécuter la suite de tests avec le binaire `isohelper`, utilisez la commande suivante :

```sh
make test
```

## Nettoyage

Pour supprimer tous les fichiers produits par `make`, utilisez la commande suivante :

```sh
make clean
```

## Structure du Projet

- `src/` : Contient les fichiers source `.c` du projet.
- `tests/` : Contient les fichiers de tests et les options pour les tests.
- `Makefile` : Le fichier Makefile pour gérer la compilation, les tests et le nettoyage.

## Exemples de Commandes

### Lister le contenu d'un fichier ISO

```sh
./isohelper mon_fichier.iso
isohelper: / > ls
```

### Changer de répertoire

```sh
isohelper: / > cd PICS
isohelper: /PICS/ > ls
```

### Afficher le contenu d'un fichier

```sh
isohelper: /PICS/ > cat README.TXT
```

## Fonctionnalités Implémentées

- Initialisation du projet avec `./configure`
- Compilation avec `make`
- Exécution de commandes interactives pour manipuler le contenu des fichiers ISO
- Tests automatisés
- Nettoyage des fichiers générés

## Fonctionnalités Autorisées et En-têtes Utilisés

Le programme utilise uniquement les fonctions et en-têtes autorisés :

### Fonctions Autorisées

- `open(2)`
- `close(2)`
- `fstat(3)`
- `isatty(3)`
- `exit(3)`
- `on_exit(3)`
- `atexit(3)`
- `free(3)`

### En-têtes Utilisés

- `<sys/mman.h>`
- `<string.h>`
- `<stdio.h>`
- `<err.h>`
- `<stdbool.h>`
- `<stdint.h>`
- `<sys/types.h>`
- `<ctype.h>`
- `<unistd.h>`


---

N'hésitez pas à consulter les fichiers source pour plus de détails sur l'implémentation.

## Contributeurs

- Jean Lepeltier

Merci d'avoir utilisé ISO Helper !
