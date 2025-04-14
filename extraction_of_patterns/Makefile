# Nom de l'exécutable final
TARGET = programme

# Compilateur C et C++ et options de compilation
CC = gcc
CXX = g++
CFLAGS = -Wall -g
CXXFLAGS = -std=c++17 -Wall -g

# Fichiers sources et objets
C_SRCS = stree_strmat.c stree_ukkonen.c repeats_supermax.c repeats_primitives.c
CPP_SRCS = main12345.cpp
OBJS = stree_strmat.o stree_ukkonen.o repeats_supermax.o repeats_primitives.o main12345.o

# Règle pour générer l'exécutable final
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET)

# Règles pour compiler les fichiers sources en objets
stree_strmat.o: stree_strmat.c
	$(CC) $(CFLAGS) -c stree_strmat.c -o stree_strmat.o

stree_ukkonen.o: stree_ukkonen.c
	$(CC) $(CFLAGS) -c stree_ukkonen.c -o stree_ukkonen.o

repeats_supermax.o: repeats_supermax.c
	$(CC) $(CFLAGS) -c repeats_supermax.c -o repeats_supermax.o

repeats_primitives.o: repeats_primitives.c
	$(CC) $(CFLAGS) -c repeats_primitives.c -o repeats_primitives.o

main12345.o: main12345.cpp
	$(CXX) $(CXXFLAGS) -c main12345.cpp -o main12345.o

# Règle de nettoyage pour supprimer les fichiers intermédiaires et l'exécutable
clean:
	rm -f $(OBJS) $(TARGET) mapFn.json
	rm -rf results
	
# Supprimer tous les fichiers objets seulement
clean_obj:
	rm -f *.o