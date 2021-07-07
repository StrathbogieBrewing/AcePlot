#include <stdlib.h>
#include <string.h>

char *cgi_getValueString(const char *key, char *queryString) {
  char *keyStart = strstr(queryString, key);
  if (keyStart) {
    int keyLength = strlen(key);
    char *valueStart = keyStart + keyLength;
    if (*valueStart++ == '=') {
      char *value = valueStart;
      while ((*value != ';') && (*value != '&') && (*value != 0)) {
        value++;
      }
      *value = '\0';
      return valueStart;
    } else {
      return NULL;
    }
  }
}

long int cgi_getLongInt(const char *key) {
  const char *queryString = getenv("QUERY_STRING");
  char query[strlen(queryString) + 1];
  strcpy(query, queryString);
  char *value = cgi_getValueString(key, query);
  if (value != NULL){
    return strtol(value, NULL, 10);
  }
    return 0;
}
