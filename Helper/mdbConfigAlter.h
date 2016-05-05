#ifndef _MDB_CONFIG_H_
#define _MDB_CONFIG_H_

#include "Helper/mdbDictionary.h"
#include <map>
#include <string>

using namespace std;


//namespace QuickMDB{

    

    // ��ֵ�������ļ����Ա����Ϣ
    class TIntAlterAttr
    {
    public:
        TIntAlterAttr();
        ~TIntAlterAttr();
        void Clear();

    public:        
        int m_iOldValue; // ��ֵ
        int m_iNewValue; // ��ֵ
    };

    // �ַ����������ļ����Ա����Ϣ
    class TStringAlterAttr
    {
    public:
        TStringAlterAttr();
        ~TStringAlterAttr();

        void Clear();
        
    public:
        std::string  m_sNewValue; // ��ֵ
        std::string m_sOldValue; // ����������
    };


    // ���б�ռ����ñ����Ϣ
    class TMdbTsAlterInfo
    {
    public:
        TMdbTsAlterInfo();
        ~TMdbTsAlterInfo();

        void Clear();

        // ���б�ռ䶼û�б仯����false, ��֮����true
        bool HaveAnyChange();
        
    public:
        std::vector<std::string> m_vAddTs; // ��ֵ�ı�ռ���Ϣ
        std::vector<std::string> m_vDelTs; // ɾ���ı�ռ�
        std::map<std::string,TIntAlterAttr> m_vPageSizeAlter; // pagesize�б���ı�ռ���Ϣ
    };

    // ĳһ�е����ñ����Ϣ
    class TColumnAlter
    {
    public:
        TColumnAlter();
        ~TColumnAlter();

        void Clear();
        
    public:
        std::string m_sColmName; // ����
        bool m_bLenAlter; // �Ƿ�length�б��
        bool m_bTypeAlter; // �Ƿ�data type �б��
        TIntAlterAttr m_tAlterInfo; // �г��ȱ����Ϣ
    };

	class TColumnInfo
	{
	public:
		TColumnInfo();
		~TColumnInfo();

		void Clear();
		
	public:
		std::string  m_sName; // ����
		int m_iPos; // ��λ��
	};

    // ĳһ�ű�����ñ˰��ظ���Ϣ
    class TMdbTabAlterInfo
    {
    public:
        TMdbTabAlterInfo();
        ~TMdbTabAlterInfo();

        void Clear();

        bool HaveAnyChange();
        
    public:        
        std::string m_sTableName; // ����
        bool m_bTsAlter; //  �Ƿ��б�ռ���
        bool m_bZipTime;
        TStringAlterAttr m_ZipTimeAlter;
        TStringAlterAttr m_tTsAlter; // ������ռ�����ñ����Ϣ
        std::vector<TColumnAlter> m_vColumnAlter; // ����������ñ����Ϣ
        std::vector<TColumnInfo> m_vAddColumn; // ��ֵ������Ϣ
        std::vector<TColumnInfo> m_vDropColumn; // ɾ��������Ϣ
    };
    


    


//}


#endif
