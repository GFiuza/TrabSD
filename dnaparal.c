#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <mpi.h>

// MAX char table (ASCII)
#define MAX 256
#define INFINITO INT_MAX
#define MESTRE 0

//O programa envia a substring com o tamanho da divisão entre string/processos-1 somado com tamanho da Query - 1. O programa tá funcionando para até 16 processos mas eu não estou conseguindo entender direito porque não funciona para 32 processos porque a quantidade de caracteres da substring de cada processos vai ter o tamQuery-1 + string/processos-1. Com 128 processos faz sentido não funcionar.

// Boyers-Moore-Hospool-Sunday algorithm for string matching
int bmhs(char *string, int n, char *substr, int m) {

	int d[MAX];
	int i, j, k;

	// pre-processing
	for (j = 0; j < MAX; j++)
		d[j] = m + 1;
	for (j = 0; j < m; j++)
		d[(int) substr[j]] = m - j;

	// searching
	i = m - 1;
	while (i < n) {
		k = i;
		j = m - 1;
		while ((j >= 0) && (string[k] == substr[j])) {
			j--;
			k--;
		}
		if (j < 0)
			return k + 1;
		i = i + d[(int) string[i + 1]];
	}

	return INFINITO;
}



FILE* openfile(char* string, char* type){
	FILE *arq = fopen(string, type);
	if (arq == NULL) {
		perror(string);
		exit(EXIT_FAILURE);
	}
	return arq;
}

void closefile(FILE* arq) {
	fflush(arq);
	fclose(arq);
}

void remove_eol(char *line) {
	int i = strlen(line) - 1;
	while (line[i] == '\n' || line[i] == '\r') {
		line[i] = 0;
		i--;
	}
}



int main(int argc, char* argv[]) {
	int np,tamBase, tamQuery, meu_rank, continua, resultp, result;
	char *base;
	char *query;

	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &meu_rank);  
	MPI_Comm_size(MPI_COMM_WORLD,&np); 

	base = (char*) malloc(sizeof(char) * 1000001);
	if (base == NULL) {
		perror("malloc base");
		exit(EXIT_FAILURE);
	}
	query = (char*) malloc(sizeof(char) * 1000001);
	if (query == NULL) {
		perror("malloc query");
		exit(EXIT_FAILURE);
	}

	if (meu_rank == MESTRE) {
		FILE *fdatabase, *fquery, *fout;
		fdatabase = openfile("dna.in", "r");
		fquery = openfile("query.in", "r");
		fout = openfile("dna.out", "w");

		char desc_dna[100], desc_query[100];
		char line[100];
		int i, found;

		fgets(desc_query, 80, fquery);
		remove_eol(desc_query);

		while (!feof(fquery)) {

			fprintf(fout, "%s\n", desc_query);
			fgets(line, 100, fquery);
			remove_eol(line);
			query[0] = 0;
			i = 0;
			do {
				strcat(query + i, line);
				if (fgets(line, 100, fquery) == NULL)
					break;
				remove_eol(line);
				i += 80;
			} while (line[0] != '>');
			strcpy(desc_query, line);

		
			// read database and search
			found = 0;
			fseek(fdatabase, 0, SEEK_SET);
			fgets(line, 100, fdatabase);
			remove_eol(line);
			while (!feof(fdatabase)) {
				strcpy(desc_dna, line);
				base[0] = 0;
				i = 0;
				fgets(line, 100, fdatabase);
				remove_eol(line);
				do {
					strcat(base + i, line);
					if (fgets(line, 100, fdatabase) == NULL)
						break;
					remove_eol(line);
					i += 80;
				} while (line[0] != '>');
				
				tamQuery = strlen(query);
				tamBase = (strlen(base)/(np-1)) + tamQuery-1;
				continua = 1;
				MPI_Bcast(&continua, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);
				MPI_Bcast(&tamQuery, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);
				MPI_Bcast(&tamBase, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);
				MPI_Bcast(query, tamQuery, MPI_CHAR, MESTRE, MPI_COMM_WORLD);

				for (int j = 1; j < np; j++)
					MPI_Send(base + ((j-1)*tamBase), tamBase + tamQuery - 1, MPI_CHAR, j, 1, MPI_COMM_WORLD);
				

				resultp = INFINITO;
				MPI_Reduce(&resultp, &result, 1, MPI_INT, MPI_MIN, MESTRE, MPI_COMM_WORLD);

				if (result != INFINITO) {
					fprintf(fout, "%s\n%d\n", desc_dna, result);
					found = 1;
				}
				memset(base, '\0', strlen(base));
			}

			if (!found)
				fprintf(fout, "NOT FOUND\n");

			memset(query, '\0', strlen(query));
		}
		continua = 0;
		MPI_Bcast(&continua, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);

	} else {
		while(1) {
			MPI_Bcast(&continua, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);
			if (!continua)
				break;
			MPI_Bcast(&tamQuery, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);
			MPI_Bcast(&tamBase, 1, MPI_INT, MESTRE, MPI_COMM_WORLD);
			MPI_Bcast(query, tamQuery, MPI_CHAR, MESTRE, MPI_COMM_WORLD);
			MPI_Recv(base, tamBase + tamQuery - 1, MPI_CHAR, MESTRE, 1, MPI_COMM_WORLD, &status);
			resultp = bmhs(base, tamBase + tamQuery - 1, query, tamQuery);
			if(resultp!=INFINITO)			
				resultp += (meu_rank-1)*tamBase;
			MPI_Reduce(&resultp, &result, 1, MPI_INT, MPI_MIN, MESTRE, MPI_COMM_WORLD);
			memset(query, '\0', strlen(query));
			memset(base, '\0', strlen(base));
		}
	}

	free(base);
	free(query);
	MPI_Finalize();
}
