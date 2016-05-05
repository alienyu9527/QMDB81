#ifndef _MDB_CONFIG_H_
#define _MDB_CONFIG_H_

#include "Helper/mdbDictionary.h"
#include <map>
#include <string>

using namespace std;


//namespace QuickMDB{

    

    // 数值型配置文件属性变更信息
    class TIntAlterAttr
    {
    public:
        TIntAlterAttr();
        ~TIntAlterAttr();
        void Clear();

    public:        
        int m_iOldValue; // 旧值
        int m_iNewValue; // 新值
    };

    // 字符串型配置文件属性变更信息
    class TStringAlterAttr
    {
    public:
        TStringAlterAttr();
        ~TStringAlterAttr();

        void Clear();
        
    public:
        std::string  m_sNewValue; // 新值
        std::string m_sOldValue; // 配置属性名
    };


    // 所有表空间配置变更信息
    class TMdbTsAlterInfo
    {
    public:
        TMdbTsAlterInfo();
        ~TMdbTsAlterInfo();

        void Clear();

        // 所有表空间都没有变化返回false, 反之返回true
        bool HaveAnyChange();
        
    public:
        std::vector<std::string> m_vAddTs; // 新值的表空间信息
        std::vector<std::string> m_vDelTs; // 删除的表空间
        std::map<std::string,TIntAlterAttr> m_vPageSizeAlter; // pagesize有变更的表空间信息
    };

    // 某一列的配置变更信息
    class TColumnAlter
    {
    public:
        TColumnAlter();
        ~TColumnAlter();

        void Clear();
        
    public:
        std::string m_sColmName; // 列名
        bool m_bLenAlter; // 是否length有变更
        bool m_bTypeAlter; // 是否data type 有变更
        TIntAlterAttr m_tAlterInfo; // 列长度变更信息
    };

	class TColumnInfo
	{
	public:
		TColumnInfo();
		~TColumnInfo();

		void Clear();
		
	public:
		std::string  m_sName; // 列名
		int m_iPos; // 列位置
	};

    // 某一张表的配置彼岸呢个信息
    class TMdbTabAlterInfo
    {
    public:
        TMdbTabAlterInfo();
        ~TMdbTabAlterInfo();

        void Clear();

        bool HaveAnyChange();
        
    public:        
        std::string m_sTableName; // 表名
        bool m_bTsAlter; //  是否有表空间变更
        bool m_bZipTime;
        TStringAlterAttr m_ZipTimeAlter;
        TStringAlterAttr m_tTsAlter; // 所属表空间的配置变更信息
        std::vector<TColumnAlter> m_vColumnAlter; // 变更的列配置变更信息
        std::vector<TColumnInfo> m_vAddColumn; // 新值的列信息
        std::vector<TColumnInfo> m_vDropColumn; // 删除的列信息
    };
    


    


//}


#endif
