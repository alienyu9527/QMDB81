#include "Common/mdbIniFiles.h"
//namespace QuickMDB
//{
        const int iMaxLineLenth = 4096;
        const int iDefintlenth = 32;
        const int iDefSeclenth = 64;

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcReadIni, TMdbNtcBaseObject);
        TMdbNtcReadIni::TMdbNtcReadIni()
        {
            m_fp = NULL;
            m_cCommentSep = '#';
        }

        TMdbNtcReadIni::~TMdbNtcReadIni()
        {
            CloseFile();
        }

        bool TMdbNtcReadIni::OpenFile(const char* pszFilePath, char cCommentSep/* = '#'*/)
        {
            if (NULL == pszFilePath || '\0' == *pszFilePath)
            {
                return false;
            }

            m_fp = fopen(pszFilePath, "r");
            if (NULL == m_fp)
            {
                return false;
            }
            m_cCommentSep = cCommentSep;
            return true;
        }

        void TMdbNtcReadIni::CloseFile()
        {
            if (NULL == m_fp)
            {
                return;
            }
            
            fclose(m_fp);
            m_fp = NULL;
            
            return ;
        }

        int TMdbNtcReadIni::ReadInteger(const char *pszSection, const char *pszKey, int iDefVal /* = 0 */)
        {
            if ((NULL == pszSection) || (NULL == pszKey))
            {
                return iDefVal;
            }
            
            char defstr[iDefintlenth] = {0};
            snprintf(defstr, iDefintlenth, "%d", iDefVal);
            
            TMdbNtcStringBuffer striVal = ReadString(pszSection, pszKey, defstr);
            if (NULL == striVal.c_str())
            {
                return iDefVal;
            }
            
            return atoi(striVal.c_str());
        }

        TMdbNtcStringBuffer TMdbNtcReadIni::ReadString(const char *pszSection, const char *pszKey, const char *pszDefVal /* = "" */)
        {
            if ((NULL == pszSection) || (NULL == pszKey) || ( '\0' == *pszSection)
                || ( '\0' == *pszKey))
            {
                return "";
            }
            else if(m_fp == NULL)
            {
                return "";
            }
            TMdbNtcStringBuffer sSection = "[";
            sSection += pszSection;
            sSection += "]";
            TMdbNtcStringBuffer strLine(iMaxLineLenth);
            
            fseek(m_fp, 0, SEEK_SET);
            bool bFlag = false;
            while (fgets(strLine.GetBuffer(), iMaxLineLenth - 1, m_fp))
            {
                ///更新长度函数
                strLine.UpdateLength();
                
                strLine.Trim(" \t\r\n");
                
                ///去掉注释
                int iNote = strLine.Find(m_cCommentSep);
                if (-1 != iNote)
                {
                    strLine.Delete(iNote, -1);
                }
                if(bFlag)
                {
                    if(strLine.at(0)=='[')//如果之前已经找到了flag，但现在是新的section，那么就结束
                    {
                        break;
                    }
                    else
                    {
                        int iPos = strLine.Find('=');
                        if (-1 != iPos)
                        {
                            TMdbNtcStringBuffer strKey = strLine.Substr(0, iPos);
                            strKey.Trim(" \t\r\n");

                            if (0 == strKey.Compare(pszKey))
                            {
                                TMdbNtcStringBuffer strValue = strLine.Substr(iPos + 1);
                                strValue.Trim(" \t\r\n");

                                return strValue;
                            }
                        }
                    }
                }
                else if(strLine.Find(sSection.c_str()) != -1)
                {            
                    bFlag = true;
                }
            }
            
            return TMdbNtcStringBuffer(pszDefVal);
        }

        bool TMdbNtcReadIni::SectionCheck(const char *pszSection)
        {
            if ((NULL == pszSection) || ( '\0' == *pszSection))
            {
                return false;
            }
            else if(m_fp == NULL)
            {
                return false;
            }
            TMdbNtcStringBuffer strLine(iMaxLineLenth);
            int iSection = -1;
            
            fseek(m_fp, 0, SEEK_SET);
            
            while (fgets(strLine.GetBuffer(), iMaxLineLenth - 1, m_fp))
            {
                ///更新长度函数
                strLine.UpdateLength();
                
                strLine.Trim(" \t\r\n");
                
                ///去掉注释
                int iNote = strLine.Find(m_cCommentSep);
                if (-1 != iNote)
                {
                    strLine.Delete(iNote, -1);
                }
                
                /* into the section */
                iSection = strLine.Find(pszSection);
                if (-1 != iSection)
                {            
                    return true;
                }
            }

            return false;
        }

        bool TMdbNtcReadIni::KeyCheck(const char *pszSection, const char *pszKey)
        {   
            if ((NULL == pszSection) || (NULL == pszKey) 
                || ( '\0' == *pszSection) || ( '\0' == *pszKey))
            {
                return false;
            }
            else if(m_fp == NULL)
            {
                return false;
            }
            TMdbNtcStringBuffer strLine(iMaxLineLenth);
            int iSection = -1;
            
            fseek(m_fp, 0, SEEK_SET);
            bool bFlag = false;
            while (fgets(strLine.GetBuffer(), iMaxLineLenth - 1, m_fp))
            {
                ///更新长度函数
                strLine.UpdateLength();
                
                strLine.Trim(" \t\r\n");
                
                ///去掉注释
                int iNote = strLine.Find(m_cCommentSep);
                if (-1 != iNote)
                {
                    strLine.Delete(iNote, -1);
                } 
                
                if (bFlag)
                {
                    int iPos = strLine.Find('=');
                    if (-1 != iPos)
                    {
                        TMdbNtcStringBuffer strKey = strLine.Substr(0, iPos);
                        strKey.Trim(" \t\r\n");
                        
                        if (0 == strKey.Compare(pszKey))
                        {
                            return true;
                        }
                    }
                    
                    continue;
                }   
                
                /* into the section */
                iSection = strLine.Find(pszSection);
                if (-1 != iSection)
                {            
                    return true;
                }
            }
            return false;
        }

		
		char *TMdbNtcReadIni::Decrypt(char * password)
		{
		    if(password == NULL) return NULL;
		    if(strlen(password) < 2) return NULL;  
		    static char sRetStr[512] = {0};
		    string srcPasswd = password;

		    memset(sRetStr,0,sizeof(sRetStr));

		    unsigned int ch = 0;
		    size_t i = 0;
		    for( i = 0;i<(strlen(password)/2); ++i)
		    {
		        sscanf(srcPasswd.substr(2*i,2).c_str(), "%x", &ch);
		        sRetStr[i] = (char)(ch-30);
		    }
		    sRetStr[i] = '\0';
		    strncpy(password, sRetStr, i);
		    password[i] = '\0';
		    return sRetStr; 
		}
        MDB_INT64 TMdbNtcIniParser::TSectionInfo::Compare(const TMdbNtcBaseObject *pObject) const
        {
            if(pObject == NULL) return 1;
            else return this->sSectionName.Compare(((TSectionInfo*)pObject)->sSectionName);
        }

        TMdbNtcStringBuffer TMdbNtcIniParser::TSectionInfo::ToString( ) const
        {
            TMdbNtcStringBuffer sRet(1024, "[%s]\n", sSectionName.c_str());
            TMdbNtcBaseList::iterator itor = lsKeyInfo.IterBegin();
            for (; itor != lsKeyInfo.IterEnd(); ++itor)
            {
                sRet += itor.data()->ToString()+"\n";
            }
            sRet.Delete((int)sRet.GetLength()-1);
            return sRet;
        }

        MDB_INT64 TMdbNtcIniParser::TKeyInfo::Compare(const TMdbNtcBaseObject *pObject) const
        {
           if(pObject == NULL) return 1;
            else return this->sKeyName.Compare(((TKeyInfo*)pObject)->sKeyName);
        }

        TMdbNtcStringBuffer TMdbNtcIniParser::TKeyInfo::ToString( ) const
        {
            if (sComment.IsEmpty())
            {
                return sKeyName+"="+sKeyValue;
            }
            else
            {
                return sKeyName+"="+sKeyValue+"#" + sComment;
            }
        }

        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcIniParser, TMdbNtcBaseObject);
        
        TMdbNtcIniParser::TMdbNtcIniParser()
        {
            m_cCommentSep = '#';
            m_lsSectionInfo.SetAutoRelease(true);
        }

        TMdbNtcIniParser::~TMdbNtcIniParser()
        {
            Relese();
        }

        bool TMdbNtcIniParser::LoadFromFile(const char* pszFilePath, char cCommentChar /* = '#' */)
        {
            if ((NULL == pszFilePath) || ( '\0' == *pszFilePath))
            {
                return false;
            } 
         
            FILE* fp = fopen(pszFilePath, "rt");
            if (NULL == fp)
            {
                return false;
            }
            bool bRet = true;
            fseek(fp, 0, SEEK_SET);
            m_sFilePath = pszFilePath;
            TMdbNtcStringBuffer strLine(iMaxLineLenth);
            TMdbNtcStringBuffer strSection;
            TMdbNtcStringBuffer strKey;
            TMdbNtcStringBuffer strValue;
            TMdbNtcStringBuffer strComment;
            TSectionInfo *pCurSection = NULL;
            while (fgets(strLine.GetBuffer(), iMaxLineLenth - 1, fp))
            {
                ///更新长度函数
                strLine.UpdateLength();
                strLine.Trim(" \t\r\n");
                strComment.Clear();
                
                ///去掉注释
                int iNote = strLine.Find(cCommentChar);
                if (-1 != iNote)
                {
                    strComment = strLine.Substr(iNote + 1, -1);
                    strComment.Trim(" \t\r\n");
                    
                    strLine.Delete(iNote, -1);
                    strLine.Trim(" \t\r\n");
                }
                
                int iPos = -1;
                
                if (-1 != (iPos = strLine.Find('=')))///key值
                {
                    if (0 < m_lsSectionInfo.GetSize())
                    {
                        if (NULL != pCurSection)
                        {
                            TMdbNtcStringBuffer sKeyName = strLine.Substr(0, iPos);
                            sKeyName.Trim(" \t\r\n");
                            if(sKeyName.IsEmpty())
                            {
                                continue;
                            }
                            TKeyInfo * pkeyinfo = new TKeyInfo ;
                            if (NULL == pkeyinfo)
                            {
                                bRet = false;
                                break;
                            }
                            pkeyinfo->sKeyName = sKeyName;
                            pkeyinfo->sComment = strComment;
                            pkeyinfo->sKeyValue = strLine.Substr(iPos + 1, -1);
                            pkeyinfo->sKeyValue.Trim(" \t\r\n");
                            pCurSection->lsKeyInfo.AddTail(pkeyinfo);
                        }

                    }
                }
                else if (-1 != (iPos = strLine.Find('[')))///段值
                {
                    int iSectionAfter = strLine.Find(']');
                    if (-1 != iSectionAfter)
                    {
                        strSection = strLine.Substr(iPos + 1, iSectionAfter - 1);
                        strSection.Trim(" \t\r\n");
                        if(strSection.IsEmpty())
                        {
                            continue;
                        }
                        TSectionInfo *pSection = new TSectionInfo;
                        if (NULL == pSection)
                        {
                            bRet = false;
                            break;
                        } 
                        pSection->sSectionName = strSection;
                        pCurSection = (TSectionInfo*)m_lsSectionInfo.FindData(*pSection);
                        if (NULL == pCurSection)
                        {
                            pCurSection = pSection;
                            m_lsSectionInfo.AddTail(pSection);
                        }
                        else
                        {
                            delete pSection;
                            pSection = NULL;
                        }
                    }
                }
                else
                {
                   continue;
                }        
                
            }
            fclose(fp);
            fp = NULL;
            //m_lsSectionInfo.Print();
            return bRet;
        }

        void TMdbNtcIniParser::Relese()
        {
            TMdbNtcBaseList::iterator itor = m_lsSectionInfo.IterBegin(), itorEnd = m_lsSectionInfo.IterEnd();
            for (; itor != itorEnd; ++itor)
            {
                TSectionInfo *pSection = (TSectionInfo *)itor.data();
                if (NULL == pSection)
                {
                    continue;
                }
                pSection->lsKeyInfo.Clear();
            }
            m_lsSectionInfo.Clear();
        }

        bool TMdbNtcIniParser::Reload()
        {
            Relese();
            return LoadFromFile(m_sFilePath.c_str(), m_cCommentSep);
        }

        bool TMdbNtcIniParser::SaveToFile(const char* pszFilePath, char cCommentChar /* = '#' */)
        {
            if (NULL == pszFilePath || ( '\0' == *pszFilePath))
            {
                return false;
            }
            
            FILE* fp = fopen(pszFilePath, "w+");
            if (NULL == fp)
            {
                return false;
            }    
            fseek(fp, 0, SEEK_SET);
            TMdbNtcStringBuffer strLine;
            TMdbNtcBaseList::iterator itor = m_lsSectionInfo.IterBegin();
            TMdbNtcBaseList::iterator itorEnd = m_lsSectionInfo.IterEnd();
            while (itor != itorEnd)
            {
                TSectionInfo *pSection = (TSectionInfo *)itor.data();
                strLine =  "[" + pSection->sSectionName;
                strLine = strLine + "]\n";
                
                fputs(strLine.GetBuffer(), fp);
                strLine.Clear();
                
                TMdbNtcBaseList::iterator listitor = pSection->lsKeyInfo.IterBegin();
                TMdbNtcBaseList::iterator listitorEnd = pSection->lsKeyInfo.IterEnd();
                while (listitor != listitorEnd)
                {
                    TKeyInfo *pKey = (TKeyInfo *)listitor.data();
                    if (NULL != pKey)
                    {
                        strLine.Snprintf(iMaxLineLenth,"%s = %s",
                                         pKey->sKeyName.c_str(),
                                         pKey->sKeyValue.c_str());
                        fputs(strLine.GetBuffer(), fp);
                        if(pKey->sComment.IsEmpty())
                        {
                            fputs("\n", fp);
                        }
                        else
                        {
                            strLine.Snprintf(iMaxLineLenth," %c%s\n",
                                m_cCommentSep, pKey->sComment.c_str());
                            fputs(strLine.GetBuffer(), fp);
                        }
                    }
                    
                    listitor++;
                }
                itor++;
            }

            fclose(fp);
            fp = NULL;
            return true;
        }

        int TMdbNtcIniParser::ReadInteger(const char *pszSection, const char *pszKey, int iDefVal /* = 0 */)
        {
            if (NULL  == pszSection || NULL == pszKey 
                ||  '\0' == *pszSection ||  '\0' == *pszKey)
            {
                return iDefVal;
            }
            char ciDefStr[iDefintlenth] = {0};
            snprintf(ciDefStr, iDefintlenth, "%d", iDefVal);

            TMdbNtcStringBuffer strValue = ReadString(pszSection, pszKey, ciDefStr);
            if (0 < strValue.GetLength())
            {
                return atoi(strValue.c_str());
            }
            
            return iDefVal;
        }

        TMdbNtcBaseList::iterator TMdbNtcIniParser::GetSectionIterator(const char *pszSection)
        {
            return m_lsSectionInfo.IterFind(TMdbNtcStringObject(pszSection));
        }

        TMdbNtcBaseList::iterator TMdbNtcIniParser::GetKeyIterator(TMdbNtcBaseList &list, const char *pszKey)
        {
            return list.IterFind(TMdbNtcStringObject(pszKey));
        }

        TMdbNtcStringBuffer TMdbNtcIniParser::ReadString(const char *pszSection, const char *pszKey, const char *pszDefVal /* = "" */)
        {
            if (NULL  == pszSection || NULL == pszKey || NULL == pszDefVal
                ||  '\0' == *pszSection ||  '\0' == *pszKey)
            {
                return pszDefVal;
            }
            
            TMdbNtcBaseList::iterator itor = GetSectionIterator(pszSection);
            if (m_lsSectionInfo.IterEnd() == itor)
            {
                return pszDefVal;
            }
            
            TSectionInfo *pSec = (TSectionInfo *)itor.data();
            itor = GetKeyIterator(pSec->lsKeyInfo, pszKey);
            if (pSec->lsKeyInfo.IterEnd() == itor)
            {
                return pszDefVal;
            }
            TKeyInfo *pKey = (TKeyInfo *)itor.data();
            return pKey->sKeyValue;
            
        }

        void TMdbNtcIniParser::WriteInteger(const char *pszSection, const char *pszKey, int iVal)
        {
            if (NULL  == pszSection || NULL == pszKey
                ||  '\0' == *pszSection ||  '\0' == *pszKey)
            {
                return;
            }
            char szVal[iDefintlenth] = "\0";
            MdbNtcSnprintf(szVal, sizeof(szVal), "%d", iVal);
            WriteString(pszSection, pszKey, szVal);
        }

        void TMdbNtcIniParser::WriteString(const char *pszSection, const char *pszKey, const char *pszVal)
        {
            if (NULL  == pszSection || NULL == pszKey || NULL == pszVal
                ||  '\0' == *pszSection ||  '\0' == *pszKey)
            {
                return;
            }

            TSectionInfo *pSection = NULL;
            TMdbNtcBaseList::iterator itor = GetSectionIterator(pszSection);
            if (m_lsSectionInfo.IterEnd() != itor)
            {
                pSection = static_cast<TSectionInfo *>(itor.data());
                itor = GetKeyIterator(pSection->lsKeyInfo, pszKey);
                if (pSection->lsKeyInfo.IterEnd() != itor)
                {
                    TKeyInfo *pKey = (TKeyInfo *)itor.data();
                    pKey->sKeyValue = pszVal;
                    return;
                }                
            }
            else
            {
                ///没有找到就添加
                pSection = new TSectionInfo;
                pSection->sSectionName = pszSection;
                m_lsSectionInfo.AddTail(pSection);
            }
            
            TKeyInfo *pKey = new TKeyInfo;
            pKey->sKeyName = pszKey;
            pKey->sKeyValue = pszVal;
            
            pSection->lsKeyInfo.AddTail(pKey);            

        }

        bool TMdbNtcIniParser::SectionCheck(const char *pszSection)
        {
            if (NULL == pszSection ||  '\0' == *pszSection)
            {
                return false;
            }
            
            TMdbNtcBaseList::iterator itor = GetSectionIterator(pszSection);
            if (m_lsSectionInfo.IterEnd() != itor)
            {
                return true;
            }

            return false;
        }

        bool TMdbNtcIniParser::KeyCheck(const char *pszSection, const char *pszKey)
        {
            if (NULL  == pszSection || NULL == pszKey
                ||  '\0' == *pszSection ||  '\0' == *pszKey)
            {
                return false;
            }

            TMdbNtcBaseList::iterator itor = GetSectionIterator(pszSection);
            if (m_lsSectionInfo.IterEnd() != itor)
            {
                TSectionInfo *pSec = (TSectionInfo *)itor.data();
                itor = GetKeyIterator(pSec->lsKeyInfo, pszKey);
                if (pSec->lsKeyInfo.IterEnd() != itor)
                {
                    return true;
                }
                
            }
            
            return false;
        }

        void TMdbNtcIniParser::RemoveSection(const char *pszSection)
        {
            if (NULL  == pszSection ||  '\0' == *pszSection)
            {
                return;
            }

            TMdbNtcBaseList::iterator itor = GetSectionIterator(pszSection);
            if (m_lsSectionInfo.IterEnd() != itor)
            {
                TSectionInfo *pSec = (TSectionInfo *)itor.data();
                pSec->lsKeyInfo.Clear();
                delete pSec;
                pSec = NULL;

                m_lsSectionInfo.IterErase(itor);
                
            }            
            return ;
        }

        void TMdbNtcIniParser::RemoveKey(const char *pszSection, const char *pszKey)
        {
            if (NULL  == pszSection ||  '\0' == *pszSection
                || (NULL == pszKey) ||  '\0' == *pszKey)
            {
                return;
            }

            TMdbNtcBaseList::iterator itor = GetSectionIterator(pszSection);
            if (m_lsSectionInfo.IterEnd() != itor)
            {
                TSectionInfo *pSec = (TSectionInfo *)itor.data();
                TMdbNtcBaseList::iterator itorkey = GetKeyIterator(pSec->lsKeyInfo, pszKey);
                
                if (pSec->lsKeyInfo.IterEnd() != itorkey)
                {
                    TKeyInfo *pKey = (TKeyInfo *)itorkey.data();
                    if (NULL != pKey)
                    {
                        delete pKey;
                        pKey = NULL;
                        
                        pSec->lsKeyInfo.IterErase(itorkey);
                    }                    
                }                
            }           
            return ;
        }
//}
