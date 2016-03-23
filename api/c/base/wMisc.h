
/**
 * Copyright (C) Anny.
 * Copyright (C) Disvr, Inc.
 */

#ifndef _W_MISC_H_
#define _W_MISC_H_

#include <sstream>
#include <vector>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/file.h>	//int flock(int fd,int operation);

#include "wCore.h"

inline const char* IP2Text(long ip)
{
	in_addr in;
	in.s_addr = ip;

	return inet_ntoa(in);
}

inline long Text2IP(const char* ipstr)
{
	return inet_addr(ipstr);
}

inline unsigned int HashString(const char* s)
{
	unsigned int hash = 5381;
	while (*s)
	{
		hash += (hash << 5) + (*s ++);
	}
	return hash & 0x7FFFFFFF;
}

//��ȡ���뼶ʱ��
inline unsigned long long GetTickCount()    //clock_t
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000 + (unsigned long long)tv.tv_usec / 1000;
}

//��ȡ΢���ʱ��
inline int64_t GetTimeofday()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + (int64_t)tv.tv_usec;
}

inline void GetTimeofday(struct timeval* pVal, void*)
{
    if (pVal == NULL)
    {
        return;
    }
    gettimeofday(pVal, NULL);
}

//long -> string
inline string Itos(const long &i)
{
    string sTmp;
    stringstream sRet(sTmp);
    sRet << i;
    return sRet.str();
}

inline void Strlow(u_char *dst, u_char *src, size_t n)
{
    while (n) 
    {
        *dst = tolower(*src);
        dst++;
        src++;
        n--;
    }
}

inline int GetCwd(char *path, size_t size)
{
    if(getcwd(path, size) == NULL)
    {
        return -1;
    }
    return 0;
}

//long -> char*
void itoa(unsigned long val, char *buf, unsigned radix);

u_char *Cpystrn(u_char *dst, u_char *src, size_t n);

//�ָ��ַ���
vector<string> Split(string sStr, string sPattern, bool bRepeat = true);

//���������Լ��
int Gcd(int a, int b);
//n�������Լ��
int Ngcd(int *arr, int n);

#endif