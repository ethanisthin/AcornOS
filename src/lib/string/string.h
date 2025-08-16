#ifndef STRING_H
#define STRING_H

int strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, int n);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, int n);
char* strcat(char* dest, const char* src);
void* memset(void* ptr, int value, int num);
void* memcpy(void* dest, const void* src, int num);
int memcmp(const void* ptr1, const void* ptr2, int num);
int int_to_string(int value, char* str, int base);
int uint_to_string(unsigned int value, char* str, int base);
int string_to_int(const char* str);
void reverse_string(char* str, int length);
int is_digit(char c);
int is_alpha(char c);
int is_space(char c);

#endif