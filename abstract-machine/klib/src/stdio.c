#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

// typedef char* va_list;
#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

// static inline void puts(const char *s) {
//   for (; *s; s++) putch(*s);
//   return;
// }
int printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    char buf[0x100] = {0};
    
    vsprintf(buf, fmt, args);
    va_end(args);
    putstr(buf);
    return strlen(buf);
}

char* itoa(int value, char *str, int radix){
    
    char reverse[36];   
    char *p = reverse;
    bool sign = (value >= 0)?true:false;

    value = (value >= 0)?value:-value;
    *p++ = '\0';
    while (value >= 0){
        *p++ = "0123456789abcdef"[value%radix];
        value /= radix;
        if (value == 0) break;
    }

    if (!sign) {
        *p = '-';
    }
    else {
        p--;
    }

    while (p >= reverse){
        *str++ = *p--;
    }

    return str;
}

char* uitoa(uint32_t value, char *str, int radix){
    char reverse[36];   
    char *p = reverse;

    *p++ = '\0';
    while (value != 0){
        *p++ = "0123456789abcdef"[value%radix];
        value /= radix;
        if (value == 0) break;
    }
    p--;

    while (p >= reverse){
        *str++ = *p--;
    }

    return str;
}

/* NOT WORK TODO */
char* gcvt(double value, int ndigit, char *buf){
    char tmpbuf[72];
    int int_part = (int)value;

    double folat_part = value - int_part;

    if (folat_part < 0) folat_part = -folat_part;
    itoa(int_part, tmpbuf, 10);

    char *p = tmpbuf;

     while (*p != '\0') p++;

    *p++ = '.';

    while (ndigit > 0 && folat_part > 0.00000001){
        *p++ = (int)(folat_part*10) + '0';
        folat_part = (folat_part * 10) - (int)(folat_part * 10);
        ndigit--;
    }

    *p = '\0';
    strcpy(buf, tmpbuf);
    return buf;
}

int vsprintf(char *buf, const char *fmt, va_list args){
    char *p;
    // va_list p_next_arg = args;
    for (p = buf; *fmt; fmt++){
        if (*fmt != '%'){
            *p++ = *fmt;
            continue;
        }
        fmt++;  // *fmt = '%'
        switch (*fmt){
            case 'd':{
                // itoa(va_arg(p_next_arg, int),p,10);
                int d = va_arg(args, int);
                itoa(d,p,10);
                p += strlen(p);
                break;
                }
            case 'x':{
                // uitoa(va_arg(p_next_arg, unsigned int),p,16);
                uitoa(va_arg(args, unsigned int),p,16);
                p += strlen(p);
                break;
            }case 'c':{
                // *p++ = va_arg(p_next_arg, char);
                char c = va_arg(args, int); 
                *p++ = c;
                break;
            }case 's':{
                *p = '\0';
                // strcat(p, va_arg(p_next_arg, char *));
                char* s = va_arg(args, char *); 
                strcat(p, s);
                p += strlen(p);
                break;
            }
            // case 'f':
            //     // gcvt(va_arg(p_next_arg, double), 6, p);
            //     gcvt(va_arg(args, double), 6, p);
            //     p += strlen(p);
            //     break;
            case 'p':{
                // uitoa(va_arg(p_next_arg, unsigned int),p,16);
                *p++ = '0'; *p++ = 'x';
                int x = va_arg(args, unsigned int);
                uitoa(x,p,16);
                p += strlen(p);
                break;
            }
            default:
                break;
        }
    }
    *p = '\0';
    return strlen(buf);
}


int sprintf(char *out, const char *fmt, ...) {
    
    va_list args;
    char tmp[0x100] = {0};
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);
    strcpy(out, tmp);
    
    return strlen(out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
    va_list args;
    char tmp[0x100] = {0};
    va_start(args, fmt);
    vsprintf(tmp, fmt, args);
    va_end(args);
    strncpy(out, tmp, n);
    return strlen(tmp);
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
