/*
 * std::string����չ��std::string_ex
 * ���ǵ�Ŀ���ǣ�std::string_ex��std::string������һЩ���ܣ���Ա���������⣬�����Ĵ�С
 * ��ȫ��ͬ��ԭ�е����ݳ�Ա����Ա����������������Ƕ���͡���Ƕ��������ȫ��ͬ���������ͼ��
 * ���໥͸���ؽ���ת����ת��ʱ�������������ʱ����
 * ע�⣺
 * 1. ֱ������ת���������ʱ���󣬶�ͨ����ַת�����ܲ��������ʱ�����磺
 *		string s1;
 *		string_ex s2;
 *		void f1(string s) {}
 *		void f2(const string& s) {}
 *		void f3(const string* s) {}
 *		void f4(string_ex s) {}
 *		void f5(const string_ex& s) {}
 *		void f6(const string_ex* s) {}
 *		f1(s2);						// ������ʱ���󣨴���һ����ʱ���󣬸���ʱ������Ƭ��
 *		f2(s2);						// ��������ʱ���󣨴���һ����ַ��
 *		f3((string*)&s2);			// ��������ʱ���󣨴���һ����ַ��
 *		f4(s1);						// ������ʱ���󣨴���һ����ʱ���󣬸���ʱ����ͨ��ת�����캯��������
 *		f5(s1);						// ������ʱ���󣨴���һ����ʱ����ĵ�ַ������ʱ����ͨ��ת�����캯��������
 *		f6(&s1);					// �﷨����
 *		f6((string_ex*)&s1);		// ��������ʱ���󣨴���һ����ַ��
 *		friend istream& operator >>(istream& is, string_ex& str)
 *		{ return std::operator>>(is, str.std_string()); }	// ��ȷ��ȷʵ���뵽��ǰ����
 *		friend istream& operator >>(istream& is, string_ex& str)
 *		{ return std::operator>>(is, (string)str; }			// �������뵽��ʱ����
 *	  Ϊ��ʵ����ʵ��������ʱ���������ת�������������ȫ�ֺ�����
 *		inline string_ex& STR_STD2EX(string& s);
 *		inline const string_ex& STR_STD2EX(const string& s);
 *		inline string& STR_EX2STD(string_ex& s);
 *		inline const string& STR_EX2STD(const string_ex& s);
 *		inline string_ex* STR_STD2EX(string* s);
 *		inline const string_ex* STR_STD2EX(const string* s);
 *		inline string* STR_EX2STD(string_ex* s);
 *		inline const string* STR_EX2STD(const string_ex* s);
 *	  �⼸��ת������ʹ���������һ������д��ȫ�֣��ܷ��㣻�Ҳ���ʧ�κ�Ч�ʣ���Ϊ��inline��������ݣ���
 *	  �Ⱥ�Ҫ��ȫ����Ϊ������ʵʵ���ڵĺ�����ʹ�þ������£�
 *		string s = "12345";
 *		string_ex se = "54321";
 *		STR_STD2EX(s) = "abcde";
 *		STR_EX2STD(se) = "edcba";
 *		cout << s << endl;
 *		cout << se << endl;
 * 2. �����format()��formatv()��Ա����������ֻ����������������Է���ʹ�ã�
 * 3. �����ʵ��ʹ���˺ܶ�����������������std::string_ex��Ч�ʺ�std::stringʵ�������൱�ģ�
 * 4. ����������չ�ĺ�������ǿ������std::string��ʵ�ֵģ��κζ�std::string��const��Ա������
 *	  ���ã������ܸı��ڲ���ԭʼ���ݣ����ֱ�Ӳ�����c_str()��data()���ص�ָ���Լ�iteratorʱ��
 *	  Ҫ����С�ģ�
 */

#ifndef __STRING_EX_H__32848923894823894902390493290423948923894__
#define __STRING_EX_H__32848923894823894902390493290423948923894__

#ifdef WIN32
#pragma warning(disable: 4786)
#endif // #ifdef WIN32

#include <string>
#include <cstdarg>
#include <iostream>

// {{{ ���ֿռ�
namespace std {
// ���ֿռ� }}}

class string_ex : public string
{
public:
	/*
	 * ������Щ�����Ƕ�string������Ӧ�����ļ򵥵��ã���Ϊ��Щ���������Ա��̳�
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
	 * �����Ƕ������������ʵ����䣬�Ա��string����
	 */
	string_ex(const string& rhs) : string(rhs) {}
	string_ex(const string& rhs, size_type pos, size_type n) : string(rhs, pos, n) {}
	string_ex& operator=(const string& rhs) { string::operator =(rhs); return *this; }
	string_ex& operator+=(const string& rhs) { string::operator +=(rhs); return *this; }
	
public:
	/*
	 * �����������ĺ���
	 */

	// ��Ϊstring_ex��string�������࣬����һ��string_ex�������һ��string�����ر�������string_ex
	// û�������µ����ݳ�Ա����Ϊ�˱�������ԣ����в����������أ���ֱ��ʹ��string�е�ʵ�ִ��룺
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
	// ��������ʱ���������ת������
	string& std_string() { return *(string*)this; }
	const string& std_string() const { return *(string*)this; }
	static string_ex& to_string_ex(string& s) { return *(string_ex*)&s; }
	static const string_ex& to_string_ex(const string& s) { return *(const string_ex*)&s; }
};

// �������ĸ���������ʱ���������ת�������ļ򵥰�װ���������
inline string_ex& STR_STD2EX(string& s) { return string_ex::to_string_ex(s); }
inline const string_ex& STR_STD2EX(const string& s) { return string_ex::to_string_ex(s); }
inline string& STR_EX2STD(string_ex& s) { return s.std_string(); }
inline const string& STR_EX2STD(const string_ex& s) { return s.std_string(); }
// ���ָ����ʽ�������أ�ʹת������������һ����
inline string_ex* STR_STD2EX(string* s) { return (string_ex*)s; }
inline const string_ex* STR_STD2EX(const string* s) { return (const string_ex*)s; }
inline string* STR_EX2STD(string_ex* s) { return (string*)s; }
inline const string* STR_EX2STD(const string_ex* s) { return (const string*)s; }

// {{{ ���ֿռ�
}
// ���ֿռ� }}}

#endif // #ifndef __STRING_EX_H__32848923894823894902390493290423948923894__
