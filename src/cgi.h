#ifndef CGI_H
#define CGI_H

#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

long int cgi_getLongInt(const char *key);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // CGI_H
