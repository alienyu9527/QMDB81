#ifdef WIN32
#pragma warning(disable: 4786)
#pragma warning(disable: 4996)
#pragma warning(disable: 4311)
#pragma warning(disable: 4267)
#endif // #ifdef WIN32

#include <cstdio>
#include <cstring>
//#include <cassert>
#include <cstdarg>
#include "string_ex.h"
#include <assert.h>

#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#else
#include<stdlib.h>
#endif // #ifdef WIN32

// {{{ 名字空间
namespace std {
// 名字空间 }}}

const int INIT_BUF_SIZE = 256;
const int MAX_STR_LEN = 255;

string_ex& string_ex::operator <<(bool b)
{
	return *this += (b ? "true": "flase");
}

string_ex& string_ex::operator <<(short s)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%d", s);
	return *this += buf;
}

string_ex& string_ex::operator <<(unsigned short us)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%u", us);
	return *this += buf;
}

string_ex& string_ex::operator <<(int n)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%d", n);
	return *this += buf;
}

string_ex& string_ex::operator <<(unsigned int un)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%u", un);
	return *this += buf;
}

string_ex& string_ex::operator <<(long l)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%ld", l);
	return *this += buf;
}

string_ex& string_ex::operator <<(unsigned long ul)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%lu", ul);
	return *this += buf;
}

string_ex& string_ex::operator <<(float f)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%g", f);
	return *this += buf;
}

string_ex& string_ex::operator <<(double d)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%g", d);
	return *this += buf;
}

string_ex& string_ex::operator <<(long double ld)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%g", (double)ld);
	return *this += buf;
}

string_ex& string_ex::operator <<(void* vp)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)vp);
	return *this += buf;
}

int string_ex::nocasecmp(const string_ex& rhs) const
{
	const char* p = c_str();
	const char* q = rhs.c_str();
	for (; *p != '\0' && *q != '\0'; ++p, ++q)
	{
		char c1 = ('A' <= *p && *p <= 'Z') ? (char)( (*p + 'a' -'A')) : *p;
		char c2 = ('A' <= *q && *q <= 'Z') ? (char)((*q + 'a' -'A')) : *q;
		if (c1 == c2)
			continue;
		return c1 - c2;
	}
	return (int)(length() - rhs.length());
}

string_ex string_ex::left(size_t len) const
{
	if (len == 0)
		return string();

	len = length() < len ? length() : len;
	return string_ex(c_str(), len);
}

string_ex string_ex::right(size_t len) const
{
	if (len == 0)
		return string();

	//size_t begin = length() - len < 0 ? 0 : length() - len;
      size_t begin ;
      if(length() - len +1 < 1)
      {
          begin = 0;
      }
      else
      {
          begin =  length() - len;
       }
      
	return string_ex(c_str() + begin);
}

string_ex& string_ex::upper()
{
	size_t i;
	for (i = 0; i < length(); ++i)
	{
		at(i) = ('a' <= at(i) && at(i) <= 'z') ? (char)((at(i) + 'A' -'a')) : at(i);
	}
	return *this;
}

string_ex& string_ex::lower()
{
	size_t i;
	for (i = 0; i < length(); ++i)
	{
		at(i) = ('A' <= at(i) && at(i) <= 'Z') ? (char)((at(i) + 'a' -'A')) : at(i);
	}
	return *this;
}

string_ex& string_ex::reverse()
{
	size_t i = 0;
	size_t j = length() -1;

	for (i = 0; i < length() / 2; ++i, --j)
	{
		char t = at(i);
		at(i) = at(j);
		at(j) =t;
	}
	return *this;
}

string_ex& string_ex::ltrim(const char* space_chars /*= " \t\v\r\n\f\a\b"*/)
{
	const char* p = c_str();
	for (; p < c_str() + length(); ++p)
	{
		if (strchr(space_chars, *p) == NULL)
			break;
	}
	if (p == c_str())
	{
		return *this;
	}
	erase(0, (long unsigned int)(p - c_str()));
	return *this;
}

string_ex& string_ex::rtrim(const char* space_chars /*= " \t\v\r\n\f\a\b"*/)
{
	const char* p = c_str() + length() - 1;
	for (; p >= c_str(); --p)
	{
		if (strchr(space_chars, *p) == NULL)
			break;
	}
	++p;
	erase((long unsigned int)(p - c_str()), (long unsigned int)(c_str() + length() - p));
	return *this;
}

string_ex& string_ex::alltrim(const char* space_chars /*= " \t\v\r\n\f\a\b"*/)
{
	ltrim(space_chars);
	rtrim(space_chars);
	return *this;
}

string_ex& string_ex::format(const char* fmt, ...)
{
	assert(fmt != NULL);

	va_list ap;
	int nbuf = INIT_BUF_SIZE;
	char* buf;

	va_start(ap, fmt);
	for (;;)
	{
		buf = new char[nbuf];
		int ret = vsnprintf(buf, (size_t)(nbuf), fmt, ap);
		if (0 <= ret && ret < nbuf)
			break;
		delete[] buf;
		nbuf *= 2;
	}
	*this = buf;
	delete[] buf;
	va_end(ap);
	return *this;
}

string_ex& string_ex::formatv(const char* fmt, va_list ap)
{
	assert(fmt != NULL);

	int nbuf = INIT_BUF_SIZE;
	char* buf;
	for (;;)
	{
		buf = new char[nbuf];
		int ret = vsnprintf(buf, (size_t)(nbuf), fmt, ap);
		if (0 <= ret && ret < nbuf)
			break;
		delete[] buf;
		nbuf *= 2;
	}
	*this = buf;
	delete[] buf;
	return *this;
}

bool string_ex::to_bool() const
{
	if (*this == string_ex("true"))
		return true;
	if (*this == string_ex("false"))
		return false;
	return to_int() != 0;
}

char string_ex::to_char() const
{
	return c_str()[0];
}

string_ex string_ex::from(char c)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%c", c);
	return string_ex(buf);
}

string_ex string_ex::from(unsigned char uc)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%u", uc);
	return string_ex(buf);
}

string_ex string_ex::from(bool b)
{
	return string_ex(b ? "true": "flase");
}

string_ex string_ex::from(short s)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%d", s);
	return string_ex(buf);
}

string_ex string_ex::from(unsigned short us)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%u", us);
	return string_ex(buf);
}

string_ex string_ex::from(int n)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%d", n);
	return string_ex(buf);
}

string_ex string_ex::from(unsigned int un)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%u", un);
	return string_ex(buf);
}

string_ex string_ex::from(long l)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%ld", l);
	return string_ex(buf);
}

string_ex string_ex::from(unsigned long ul)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%lu", ul);
	return string_ex(buf);
}

string_ex string_ex::from(float f)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%g", f);
	return string_ex(buf);
}

string_ex string_ex::from(double d)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%g", d);
	return string_ex(buf);
}

string_ex string_ex::from(long double ld)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "%g", (double)ld);
	return string_ex(buf);
}

string_ex string_ex::from(void* vp)
{
	char buf[MAX_STR_LEN + 1];
	snprintf(buf, sizeof(buf), "0x%08lX", (unsigned long)vp);
	return string_ex(buf);
}

unsigned char string_ex::to_uchar() const
{
	return (unsigned char)strtoul(c_str(), NULL, 0);
}

int string_ex::to_int() const
{
	return (int)strtol(c_str(), NULL, 0);
}

unsigned int string_ex::to_uint() const
{
	return (unsigned int)strtoul(c_str(), NULL, 0);
}

short string_ex::to_short() const
{
	return (short)strtol(c_str(), NULL, 0);
}

unsigned short string_ex::to_ushort() const
{
	return (unsigned short)strtoul(c_str(), NULL, 0);
}

long string_ex::to_long() const
{
	return strtol(c_str(), NULL, 0);
}

unsigned long string_ex::to_ulong() const
{
	return strtoul(c_str(), NULL, 0);
}

float string_ex::to_float() const
{
	return (float)strtod(c_str(), NULL);
}

double string_ex::to_double() const
{
	return strtod(c_str(), NULL);
}

long double string_ex::to_ldouble() const
{
	return (long double)strtod(c_str(), NULL);
}

bool string_ex::is_white_space(char c, const char* space_chars /*= " \t\v\r\n\f\a\b"*/)
{
	return strchr(space_chars, c) != NULL;
}

// {{{ 名字空间
}
// 名字空间 }}}
