#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <assert.h>
#include "visited.h"

int main() {
    printf("=== Testing Visited Set ===\n");
    
    visited_set_t set;
    assert(visited_init(&set, 100) == 0);
    printf("✓ Visited set initialized\n");
    
    assert(visited_check_and_add(&set, "http://a.com") == false);
    assert(visited_check_and_add(&set, "http://b.com") == false);
    printf("✓ Add works\n");
    
    assert(visited_check_and_add(&set, "http://a.com") == true);
    printf("✓ Duplicate detection works\n");
    
    assert(visited_count(&set) == 2);
    printf("✓ Count works\n");
    
    visited_destroy(&set);
    printf("✓ Destroy works\n");
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
