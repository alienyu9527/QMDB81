////////////////////////////////////////////////
// Name: mdbFileList.h
// Author: Li.ShuGang
// Date: 2009/03/27
// Description: ��ȡminidb���ɵ���־�ļ���
////////////////////////////////////////////////
/*
* $History: mdbFileList.h $
 * 
 * *****************  Version 1.0  ***************** 
*/
#ifndef __MINI_DATABASE_FILE_LIST_H__
#define __MINI_DATABASE_FILE_LIST_H__

#include "Helper/mdbStruct.h"

#ifdef WIN32
#include <windows.h>
#endif

//namespace QuickMDB{

#define MAX_FILE_COUNTS 1000

class TMdbFileList
{
public:
    TMdbFileList();
    ~TMdbFileList();
    
public:
	/******************************************************************************
	* ��������	:  Init
	* ��������	:  ���ö�ȡ��Ŀ¼
	* ����		:  pszPath �ļ�·��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	*******************************************************************************/
    int Init(const char* pszPath);

	/******************************************************************************
	* ��������	:  GetFileList
	* ��������	:  ��ȡ�ļ��б�(�ļ�������)
	* ����		:  iCounts ��iPos
	* ����		:  pszHeadFlag �ļ�ͷ��ʾ(Ĭ��Ora_��ͷ)��pszTail
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	*******************************************************************************/
    int GetFileList(int iCounts, int iPos, const char* pszHeadFlag="Ora_", const char* pszTail="");	

    /******************************************************************************
    * ��������	:  GetFileList
    * ��������	:  ��ȡ�ļ��б�(�ļ�������)
    * ����		:  pszHeadFlag �ļ�ͷ��ʾ(Ĭ��Ora_��ͷ)��pszTail
    * ���		:  ��
    * ����ֵ	:  0 - �ɹ�!0 -ʧ��
    * ����		:  li.ming
    *******************************************************************************/
    int GetFileList(const char* pszHeadFlag, const char* pszTail="");
    
    /******************************************************************************
	* ��������	:  Next
	* ��������	:  ��ȡ��һ���ļ���(��·��)
	* ����		:  ��
	* ���		:  pszFullFileName �ļ���
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	*******************************************************************************/
    int Next(char* pszFullFileName);
	int GetFileNameFromFullFileName(char* &pszFileName);

    /******************************************************************************
	* ��������	:  Clear
	* ��������	:  �����¼�����¿�ʼ
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  0 - �ɹ�!0 -ʧ��
	*******************************************************************************/
    void Clear();

    /******************************************************************************
	* ��������	:  GetFileCounts
	* ��������	:  ��ȡ��ǰ���ļ���
	* ����		:  ��
	* ���		:  ��
	* ����ֵ	:  ��ǰ�ļ�����
	*******************************************************************************/
    int GetFileCounts()
    {
        return  m_iFileCounts;   
    }

private:
    char m_sPath[MAX_PATH_NAME_LEN];   //·����
    int  m_iFileCounts;                //���ζ�ȡ�ļ���
    char *m_FileName[MAX_FILE_COUNTS+1]; //�ļ���   
    int  m_iCurPos;                    //����ļ�λ��
};

//}

#endif //__MINI_DATABASE_FILE_LIST_H__


