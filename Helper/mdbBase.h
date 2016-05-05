/****************************************************************************************
*@Copyrights  2008，中兴软创（南京）计算机有限公司 开发部 CCB项目--C++框架组
*@                   All rights reserved.
*@Name：        mdbBase.h        
*@Description： Base64编码
*@Author:       jiang.mingjun
*@Date：        2010年10月18日
*@History:
******************************************************************************************/
#ifndef __MDB_BASE_H__
#define __MDB_BASE_H__


#include <string>

using namespace std;
//namespace QuickMDB{


	class Base
	{
		public:
			static const std::string base64_chars;
			//static inline bool is_base64(unsigned char c) 
			static bool is_base64(unsigned char c) 
			{
				return (isalnum(c) || (c == '+') || (c == '/'));
			}
			static std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) ;
			static std::string base64_decode(std::string const& encoded_string) ;		
			
	};
//}
#endif

