#include "Helper/mdbEncrypt.h"
#include <string.h>
#include <string>
#include <cstring>

//namespace QuickMDB{
    /******************************************************************************
    * ��������	:  EncryptEx()
    * ��������	:  �����ַ���
    * ����		:  cSrc ����ǰ�ַ���
    * ���		:  cDest ���ܺ��ַ���
    * ����ֵ	:  ��
    * ����		:  cao.peng
    *******************************************************************************/
    void TMdbEncrypt::EncryptEx(char*   cSrc,char*   cDest)   
    {   
        char c;   
        int i;
        int h;
        int l;
        int j=2;  
        if(NULL == cSrc || NULL == cDest)
        {
            return;
        }
        strcat(cDest,"!!");
        for(i=0;i<(int)strlen(cSrc);i++)   
        {   
            c = cSrc[i];   
            h = (c - 0)/10;   
            l = (c - 0)%10;   
            cDest[j]=h+'A';   
            cDest[j+1]=l+'A';   
            j += 2;   
        }   
        strcat(cDest,"@@");
        cDest[j+2]='\0';     
        return   ;   
    }

    /******************************************************************************
    * ��������	:  DecryptEx()
    * ��������	:  �����ַ���
    * ����		:  cSrc ����ǰ�ַ���
    * ���		:  cDest ���ܺ��ַ���
    * ����ֵ	:  ��
    * ����		:  cao.peng
    *******************************************************************************/
    void TMdbEncrypt::DecryptEx(char*   cSrc,char*   cDest)   
    {   
        int i;
        int h;
        int l;
        int j=0;   
        if(NULL == cSrc || NULL == cDest)
        {
            return;
        }
        if(!IsEncryptStr(cSrc))
        {
            return;
        }
        cSrc = cSrc + 2;
        for(i=0;i<(int)strlen(cSrc)-2;i=i+2)   
        {   
            h = (cSrc[i]-'A');   
            l = (cSrc[i+1]-'A');   
            cDest[j] = h*10 + l;   
            j++;   
        }   
        cDest[j]='\0';   
        return;   
    } 

    /******************************************************************************
    * ��������	:  IsEncryptStr()
    * ��������	:  �ж�һ���ַ����Ƿ�Ϊ�ڲ������ַ���
    * ����		:  pchString Դ�ַ���
    * ���		:  ��
    * ����ֵ	:  true Ϊ�ڲ������ַ�����false : �Ǽ����ַ���
    * ����		:  cao.peng
    *******************************************************************************/
    bool TMdbEncrypt::IsEncryptStr(const char *pchString)
    {
        if(NULL == pchString)
        {
            return false;
        }

        if(pchString[0] == '!' 
        && pchString[1] == '!'
        && pchString[strlen(pchString)-1] == '@'
        && pchString[strlen(pchString)-2] == '@')
        {
            return true;
        }
        else
        {
         return false;
        }
    }

//}
