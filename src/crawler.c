#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include "html.h"
#include "queue.h"
#include "visited.h"

#define NUM_THREADS 4
#define QUEUE_CAP 1024
#define HASH_BUCKETS 1024

static int         max_depth     = 3;
static int         max_pages     = 100;
static const char *output_dir    = "output";



static bounded_queue_t work_queue; // all threads read from and write to this queue, if removed crawler cannot store pending URLs
static visited_set_t visited; // necessary to prevent loops

static int             pages_crawled = 0;
static pthread_mutex_t pages_mutex   = PTHREAD_MUTEX_INITIALIZER;

static int idle_count =0; // counts the amount of worker threads that are idle, if every thread is idle , queue is empty
static pthread_mutex_t idle_mutex =PTHREAD_MUTEX_INITIALIZER; // protects idle_count , mutex is necessary to prevent race conditions
static pthread_cond_t idle_cond = PTHREAD_COND_INITIALIZER;

static void save_page(int docid, const char *url, const char *html)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/%d.html", output_dir, docid);

    FILE *f = fopen(path, "w");
    if (!f) {
        fprintf(stderr, "  save_page: could not open %s\n", path);
        return;
    }
    fprintf(f, "<!-- crawled from: %s -->\n", url);
    fputs(html, f);
    fclose(f);
}



static void *worker(void *arg)
{
    (void)arg;

    while (1){
        url_entry_t entry;


        pthread_mutex_lock(&idle_mutex);
        idle_count++;

        if (idle_count == NUM_THREADS && queue_size(&work_queue) == 0){
            queue_shutdown(&work_queue);

            pthread_cond_broadcast(&idle_cond);
            pthread_mutex_unlock(&idle_mutex);
            break;
        }
        pthread_mutex_unlock(&idle_mutex);

        bool got = queue_dequeue(&work_queue, &entry);

        pthread_mutex_lock(&idle_mutex);
        idle_count--;
        pthread_mutex_unlock(&idle_mutex);

        if (!got) break;
         pthread_mutex_lock(&pages_mutex);
        if (pages_crawled >= max_pages) {
            pthread_mutex_unlock(&pages_mutex);
            free(entry.url);
            queue_shutdown(&work_queue);
            break;
        }
        int docid = pages_crawled++;
        pthread_mutex_unlock(&pages_mutex);


        printf("[depth %d] Crawling: %s\n", entry.depth, entry.url);

        char *html = fetch_html(entry.url);
        if (!html) {
            fprintf(stderr, "  fetch failed: %s\n", entry.url);
            free(entry.url);
            continue;
    
        }

        save_page(docid, entry.url, html);

        if (entry.depth < max_depth) {
            int link_count = 0;
            char **links   = extract_urls(html, entry.url, &link_count);

            for (int i = 0; i < link_count; i++) {

                if (!visited_check_and_add(&visited, links[i])){

                    if (queue_enqueue(&work_queue, links[i], entry.depth + 1) != 0){
                        
                        fprintf(stderr, " queue full, dropping: %s\n", links[i]);
                    }
                } 
                free(links[i]);
            }
            free(links);
        }

        free(html);
        free(entry.url);

    }

    return NULL;

}


void crawler_run(const char *seed_url)
{
     mkdir(output_dir, 0755);

    if (visited_init(&visited, HASH_BUCKETS) != 0) {
        fprintf(stderr, "crawler_run: visited_init failed\n");
        return;
    }
    if (queue_init (&work_queue, QUEUE_CAP) != 0){
        fprintf(stderr, "crawler_run: queue_init failed\n");
        queue_destroy(&work_queue);
        return;

    }

    visited_check_and_add(&visited, seed_url);
    if (queue_enqueue(&work_queue, seed_url, 0) != 0) {
        fprintf(stderr, "crawler_run: failure to enqueue seed URL\n");
        queue_destroy(&work_queue);
        visited_destroy(&visited);
        return;

    }

    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++){
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0){
            perror("pthread_create");

            queue_shutdown(&work_queue);
            for (int j = 0; j < i; j++) pthread_join(threads[j], NULL);
            queue_destroy(&work_queue);
            visited_destroy(&visited);
            return;
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("\n=== Crawl complete ===\n");
    printf(" Total URLs Visited : %d\n", visited_count(&visited));
    printf(" Peak Queue Size   : %d\n", queue_max_size(&work_queue));
    printf("  Pages saved to     : %s/\n", output_dir);

    queue_destroy(&work_queue);
    visited_destroy(&visited);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <seed-url> [--max-depth N] [--max-pages N] [--output DIR]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *seed_url = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--max-depth") == 0 && i + 1 < argc)
            max_depth = atoi(argv[++i]);
        else if (strcmp(argv[i], "--max-pages") == 0 && i + 1 < argc)
            max_pages = atoi(argv[++i]);
        else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            output_dir = argv[++i];
        else
            seed_url = argv[i];
    }

    printf("Starting crawl from: %s\n", seed_url);
    printf("Threads: %d  | Max Depth: %d\n\n", NUM_THREADS, max_depth);

    crawler_run(seed_url);
    return EXIT_SUCCESS;
}




