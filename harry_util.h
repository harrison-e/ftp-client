// harry_util.h 
// utilities for me
#include <stdbool.h>

/**
 *  Convenience functions
 */

// debug print 
const bool H_DEBUG = false;
void HU_dprintf(char* message) {
  if (H_DEBUG)
    printf("DEBUG: %s\n", message);
}

// error print 
void HU_eprintf(char* message) {
  fprintf(stderr, "ERROR: %s\n", message);
}

// todo print 
void HU_todoprintf(char* message) {
  printf("TODO: %s\n", message);
}
