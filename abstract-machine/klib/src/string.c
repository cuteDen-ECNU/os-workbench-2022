#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
    int len;

    for(len = 0; *s; s++) len++;
    return len;
}

char *strcpy(char *dst, const char *src) {
  char* addr = dst;
  do *dst++ = *src++; while (*src);   // end by zero
  return addr; 
}

char *strncpy(char *dst, const char *src, size_t n) {
  char* addr = dst;
  do *dst++ = *src++; while (n--);   // end by zero
  return addr; 
}

char *strcat(char *dst, const char *src) {
  char *addr = dst;

  while (*dst) dst++;
  do *dst++ = *src++; while (*src);   // end by zero
  return addr;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1||*s2){
      if (*s1 == *s2){
          s1++;
          s2++;
      }
      else return *s1 - *s2;
  }
  return 0;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  panic("Not implemented");
}

void *memset(void *s, int c, size_t n) {
  const unsigned char uc = c;
  char * temp = (char *)s;
  for(; n != 0; n--) *temp++ = uc;
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
  panic("Not implemented");
}

#endif
