/*
 * std::string的扩展：std::string_ex
 * 我们的目标是：std::string_ex比std::string除多了一些功能（成员函数）以外，其对象的大小
 * 完全相同，原有的数据成员、成员函数、操作符、内嵌类型、内嵌常量等完全相同，两种类型间可
 * 以相互透明地进行转换，转换时不产生额外的临时对象；
 * 注意：
 * 1. 直接类型转换会产生临时对象，而通过地址转换可能不会产生临时对象，如：
 *		string s1;
 *		string_ex s2;
 *		void f1(string s) {}
 *		void f2(const string& s) {}
 *		void f3(const string* s) {}
 *		void f4(string_ex s) {}
 *		void f5(const string_ex& s) {}
 *		void f6(const string_ex* s) {}
 *		f1(s2);						// 产生临时对象（传递一个临时对象，该临时对象被切片）
 *		f2(s2);						// 不产生临时对象（传递一个地址）
 *		f3((string*)&s2);			// 不产生临时对象（传递一个地址）
 *		f4(s1);						// 产生临时对象（传递一个临时对象，该临时对象通过转换构造函数产生）
 *		f5(s1);						// 产生临时对象（传递一个临时对象的地址，该临时对象通过转换构造函数产生）
 *		f6(&s1);					// 语法错误！
 *		f6((string_ex*)&s1);		// 不产生临时对象（传递一个地址）
 *		friend istream& operator >>(istream& is, string_ex& str)
 *		{ return std::operator>>(is, str.std_string()); }	// 正确：确实输入到当前对象
 *		friend istream& operator >>(istream& is, string_ex& str)
 *		{ return std::operator>>(is, (string)str; }			// 错误：输入到临时对象！
 *	  为了实现切实不产生临时对象的类型转换，请调用如下全局函数：
 *		inline string_ex& STR_STD2EX(string& s);
 *		inline const string_ex& STR_STD2EX(const string& s);
 *		inline string& STR_EX2STD(string_ex& s);
 *		inline const string& STR_EX2STD(const string_ex& s);
 *		inline string_ex* STR_STD2EX(string* s);
 *		inline const string_ex* STR_STD2EX(const string* s);
 *		inline string* STR_EX2STD(string_ex* s);
 *		inline const string* STR_EX2STD(const string_ex* s);
 *	  这几个转化函数使用起来象宏一样（大写，全局，很方便；且不损失任何效率，因为是inline函数）简捷，但
 *	  比宏要安全，因为它们是实实在在的函数，使用举例如下：
 *		string s = "12345";
 *		string_ex se = "54321";
 *		STR_STD2EX(s) = "abcde";
 *		STR_EX2STD(se) = "edcba";
 *		cout << s << endl;
 *		cout << se << endl;
 * 2. 这里的format()和formatv()成员函数不会出现缓冲溢出的情况，可以放心使用；
 * 3. 这里的实现使用了很多内联函数，这样，std::string_ex的效率和std::string实际上是相当的；
 * 4. 我们这里扩展的函数都是强烈依赖std::string来实现的，任何对std::string非const成员函数的
 *	  调用，都可能改变内部的原始数据，因此直接操作由c_str()或data()返回的指针以及iterator时，
 *	  要格外小心；
 */

#ifndef __STRING_EX_H__32848923894823894902390493290423948923894__
#define __STRING_EX_H__32848923894823894902390493290423948923894__

#ifdef WIN32
#pragma warning(disable: 4786)
#endif // #ifdef WIN32

#include <string>
#include <cstdarg>
#include <iostream>

// {{{ 名字空间
namespace std {
// 名字空间 }}}

class string_ex : public string
{
public:
	/*
	 * 下面这些函数是对string类中相应函数的简单调用，因为这些函数不可以被继承
	 */
	string_ex() : string() {}
    string_ex(const string_ex& rhs) : string(rhs) {}
    string_ex(const string_ex& rhs, size_type pos, size_type n) : string(rhs, pos, n) {}
    string_ex(const char *s, size_type n) : string(s, n) {}
    string_ex(const char *s) : string(s) {}
    string_ex(size_type n, char c) : string(n, c) {}
    string_ex(const_iterator first, const_iterator last) : string(first, last) {}

	string_ex& operator=(const string_ex& rhs) { string::operator =(rhs); return *this; }
    string_ex& operator=(const char *s) { string::operator =(s); return *this; }
    string_ex& operator=(char c) { string::operator =(c); return *this; }

	const char& operator[](size_type pos) const { return at(pos); }
    char& operator[](size_type pos) { return at(pos); }

	string_ex& operator+=(const string_ex& rhs) { string::operator +=(rhs); return *this; }
    string_ex& operator+=(const char *s) { string::operator +=(s); return *this; }
    string_ex& operator+=(char c) { string::operator +=(c); return *this; }

	friend ostream& operator <<(ostream& os, const string_ex& str) { return std::operator <<(os, str.std_string()); }
	friend istream& operator >>(istream& is, string_ex& str) { return std::operator>>(is, str.std_string()); }

	/*
	 * 下面是对上述函数的适当补充，以便和string兼容
	 */
	string_ex(const string& rhs) : string(rhs) {}
	string_ex(const string& rhs, size_type pos, size_type n) : string(rhs, pos, n) {}
	string_ex& operator=(const string& rhs) { string::operator =(rhs); return *this; }
	string_ex& operator+=(const string& rhs) { string::operator +=(rhs); return *this; }
	
public:
	/*
	 * 下面的是扩充的函数
	 */

	// 因为string_ex是string的派生类，所以一个string_ex对象就是一个string对象（特别是这里string_ex
	// 没有增加新的数据成员），为了避免二义性，下列操作符不重载，而直接使用string中的实现代码：
	// + == != > < >= <=
	
	string_ex& operator <<(const string_ex& rhs) { return *this += rhs; }
	string_ex& operator <<(const char *s) { return *this += string_ex(s); }
	string_ex& operator <<(char c) { return *this += c; }
	string_ex& operator <<(unsigned char uc);
	string_ex& operator <<(bool b);
	string_ex& operator <<(short s);
	string_ex& operator <<(unsigned short s);
	string_ex& operator <<(int n);
	string_ex& operator <<(unsigned int un);
	string_ex& operator <<(long l);
	string_ex& operator <<(unsigned long ul);
	string_ex& operator <<(float f);
	string_ex& operator <<(double d);
	string_ex& operator <<(long double ld);
	string_ex& operator <<(void* vp);

	int nocasecmp(const string_ex& rhs) const;
	string_ex left(size_t len) const;
	string_ex right(size_t len) const;
	string_ex& upper();
	string_ex& lower();
	string_ex& reverse();

	string_ex& ltrim(const char* space_chars = " \t\v\r\n\f\a\b");
	string_ex& rtrim(const char* space_chars = " \t\v\r\n\f\a\b");
	string_ex& alltrim(const char* space_chars = " \t\v\r\n\f\a\b");
	string_ex& format(const char* fmt, ...);
	string_ex& formatv(const char* fmt, va_list arg);

	static string_ex from(char c);
	static string_ex from(unsigned char uc);
	static string_ex from(bool b);
	static string_ex from(short s);
	static string_ex from(unsigned short us);
	static string_ex from(int n);
	static string_ex from(unsigned int un);
	static string_ex from(long l);
	static string_ex from(unsigned long ul);
	static string_ex from(float f);
	static string_ex from(double d);
	static string_ex from(long double ld);
	static string_ex from(void* vp);

	bool to_bool() const;
	char to_char() const;
	unsigned char to_uchar() const;
	int to_int() const;
	unsigned int to_uint() const;
	short to_short() const;
	unsigned short to_ushort() const;
	long to_long() const;
	unsigned long to_ulong() const;
	float to_float() const;
	double to_double() const;
	long double to_ldouble() const;

	static bool is_white_space(char c, const char* space_chars = " \t\v\r\n\f\a\b"); 

public:
	// 不产生临时对象的类型转换函数
	string& std_string() { return *(string*)this; }
	const string& std_string() const { return *(string*)this; }
	static string_ex& to_string_ex(string& s) { return *(string_ex*)&s; }
	static const string_ex& to_string_ex(const string& s) { return *(const string_ex*)&s; }
};

// 对上面四个不产生临时对象的类型转换函数的简单包装，方便调用
inline string_ex& STR_STD2EX(string& s) { return string_ex::to_string_ex(s); }
inline const string_ex& STR_STD2EX(const string& s) { return string_ex::to_string_ex(s); }
inline string& STR_EX2STD(string_ex& s) { return s.std_string(); }
inline const string& STR_EX2STD(const string_ex& s) { return s.std_string(); }
// 针对指针形式进行重载，使转换操作更具有一般性
inline string_ex* STR_STD2EX(string* s) { return (string_ex*)s; }
inline const string_ex* STR_STD2EX(const string* s) { return (const string_ex*)s; }
inline string* STR_EX2STD(string_ex* s) { return (string*)s; }
inline const string* STR_EX2STD(const string_ex* s) { return (const string*)s; }

// {{{ 名字空间
}
// 名字空间 }}}

#endif // #ifndef __STRING_EX_H__32848923894823894902390493290423948923894__
