#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORDS 1000

FILE *docs_file;

typedef struct {
    char word[100];
    int doc_ids[100];
    int doc_count;
} WordEntry;

WordEntry index_table[MAX_WORDS];
int index_size = 0;

// Find word in index
int find_word(char *word) {
    for (int i = 0; i < index_size; i++) {
        if (strcmp(index_table[i].word, word) == 0) {
            return i;
        }
    }
    return -1;
}

// Add word to index
void add_word(char *word, int docid) {
    int idx = find_word(word);

    if (idx == -1) {
        strcpy(index_table[index_size].word, word);
        index_table[index_size].doc_ids[0] = docid;
        index_table[index_size].doc_count = 1;
        index_size++;
    } else {
        for (int i = 0; i < index_table[idx].doc_count; i++) {
            if (index_table[idx].doc_ids[i] == docid) {
                return;
            }
        }
        index_table[idx].doc_ids[index_table[idx].doc_count++] = docid;
    }
}

int main() {
    char *files[] = {"./doc1.txt", "./doc2.txt"};
    int num_files = 2;

    // open docs.tsv
    docs_file = fopen("index/docs.tsv", "w");
    if (!docs_file) {
        perror("Error creating docs.tsv");
        return 1;
    }

    // BUILD INDEX
    for (int d = 0; d < num_files; d++) {

        FILE *fp = fopen(files[d], "r");
        if (!fp) {
            perror("Error opening file");
            continue;
        }

        // store doc mapping
        fprintf(docs_file, "%d %s\n", d + 1, files[d]);

        char word[100];

        while (fscanf(fp, "%99s", word) == 1) {

            for (int i = 0; word[i]; i++) {
                word[i] = tolower(word[i]);
            }

            add_word(word, d + 1);
        }

        fclose(fp);
    }

    fclose(docs_file);

    // PRINT INDEX
    FILE *out = fopen("index.txt", "w");
    if (!out) {
        perror("Error creating index file");
        return 1;
    }

    printf("\nInverted Index:\n");

    for (int i = 0; i < index_size; i++) {

        printf("%s: ", index_table[i].word);
        fprintf(out, "%s:", index_table[i].word);

        for (int j = 0; j < index_table[i].doc_count; j++) {
            printf("%d ", index_table[i].doc_ids[j]);
            fprintf(out, " %d", index_table[i].doc_ids[j]);
        }

        printf("\n");
        fprintf(out, "\n");
    }

    fclose(out);
    return 0;
}