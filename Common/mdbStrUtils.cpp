#include "Common/mdbStrUtils.h"
#include <assert.h>
//namespace QuickMDB
//{
        TMdbNtcStrFunc g_oMdbNtcStrFunc;
        const unsigned char g_sMdbNtcLower[256] =     {
                                        0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                        11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                                        21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                                        31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                                        41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
                                        51, 52, 53, 54, 55, 56, 57, 58, 59, 60,
                                        61, 62, 63, 64, 97, 98, 99, 100,101,102,
                                        103,104,105,106,107,108,109,110,111,112,
                                        113,114,115,116,117,118,119,120,121,122,
                                        91, 92, 93, 94, 95, 96, 97, 98, 99, 100,
                                        101,102,103,104,105,106,107,108,109,110,
                                        111,112,113,114,115,116,117,118,119,120,
                                        121,122,123,124,125,126,127,128,129,130,
                                        131,132,133,134,135,136,137,138,139,140,
                                        141,142,143,144,145,146,147,148,149,150,
                                        151,152,153,154,155,156,157,158,159,160,
                                        161,162,163,164,165,166,167,168,169,170,
                                        171,172,173,174,175,176,177,178,179,180,
                                        181,182,183,184,185,186,187,188,189,190,
                                        191,192,193,194,195,196,197,198,199,200,
                                        201,202,203,204,205,206,207,208,209,210,
                                        211,212,213,214,215,216,217,218,219,220,
                                        221,222,223,224,225,226,227,228,229,230,
                                        231,232,233,234,235,236,237,238,239,240,
                                        241,242,243,244,245,246,247,248,249,250,
                                        251,252,253,254,255};
        TMdbNtcStrFunc::TMdbNtcStrFunc()
        {
            *m_szBuffer = '\0';
        }
        bool TMdbNtcStrFunc::MatchString(const char * pszStr, const char * pszNameRule, bool bMatchCase /* = true */)
        {
            const char * pchSrc = pszStr;
            const char * lpszMatch = pszNameRule;
            if(pchSrc == NULL || lpszMatch == NULL)
            {
                return false;
            }

            if(lpszMatch[0] == 0)//Is a empty string
            {
                if(pchSrc[0] == 0)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            int i = 0, j = (int)strlen(pchSrc);

            //生成比较用临时源字符串'szSource'
            char* szSource = new char[ j+1 ];

            if( bMatchCase )
            {    
                memcpy(szSource, pchSrc, (MDB_UINT32)j);
                szSource[j] = '\0';
            }
            else
            {    //Lowercase 'pchSrc' to 'szSource'
                i = 0;
                while(pchSrc[i])
                {
                    if(pchSrc[i] >= 'A' && pchSrc[i] <= 'Z')
                    {
                        szSource[i] = (char)(pchSrc[i] - 'A' + 'a');
                    }
                    else
                    {
                        szSource[i] = pchSrc[i];
                    }
                    i++;
                }
                szSource[i] = 0;
            }

            //生成比较用临时匹配字符串'szMatcher'
            char* szMatcher = new char[strlen(lpszMatch)+1];

            //把lpszMatch里面连续的“*”并成一个“*”后复制到szMatcher中
            i = j = 0;
            while(lpszMatch[i])
            {
                szMatcher[j++] = (!bMatchCase) ?
                    ( (lpszMatch[i] >= 'A' && lpszMatch[i] <= 'Z') ?//Lowercase lpszMatch[i] to szMatcher[j]
                    (char)(lpszMatch[i] - 'A' + 'a' ):
                lpszMatch[i]
                ) :
                lpszMatch[i];       //Copy lpszMatch[i] to szMatcher[j]
                //Merge '*'
                if(lpszMatch[i] == '*')
                {
                    while(lpszMatch[++i] == '*');
                }
                else
                {
                    i++;
                }
            }
            szMatcher[j] = 0;

            //开始进行匹配检查

            int nMatchOffset, nSourOffset;

            bool bIsMatched = true;
            nMatchOffset = nSourOffset = 0;
            while(szMatcher[nMatchOffset])
            {
                if(szMatcher[nMatchOffset] == '*')
                {
                    if(szMatcher[nMatchOffset+1] == 0)
                    {    //szMatcher[nMatchOffset]是最后一个字符

                        bIsMatched = true;
                        break;
                    }
                    else
                    {    //szMatcher[nMatchOffset+1]只能是'?'或普通字符

                        int nSubOffset = nMatchOffset+1;

                        while(szMatcher[nSubOffset])
                        {
                            if(szMatcher[nSubOffset] == '*')
                            {
                                break;
                            }                    
                            nSubOffset++;
                        }

                        if( strlen(szSource+nSourOffset) <
                            size_t(nSubOffset-nMatchOffset-1) )
                        {    //源字符串剩下的长度小于匹配串剩下要求长度
                            bIsMatched = false; //判定不匹配
                            break;            //退出
                        }

                        if(!szMatcher[nSubOffset])//nSubOffset is point to ender of 'szMatcher'
                        {    //检查剩下部分字符是否一一匹配

                            nSubOffset--;
                            int nTempSourOffset = (int)strlen(szSource)-1;
                            //从后向前进行匹配
                            while(szMatcher[nSubOffset] != '*')
                            {
                                if(szMatcher[nSubOffset] == '?')
                                {
                                    ;
                                }
                                else
                                {
                                    if(szMatcher[nSubOffset] != szSource[nTempSourOffset])
                                    {
                                        bIsMatched = false;
                                        break;
                                    }
                                }
                                nSubOffset--;
                                nTempSourOffset--;
                            }
                            break;
                        }
                        else//szMatcher[nSubOffset] == '*'
                        {
                            nSubOffset -= nMatchOffset;

                            char* szTempFinder = new char[nSubOffset];
                            nSubOffset--;
                            memcpy(szTempFinder, szMatcher+nMatchOffset+1, (MDB_UINT32)nSubOffset);
                            szTempFinder[nSubOffset] = 0;

                            int nPos = FindString(szSource+nSourOffset, szTempFinder, 0);
                            if( NULL != szTempFinder )
                            {
                                delete [] szTempFinder;
                                szTempFinder = NULL;
                            }

                            if(nPos != -1)//在'szSource+nSourOffset'中找到szTempFinder
                            {
                                nMatchOffset += nSubOffset;
                                nSourOffset += (nPos+nSubOffset-1);
                            }
                            else
                            {
                                bIsMatched = false;
                                break;
                            }
                        }
                    }
                }        //end of "if(szMatcher[nMatchOffset] == '*')"
                else if(szMatcher[nMatchOffset] == '?')
                {
                    if(!szSource[nSourOffset])
                    {
                        bIsMatched = false;
                        break;
                    }
                    if(!szMatcher[nMatchOffset+1] && szSource[nSourOffset+1])
                    {    //如果szMatcher[nMatchOffset]是最后一个字符，
                        //且szSource[nSourOffset]不是最后一个字符
                        bIsMatched = false;
                        break;
                    }
                    nMatchOffset++;
                    nSourOffset++;
                }
                else//szMatcher[nMatchOffset]为常规字符
                {
                    if(szSource[nSourOffset] != szMatcher[nMatchOffset])
                    {
                        bIsMatched = false;
                        break;
                    }
                    if(!szMatcher[nMatchOffset+1] && szSource[nSourOffset+1])
                    {
                        bIsMatched = false;
                        break;
                    }
                    nMatchOffset++;
                    nSourOffset++;
                }
            }

            if( NULL != szSource )
            {
                delete [] szSource;
                szSource = NULL;
            }
            if( NULL != szMatcher )
            {
                delete [] szMatcher;
                szMatcher = NULL;
            }
            
            return bIsMatched;
        }


        int  TMdbNtcStrFunc::FindString(const char* pszSrc, const char* pszFind, int iStart /* = 0 */)
        {
            //    ASSERT(pchSrc && lpszFind && nStart >= 0);
            if(pszSrc == NULL || pszFind == NULL || iStart < 0)
            {
                return -1;
            }

            int m = (int)strlen(pszSrc);
            int n = (int)strlen(pszFind);

            if( iStart+n > m )
            {
                return -1;
            }

            if(n == 0)
            {
                return iStart;
            }

            //KMP算法
            int* next = new int[n];
            //得到查找字符串的next数组
            {
                n--;

                int j, k;
                j = 0;
                k = -1;
                next[0] = -1;
                while(j < n)
                {
                    if(k == -1 || pszFind[j] == '?' || pszFind[j] == pszFind[k])
                    {
                        j++;
                        k++;                
                        //下面为优化处理，解决存在多比较的情况
                        if(pszFind[j] == pszFind[k])
                        {
                            next[j] = next[k];
                        }
                        else
                        {
                            next[j] = k;
                        }
                    }
                    else
                    {
                        k = next[k];
                    }        
                }
                n++;
            }
            int i = iStart, j = 0;
            while(i < m && j < n)
            {
                if(j == -1 || pszFind[j] == '?' || pszSrc[i] == pszFind[j])
                {
                    i++;
                    j++;
                }
                else
                {
                    j = next[j];
                }
            }
            if( NULL != next )
            {
                delete [] next;
                next = NULL;
            }
            
            if(j >= n)
            {
                return i-n;
            }
            else
            {
                return -1;
            }
        }

        char* TMdbNtcStrFunc::ToLower(char* pszStr)
        {
#ifdef OS_WINDOWS
            return strlwr(pszStr);
#else
            char* p = pszStr;     
            for(; *p != '\0'; ++p)
            {
                *p = (char)tolower(*p);
            }
            return pszStr;
#endif // OS_WINDOWS
        }

        char* TMdbNtcStrFunc::ToUpper(char* pszStr)
        {
#ifdef OS_WINDOWS
            return strlwr(pszStr);
#else
            char* p = pszStr;     
            for(; *p != '\0'; ++p)
            {
                *p = (char)toupper(*p);
            }
            return pszStr;
#endif // OS_WINDOWS
        }

        char* TMdbNtcStrFunc::TrimLeft(char *pszSrc, char cFill)
        {
            assert(pszSrc != NULL);
            if(*pszSrc == '\0') return pszSrc;
            char *pszTemp = pszSrc;
            while(*pszTemp)
            {
                if (*pszTemp != cFill)
                    break;
                else
                    pszTemp++;
            }
            if(pszTemp != pszSrc)
            {
                MDB_UINT32 uiLength = (MDB_UINT32)strlen(pszTemp);
                memmove(pszSrc, pszTemp, uiLength);
                *(pszSrc+uiLength) = '\0';
            }            
            return pszSrc;
        }

        char* TMdbNtcStrFunc::TrimRight(char *pszSrc, char cFill)
        {    
            assert(pszSrc != NULL);
            if(*pszSrc == '\0') return pszSrc;
            char *pszTemp;
            pszTemp = pszSrc+strlen(pszSrc)-1;
            do 
            {
                if (*pszTemp != cFill)
                    break;
                else
                    --pszTemp;
            } while (pszTemp >= pszSrc);
            *(pszTemp+1) = '\0';
            return pszSrc;
        }

        char* TMdbNtcStrFunc::Trim(char *pszSrc, char cFill)
        {    
            assert(pszSrc != NULL);
            if(*pszSrc == '\0') return pszSrc;
            TrimRight(pszSrc, cFill);
            TrimLeft(pszSrc, cFill);
            return pszSrc;
        }

        char* TMdbNtcStrFunc::TrimLeft(char *pszSrc, const char* pszFill /* = " \t" */, int iFillLength /* = -1 */)
        {
            assert(pszSrc != NULL);
            if(*pszSrc == '\0') return pszSrc;
            if(iFillLength == -1) iFillLength = (int)strlen(pszFill);
            char *pszTemp = pszSrc;    
            for(int i=0; i < iFillLength && pszTemp[0]; )
            {
                if (pszTemp[0] == pszFill[i])
                {
                    ++pszTemp;
                    i=0;
                }
                else
                {
                    ++i;
                }
            }

            if(pszTemp > pszSrc)
            {
                MDB_UINT32 uiLength = (MDB_UINT32)strlen(pszTemp);
                memmove(pszSrc, pszTemp, uiLength);
                *(pszSrc+uiLength) = '\0';
            }
            return pszSrc;
        }

        char* TMdbNtcStrFunc::TrimRight(char *pszSrc, const char* pszFill /* = " \t" */, int iFillLength /* = -1 */)
        {
            assert(pszSrc != NULL);
            if(*pszSrc == '\0') return pszSrc;
            if(iFillLength == -1) iFillLength = (int)strlen(pszFill);
            char *pszTemp=NULL;    
            if (pszSrc[0]=='\0')
            {
                return pszSrc;
            }
            pszTemp = pszSrc+strlen(pszSrc)-1;

            int i=0;
            int iIndex=iFillLength;

            for(i=0;i<iIndex && pszTemp >= pszSrc;)
            {
                if (pszTemp[0] == pszFill[i])
                {
                    pszTemp--;
                    i=0;
                }
                else
                {
                    i++;
                }
            }

            pszSrc[pszTemp-pszSrc+1] = '\0';
            return pszSrc;
        }

        char* TMdbNtcStrFunc::Trim(char *pszSrc, const char* pszFill /* = " \t" */, int iFillLength /* = -1 */)
        {
            assert(pszSrc != NULL);
            if(*pszSrc == '\0') return pszSrc;
            if(iFillLength == -1) iFillLength = (int)strlen(pszFill);
            if(iFillLength < 0)
            {
                return pszSrc;
            }
            TrimRight(pszSrc, pszFill, iFillLength);
            TrimLeft(pszSrc, pszFill, iFillLength);
            return pszSrc;
        }

        const char* TMdbNtcStrFunc::ReplaceEnv(const char * pszSrc)
        {
            if(pszSrc == NULL || *pszSrc == '\0') return "";            
            m_sRetStr = pszSrc;
            do
            {
                int iStartPos = -1, iEndPos = -1;
                iStartPos = m_sRetStr.Find("$(");
                if( iStartPos == -1)                       //如果没有环境变量引用的话，返回
                {
                    break;
                }
                iEndPos = m_sRetStr.Find(')', iStartPos + 2);
                if( iEndPos == -1)
                {
                    break;
                }
                TMdbNtcStringBuffer env = m_sRetStr.Substr(iStartPos + 2, iEndPos - iStartPos - 2);
                size_t replaceLen = env.GetLength() + 3;              //要替换的长度
                const char * ptr = getenv(env.c_str());
                if( ptr == NULL )                               //将实际的变量值保存到env中
                {
                    m_sRetStr.Delete(iStartPos, (int)replaceLen);
                }
                else
                {
                    m_sRetStr.Replace(iStartPos, (int)replaceLen, ptr);
                }
            } while (1);
            return m_sRetStr.c_str();
        }

        const char* TMdbNtcStrFunc::Replace(const char* pszOrgSrc, const char* pszSubStr, const char* pszReplaceStr)
        {
            if(pszOrgSrc == NULL || *pszOrgSrc == '\0') return "";
            if(pszSubStr == NULL || *pszSubStr == '\0' || pszReplaceStr == NULL) return pszOrgSrc;
            TMdbNtcStringBuffer rtn = pszOrgSrc;
            int spos;
            spos = rtn.Find(pszSubStr);
            if( spos == -1 )
            {
                return pszOrgSrc;
            }
            TMdbNtcStringBuffer left = rtn.Substr(0,spos);
            TMdbNtcStringBuffer right = rtn.Substr(spos + (int)strlen(pszSubStr));
            m_sRetStr = left + pszReplaceStr + Replace(right.c_str(), pszSubStr, pszReplaceStr);
            return m_sRetStr.c_str();
        }

        char* TMdbNtcStrFunc::Replace(const char* pszOrgStr, const char * pszSubStr, const char * pszReplaceStr,
                                      char *pszOutStr, bool bsensitive)
        {
            int offset = 0;
            int retlen = 0;
            const char *p1 = pszOrgStr;
            char *p2 = pszOutStr;
            char *pRep = (char*)pszReplaceStr;

            while(*p1 != '\0')
            {
                if (bsensitive)
                {
                    if (*p1 == pszSubStr[offset])
                        offset++;
                    else
                        offset=0;
                }
                else
                {
                    if (g_sMdbNtcLower[(int)(*p1)] == g_sMdbNtcLower[(int)pszSubStr[offset]]) 
                        offset++;
                    else
                        offset=0;
                }

                *p2++ = *p1++;
                retlen++;

                if (pszSubStr[offset] == 0)
                {
                    retlen -= offset;
                    p2 -= offset;
                    pRep = (char*)pszReplaceStr;
                    while(*pRep != '\0')
                    {
                        *p2++ = *pRep++;
                        retlen++;
                    }
                    offset=0;
                }
            }
            *p2++ = 0;

            return pszOutStr;
        }

        bool TMdbNtcStrFunc::IsDateTime(const char * pszStr, int iLength /* = -1 */)
        {
            if(pszStr == NULL || *pszStr == '\0') return false;
            if(iLength == -1) iLength = (int)strlen(pszStr);
            if (iLength != 14) return false;
            if (!IsDate(pszStr, 8)) return false;
            if (!IsTime(pszStr+8, 6)) return false;
            return true;
        }

        bool TMdbNtcStrFunc::IsDate(const char * pszStr, int iLength /* = -1 */)
        {
            if(pszStr == NULL || *pszStr == '\0') return false;            
            if(iLength == -1) iLength = (int)strlen(pszStr);
            if(iLength != 8) return false;
            for (int i = 0; i < iLength; i++)
            {
                if ((pszStr[i] < '0') || (pszStr[i] > '9'))
                    return false;
            }
            int iYear = (pszStr[0]-'0')*1000+(pszStr[1]-'0')*100+(pszStr[2]-'0')*10+pszStr[3]-'0',
                iMonth = (pszStr[4]-'0')*10+pszStr[5]-'0',
                iDay = (pszStr[6]-'0')*10+pszStr[7]-'0';

            if(iYear < 1800 || iYear > 8099) return false;
            if((iMonth < 1) || (iMonth > 12)) return false;
            if((iDay < 1) || (iDay > 31)) return false;
            if(iMonth == 2)
            {
                if((iYear%4==0&&iYear%100!=0)||iYear%400==0) //judge leap year
                {
                    if(iDay > 29) return false;
                }
                else
                {
                    if(iDay > 28) return false;
                }
            }
            return true;
        }

        bool TMdbNtcStrFunc::IsIPAddress(const char * pszStr, int iLength /* = -1 */)
        {
            const char* p = pszStr, *pszSubStart = pszStr;
            int iCount = 0;
            while (*p && (iLength == -1 || p-pszStr < iLength))
            {
                if(*p == '.')
                {
                    if(p == pszSubStart)
                    {
                        break;
                    }
                    else
                    {
                        pszSubStart = p+1;
                        ++iCount;
                    }
                }
                else if(p-pszSubStart < 3)
                {
                    if(p != pszSubStart && *pszSubStart == '2')
                    {
                        if(!(*p >= '0' && *p <= '5'))//需为0-5
                        {
                            break;
                        }
                    }
                    else
                    {
                        if(!(*p >= '0' && *p <= '9'))//需为0-9
                        {
                            break;
                        }
                    }
                }
                else//不可以超过3位数字
                {
                    break;
                }
                ++p;
            }
            if((iLength != -1 && p-pszStr < iLength) || *p != '\0'
                || iCount != 3) return false;
            else return true;
        }

        bool TMdbNtcStrFunc::IsTime(const char *pszStr, int iLength /* = -1 */)
        {
            if(pszStr == NULL || *pszStr == '\0') return false;
            if(iLength == -1) iLength = (int)strlen(pszStr);
            if(iLength != 6) return false;
            for (int i = 0; i < iLength; i++)
            {
                if ((pszStr[i] < '0') || (pszStr[i] > '9'))
                    return false;
            }
            int iHour = (pszStr[0]-'0')*10+(pszStr[1]-'0'),
                iMinute = (pszStr[2]-'0')*10+pszStr[3]-'0',
                iSecond = (pszStr[4]-'0')*10+pszStr[5]-'0';
            
            if(iHour > 23 || iMinute > 59 || iSecond > 59) return false;
            else return true;
        }

        bool TMdbNtcStrFunc::IsDigital(const char* pszStr)
        {
            if(pszStr == NULL || *pszStr == '\0') return false;
            if(*pszStr == '+' || *pszStr == '-') ++pszStr;
            while( *pszStr != '\0' )
            {
                if( !isdigit(*pszStr) )
                {
                    return false;
                }
                ++pszStr;
            }
            return true;
        }

        bool TMdbNtcStrFunc::IsAlpha(const char * pszStr)
        {
            if(pszStr == NULL || *pszStr == '\0') return false;
            while( *pszStr != '\0' )
            {
                if( !isalpha(*pszStr) )
                {
                    return false;
                }
                ++pszStr;
            }
            return true;
        }

        const char* TMdbNtcStrFunc::StrPrefix(const char *pszSrc, const char *pszPrefix, bool bCase /* = true */)
        {
            if(pszSrc == NULL || pszPrefix == NULL) return NULL;
            int iResult = 0;
            while(*pszSrc && *pszPrefix)
            {
                iResult = *pszSrc-*pszPrefix;
                if(iResult != 0 && (bCase || (iResult != 32 && iResult != -32)))
                {
                    return NULL;
                }
                else
                {
                    ++pszSrc;
                    ++pszPrefix;
                }
            }
            if(*pszPrefix == '\0') return pszSrc;
            else return NULL;
        }

        const char* TMdbNtcStrFunc::StrSuffix(const char *pszSrc, const char *pszSuffix, bool bCase /* = true */)
        {
            if(pszSrc == NULL || pszSuffix == NULL) return NULL;
            int iResult = 0;
            size_t iSuffixLength = strlen(pszSuffix), iSrcLength = strlen(pszSrc);
            const char* pszSrcEnd = pszSrc+iSrcLength-iSuffixLength;
            if(iSuffixLength == 0) return pszSrcEnd;
            else if(iSuffixLength > iSrcLength) return NULL;
            while(*pszSrcEnd && *pszSuffix)
            {
                iResult = *pszSrcEnd-*pszSuffix;
                if(iResult != 0 && (bCase || (iResult != 32 && iResult != -32)))
                {
                    return NULL;
                }
                else
                {
                    ++pszSrcEnd;
                    ++pszSuffix;
                }
            }
            if(*pszSuffix == '\0') return pszSrc+iSrcLength-iSuffixLength;
            else return NULL;
        }

        char* TMdbNtcStrFunc::StrCopy(char * pszDest, const char* pszSrc, MDB_UINT32 uiCopyLength, char cEndTag/* ='' */)
        {
            if(pszDest == NULL) return pszDest;
            else if(pszSrc == NULL || uiCopyLength == 0)
            {
                *pszDest = '\0';
            }
            else
            {
                for (MDB_UINT32 i=0; i < uiCopyLength && *pszSrc ; ++i, ++pszDest, ++pszSrc)
                {
                    *pszDest=*pszSrc;
                    if(*pszDest == cEndTag)
                    {
                        *pszDest='\0';
                        break;
                    }
                }
                if(*pszDest != '\0')
                {
                    *pszDest = '\0';
                }
            }
            return pszDest;
        }

        const char* TMdbNtcStrFunc::FilterSeparate(const char *pszSrc, const char *pszSeparate)
        {
            int strStartPos;
            int strLen;
            char cSeparate;

            TMdbNtcStringBuffer sBuff, sTarget;
            char* pszBuf = sBuff.GetBuffer(1024);
            char* pszTarget = sTarget.GetBuffer(1024);
            strncpy(pszBuf, pszSrc, sBuff.GetAllocLength()-1);
            while(1)
            {
                cSeparate = *pszSeparate++;
                if(cSeparate == '\0')
                {
                    break;
                }
                int i=0;
                *pszTarget = '\0';
                while(pszBuf[i] != '\0')
                {
                    strLen = 0;
                    strStartPos = i;
                    while((pszBuf[i] != cSeparate)&&(pszBuf[i] !='\0'))
                    {
                        i++;
                        strLen++;
                    }

                    if( strLen > 0 )
                        strncat(pszTarget,pszBuf+strStartPos,(MDB_UINT32)strLen);

                    if (pszBuf[i] == cSeparate)
                        i++;
                }
                strncpy(pszBuf, pszTarget, sBuff.GetAllocLength()-1);
            }
            sTarget.UpdateLength();
            m_sRetStr = sTarget;
            return m_sRetStr.c_str();
        }

        char * TMdbNtcStrFunc::FormatChar(char *pszLine)
        {
            char *pStrPos = pszLine;
            while(*pStrPos)
            {
                if(*pStrPos=='\r')
                    *pStrPos = '\0';
                else if(*pStrPos=='\n')
                    *pStrPos = '\0';
                if(*pStrPos == '\0')
                    break;
                pStrPos++;
            }
            return pszLine;
        }

        bool TMdbNtcStrFunc::IsEmpty(const char *pszSrc)
        {
            if(pszSrc == NULL || *pszSrc == '\0') return true;
            else return false;
        }

        MDB_INT64 TMdbNtcStrFunc::HexToInt64(const char* pszSrc, int iLength /* = -1 */)
        {
            MDB_INT64 iRet = 0;
            char * p = (char *)pszSrc;
            char c;

            if (NULL==pszSrc)
            {
                return iRet;
            }

            while ( *p!='\0' && (iLength == -1 || p-pszSrc < iLength))
            {
                c = *p;
                if ( c>='0'&& c<='9')
                {
                    iRet = iRet*16 + (c - '0');
                }
                else if ( c>='A' && c<='F')
                {
                    iRet = iRet*16 + (c - 'A' + 10);
                }
                else if( c>='a' && c<='f')
                {
                    iRet = iRet*16 + (c - 'a' +10);
                }
                else
                {
                    return -1;
                }
                p++;
            }

            return iRet;
        }


        char* TMdbNtcStrFunc::FilterChar(const char *pszSrc, char *pszDest, int iMaxDestLen, char cFilter)
        {
            if(NULL == pszSrc || NULL == pszDest) return NULL;    
            int index = 0;
            while(0 != *pszSrc && index < iMaxDestLen)
            {
                if(*pszSrc == cFilter)
                {//过滤字符
                    pszSrc++;
                    continue;        
                }
                else
                {
                    pszDest[index] = *pszSrc;
                    pszSrc++;
                    index++;
                }
            }
            pszDest[index] = '\0';
            return pszDest;
        }

        //将字符串进行Hash，转换成一个比较散列的值
        MDB_INT64 TMdbNtcStrFunc::StrToHash(const char* pszSrc)
        {
            const unsigned char* pSrc = reinterpret_cast<const unsigned char*>(pszSrc);
            MDB_UINT64 h = 0;
            MDB_UINT64 g;
            while(*pSrc != 0)
            {
                h =(h<< 4) + (MDB_UINT64)(*pSrc++);
                g = h & 0xf0000000L;
                
                if( g ) 
                    h ^= g >> 24;
                    
                h &= ~g;
            }
            
            return (MDB_INT64)h;
        }

        bool TMdbNtcStrFunc::IsOutOfInt(const char *pszStr, int iSignFlag)
        {
            bool isOut = true;
            int index = 0;
            if(-1 == iSignFlag)
            {                
                char intMin[10] = {'2','1','4','7','4','8','3','6','4','8'};
                while( *pszStr != '\0' )
                {
                    if(*pszStr > intMin[index])
                    {
                        return isOut;  
                    }
                    else if((int)strlen(pszStr)+index>10)
                    {
                        return isOut;
                    }
                    ++index; 
                    ++pszStr;    
                }    
            }
            if(1 == iSignFlag)
            {
                char intMax[10] = {'2','1','4','7','4','8','3','6','4','7'};
                while( *pszStr != '\0' )
                {
                    if(*pszStr > intMax[index])
                    {
                        return isOut;  
                    }
                    else if((int)strlen(pszStr)+index>10)
                    {
                        return isOut;
                    }
                    ++index;
                    ++pszStr;    
                }    
            }
            isOut = false;
            return isOut;      
        }
        bool TMdbNtcStrFunc::IsOutOfInt64(const char * pszStr, int iSignFlag)
        {
            bool isOut = true;
            int index = 0;
            if(-1 == iSignFlag)
            {                
                char llongMin[19] = {'9','2','2','3','3','7','2','0','3','6'
                              ,'8','5','4','7','7','5','8','0','8'};
                while( *pszStr != '\0' )
                {
                    if(*pszStr > llongMin[index])
                    {
                        return isOut;  
                    }
                    else if((int)strlen(pszStr)+index>19)
                    {
                        return isOut;
                    }
                    ++index; 
                    ++pszStr;    
                }    
            }
            if(1 == iSignFlag)
            {
                char llongMax[19] = {'9','2','2','3','3','7','2','0','3','6'
                              ,'8','5','4','7','7','5','8','0','7'};
                while( *pszStr != '\0' )
                {
                    if(*pszStr > llongMax[index])
                    {
                        return isOut;  
                    }
                    else if((int)strlen(pszStr)+index>19)
                    {
                        return isOut;
                    }
                    ++index;  
                    ++pszStr;    
                }    
            }
            isOut = false;
            return isOut;   
        }

        const char* TMdbNtcStrFunc::FloatToStr(double dValue, MDB_INT8 iDecimals)
        {
            m_sRetStr.Clear();
            m_sRetStr.setdecimals(iDecimals);
            m_sRetStr<<dValue;
            return m_sRetStr.c_str();
        }

        const char* TMdbNtcStrFunc::IntToHexStr(MDB_INT64 iValue)
        {
            if(iValue != 0)
            {
                MDB_UINT8 uiTemp = 0, uiByte = 0, j = 0;
                int k = 0;
                if(MdbNtcIsBigEndian())
                {
                    for (j = 0, k = 0; k < 8; ++k)
                    {
                        uiByte = (MDB_UINT8)(*(((char*)&iValue)+k));
                        if(uiByte == 0 && j == 0) continue;
                        uiTemp = (MDB_UINT8)(uiByte/16);
                        if(uiTemp < 10) m_szBuffer[j++] = (char)('0'+uiTemp);
                        else            m_szBuffer[j++] = (char)('A'+(uiTemp-10));
                        uiTemp = (MDB_UINT8)(uiByte%16);
                        if(uiTemp < 10) m_szBuffer[j++] = (char)('0'+uiTemp);
                        else            m_szBuffer[j++] = (char)('A'+(uiTemp-10));
                    }
                }
                else
                {
                    for (j = 0, k = 7; k >= 0; --k)
                    {
                        uiByte = (MDB_UINT8)(*(((char*)&iValue)+k));
                        if(uiByte == 0 && j == 0) continue;
                        uiTemp = (MDB_UINT8)(uiByte/16);
                        if(uiTemp < 10) m_szBuffer[j++] = (char)('0'+uiTemp);
                        else            m_szBuffer[j++] = (char)('A'+(uiTemp-10));
                        uiTemp = (MDB_UINT8)(uiByte%16);
                        if(uiTemp < 10) m_szBuffer[j++] = (char)('0'+uiTemp);
                        else            m_szBuffer[j++] = (char)('A'+(uiTemp-10));
                    }                    
                }
                m_szBuffer[j] = '\0';
            }
            else
            {
                memset(m_szBuffer, '0', 2);
                m_szBuffer[2] = '\0';
            }
            return m_szBuffer;
        }
//}
