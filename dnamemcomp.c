#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>

// MAX char table (ASCII)
#define MAX 256
#define INFINITO INT_MAX

int minimo (int a, int b){
	if(a < b)
		return a;
	return b;
}

// Boyers-Moore-Hospool-Sunday algorithm for string matching
int bmhs(char *string, int n, char *substr, int m, int num_threads) {

	int d[MAX];
	int adicionar;
	int retorno = INFINITO;
	// pre-processing
	for (int a = 0; a < MAX; a++)
		d[a] = m + 1;
	for (int a = 0; a < m; a++)
		d[(int) substr[a]] = m - a;

	omp_set_num_threads(num_threads);
	// searching
	#pragma omp parallel reduction(min:retorno)
	{
		int id = omp_get_thread_num();
		int nthreads = omp_get_num_threads();
		int i, j, k;
		i = (m - 1) + (id * n/nthreads);
		while (i < (n/nthreads*(id+1))) {
				k = i;
				j = m - 1;
				while ((j >= 0) && (string[k] == substr[j])) {
					j--;
					k--;
				}
				if (j < 0){
					#pragma parallel critical
					retorno = k + 1;
					i = n;
				}
				else
					i = i + d[(int) string[i + 1]];
		}
	}
	if(retorno != INFINITO)
		return retorno;
	return -1;
}

FILE* openfile(char* nome, char* modo) {
	FILE *arquivo;
	arquivo = fopen(nome, modo);
	if (arquivo == NULL) {
		perror(nome);
		exit(EXIT_FAILURE);
	}
}

void closefiles(FILE* arquivo) {
	fflush(arquivo);
	fclose(arquivo);
}

void remove_eol(char *line) {
	int i = strlen(line) - 1;
	while (line[i] == '\n' || line[i] == '\r') {
		line[i] = 0;
		i--;
	}
}

int main(int argc, char* argv) {

	FILE *fdatabase, *fquery, *fout;
	char *bases;
	char *str;
	bases = (char*) malloc(sizeof(char) * 1000001);
	if (bases == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	str = (char*) malloc(sizeof(char) * 1000001);
	if (str == NULL) {
		perror("malloc str");
		exit(EXIT_FAILURE);
	}

	fdatabase = openfile("dna.in", "r");
	fquery = openfile("query.in", "r");
	fout = openfile("dna.out", "w");

	char desc_dna[100], desc_query[100];
	char line[100];
	int i, found, result;

	fgets(desc_query, 100, fquery);
	remove_eol(desc_query);
	while (!feof(fquery)) {
		fprintf(fout, "%s\n", desc_query);
		// read query string
		fgets(line, 100, fquery);
		remove_eol(line);
		str[0] = 0;
		i = 0;
		do {
			strcat(str + i, line);
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
			bases[0] = 0;
			i = 0;
			fgets(line, 100, fdatabase);
			remove_eol(line);
			do {
				strcat(bases + i, line);
				if (fgets(line, 100, fdatabase) == NULL)
					break;
				remove_eol(line);
				i += 80;
			} while (line[0] != '>');

			result = bmhs(bases, strlen(bases), str, strlen(str), 4);
			if (result > 0) {
				fprintf(fout, "%s\n%d\n", desc_dna, result);
				found++;
			}
		}

		if (!found)
			fprintf(fout, "NOT FOUND\n");
	}

	fclose(fdatabase);
	fclose(fquery);
	fclose(fout);

	free(str);
	free(bases);

	return EXIT_SUCCESS;
}
