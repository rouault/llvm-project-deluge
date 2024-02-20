int rep_vsnprintf (char *str, __SIZE_TYPE__ count, const char *fmt, __builtin_va_list args);
int rep_snprintf(char *str,__SIZE_TYPE__ count,const char *fmt,...);
int rep_printf(const char *fmt, ...);
int rep_vasprintf(char **ptr, const char *format, __builtin_va_list ap);
int rep_asprintf(char **ptr, const char *format, ...);
