#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include "queue.h"

#define TEST_CAPACITY 10

int main() {
    printf("=== Testing Bounded Queue ===\n");
    
    bounded_queue_t q;
    assert(queue_init(&q, TEST_CAPACITY) == 0);
    printf("✓ Queue initialized\n");
    
    // Test enqueue
    assert(queue_enqueue(&q, "http://test.com/1", 0) == 0);
    assert(queue_enqueue(&q, "http://test.com/2", 1) == 0);
    printf("✓ Enqueue works\n");
    
    // Test size
    assert(queue_size(&q) == 2);
    printf("✓ Size tracking works\n");
    
    // Test dequeue
    url_entry_t e1, e2;
    assert(queue_dequeue(&q, &e1) == true);
    assert(queue_dequeue(&q, &e2) == true);
    assert(strcmp(e1.url, "http://test.com/1") == 0);
    assert(e1.depth == 0);
    printf("✓ Dequeue works\n");
    
    free(e1.url);
    free(e2.url);
    
    // Test shutdown
    queue_shutdown(&q);
    printf("✓ Shutdown works\n");
    
    queue_destroy(&q);
    printf("✓ Destroy works\n");
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}