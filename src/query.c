#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_DOCS 100
#define MAX_LINE 256
#define MAX_QUERY 10

// ---------------- DOC MAP ----------------

typedef struct {
    int id;
    char path[200];
} Doc;

Doc docs[MAX_DOCS];
int doc_count = 0;

void load_docs() {
    FILE *fp = fopen("index/docs.tsv", "r");
    if (!fp) {
        perror("docs.tsv missing");
        exit(1);
    }

    while (fscanf(fp, "%d %s", &docs[doc_count].id, docs[doc_count].path) == 2) {
        doc_count++;
    }

    fclose(fp);
}

char* get_doc(int id) {
    for (int i = 0; i < doc_count; i++) {
        if (docs[i].id == id) {
            return docs[i].path;
        }
    }
    return "UNKNOWN";
}

// ---------------- CLEAN ----------------

void clean(char *s) {
    int len = strlen(s);

    while (len > 0 &&
          (s[len-1] == '\n' || s[len-1] == ' ' || s[len-1] == '\r')) {
        s[len-1] = '\0';
        len--;
    }

    for (int i = 0; s[i]; i++) {
        s[i] = tolower(s[i]);
    }
}

// ---------------- INTERSECTION ----------------

int intersect(int *a, int a_size, int *b, int b_size, int *out) {
    int k = 0;

    for (int i = 0; i < a_size; i++) {
        for (int j = 0; j < b_size; j++) {
            if (a[i] == b[j]) {
                out[k++] = a[i];
            }
        }
    }

    return k;
}

// ---------------- MAIN ----------------

int main() {

    load_docs();

    FILE *fp = fopen("index.txt", "r");
    if (!fp) {
        perror("index.txt missing");
        return 1;
    }

    char input[256];
    printf("Enter query: ");
    fgets(input, sizeof(input), stdin);
    clean(input);

    // ---------------- SPLIT QUERY ----------------

    char *words[MAX_QUERY];
    int word_count = 0;

    char *token = strtok(input, " ");
    while (token && word_count < MAX_QUERY) {
        words[word_count++] = token;
        token = strtok(NULL, " ");
    }

    // ---------------- RESULT INITIALIZATION ----------------

    int final_results[MAX_DOCS];
    int final_size = 0;
    int first = 1;

    char line[MAX_LINE];

    // ---------------- READ INDEX ----------------

    while (fgets(line, sizeof(line), fp)) {

        char word[100];
        int docs_list[MAX_DOCS];
        int count = 0;

        char *t = strtok(line, ": \n");
        if (!t) continue;

        strcpy(word, t);
        clean(word);

        while ((t = strtok(NULL, " \n")) != NULL) {
            docs_list[count++] = atoi(t);
        }

        // ---------------- MATCH QUERY WORDS ----------------

        for (int w = 0; w < word_count; w++) {

            if (strcmp(word, words[w]) == 0) {

                if (first) {
                    for (int i = 0; i < count; i++) {
                        final_results[i] = docs_list[i];
                    }
                    final_size = count;
                    first = 0;
                } else {
                    int temp[MAX_DOCS];
                    final_size = intersect(final_results, final_size,
                                           docs_list, count, temp);

                    for (int i = 0; i < final_size; i++) {
                        final_results[i] = temp[i];
                    }
                }
            }
        }
    }

    fclose(fp);

    // ---------------- OUTPUT ----------------

    if (final_size == 0) {
        printf("No results found\n");
        return 0;
    }

    printf("\nResults:\n");

    for (int i = 0; i < final_size; i++) {
        printf("%d %s\n",
               final_results[i],
               get_doc(final_results[i]));
    }

    return 0;
}