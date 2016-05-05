/****************************************************************************************
*@Copyrights  2013，中兴软创（南京）计算机有限公司 方案架构--QuickMDB小组
*@            All rights reserved.
*@Name：	  mdbJson.h		
*@Description： json解析生成器
*@Author:	jin.shaohua
*@Date：	    2013.2
*@History:
******************************************************************************************/
#ifndef _MDB_JSON_H_
#define _MDB_JSON_H_
#include <string>
#include "rapidjson/document.h" // rapidjson's DOM-style API  
#include "rapidjson/prettywriter.h" // for stringify JSON  
#include "rapidjson/filestream.h"  

//namespace QuickMDB{

    class TMdbStringStream 
    {
    public:
        typedef char Ch;	//!< Character type. Only support char.

        TMdbStringStream(std::string * pBuf) :m_psBuf(pBuf),count_(0),m_iPos(0) { Read(); }
        char Peek() const { return current_; }
        char Take() { char c = current_; Read(); return c; }
        size_t Tell() const { return count_; }
        void Put(char c) { (*m_psBuf) += c;}

        // Not implemented
        char* PutBegin() { return 0; }
        size_t PutEnd(char*) { return 0; }
        
    public:
        std::string * m_psBuf;
        
    private:
        
        void Read() 
        {
            if(NULL == m_psBuf || m_iPos == m_psBuf->length())
            {
                current_ = '\0';
            }
            else
            {
                current_ = m_psBuf->at(m_iPos);
                count_++;
            }
        }
        
        char current_;
        size_t count_;
        size_t m_iPos;
    };

#define MDB_JSON_WRITER_DEF rapidjson::PrettyWriter<TMdbStringStream>

#define INIT_MDB_JSON_WRITER(_writer,_string) \
    TMdbStringStream s(&_string);\
    MDB_JSON_WRITER_DEF  _writer(s);

//}

#endif

