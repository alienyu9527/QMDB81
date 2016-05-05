#include "Common/mdbNtcSplit.h"
//#include "Sdk/mdbMemoryLeakDetectInterface.h"

//namespace QuickMDB
//{
        MDB_ZF_IMPLEMENT_OBJECT(TMdbNtcSplit, TMdbNtcBaseObject);
        TMdbNtcSplit::TMdbNtcSplit( )
        {
                m_pBuffer = new char[ MDB_NTC_ZS_SPLIT_INIT_BUFFER_LENGTH + 1 ];
                if( NULL == m_pBuffer )
                {
//                    return false;
                }
                m_uiBufferLength = MDB_NTC_ZS_SPLIT_INIT_BUFFER_LENGTH;
                memset( m_pBuffer, 0, m_uiBufferLength + 1 );
                m_oTSplitField.m_uiFieldSize = 0;
                m_oTSplitField.m_ppFieldValue = new char* [ m_oTSplitField.m_uiFieldCapacity ];
                if( NULL == m_oTSplitField.m_ppFieldValue )
                {
//                    return false;
                }
                memset( m_oTSplitField.m_ppFieldValue, 0, sizeof(char *) * m_oTSplitField.m_uiFieldCapacity );
                m_oTSplitField.m_piFieldValueLength = new int [ m_oTSplitField.m_uiFieldCapacity ];
                if( NULL == m_oTSplitField.m_piFieldValueLength )
                {
//                    return false;
                }
                memset( m_oTSplitField.m_piFieldValueLength, 0, sizeof(int) * m_oTSplitField.m_uiFieldCapacity );
        }

        TMdbNtcSplit::~TMdbNtcSplit( )
        {
            m_uiBufferLength = 0;
            if( NULL != m_pBuffer )
            {
                delete [] m_pBuffer;
                m_pBuffer = NULL;
            }
        }

        bool TMdbNtcSplit::SplitString( const char *sSplitString, const char cSplitChar, bool bSkipEmptyField )
        {
            m_oTSplitField.m_uiFieldSize = 0;
            int iLength = 0;
            if( ( NULL == sSplitString )
             || ( ( iLength = (int)strlen( sSplitString ) ) < 1 )
             || ( '\0' == cSplitChar ) )
            {
                return false;
            }
            if( iLength > (int)m_uiBufferLength )
            {
                if( !GrowBuffer( iLength ) )
                    return false;
            }
            m_pBuffer[ 0 ] = '\0';
            m_cSplitChar = cSplitChar;
            strcpy( m_pBuffer, sSplitString );
            char *pCur = m_pBuffer;
            char *pPre = pCur;
            do
            {
            	if( NULL != ( pCur = strchr( pCur, m_cSplitChar ) ) )
            	{
	                *pCur = '\0';
                	if( pCur != pPre || !bSkipEmptyField )
            		{
	                	m_oTSplitField.m_ppFieldValue[ m_oTSplitField.m_uiFieldSize ] = pPre;
	                	m_oTSplitField.m_piFieldValueLength[ m_oTSplitField.m_uiFieldSize++ ] = (int)(pCur - pPre);
        	        	if( m_oTSplitField.m_uiFieldCapacity == m_oTSplitField.m_uiFieldSize )
            	    	{
                	    	if( !GrowSplitField( ) )
                    		{
                        		m_oTSplitField.m_uiFieldSize = 0;
                        		return false;
                    		}
                		}
            		}
					pPre = ++pCur;
            	}
            	else
            	{
            		if( *pPre != '\0' || !bSkipEmptyField )
            		{
	                	m_oTSplitField.m_ppFieldValue[ m_oTSplitField.m_uiFieldSize ] = pPre;
	                	m_oTSplitField.m_piFieldValueLength[ m_oTSplitField.m_uiFieldSize++ ] = (int)strlen( pPre );
        	        	if( m_oTSplitField.m_uiFieldCapacity == m_oTSplitField.m_uiFieldSize )
            	    	{
                	    	if( !GrowSplitField( ) )
                    		{
                        		m_oTSplitField.m_uiFieldSize = 0;
                        		return false;
                    		}
                		}
                	}
            	}
           	} while( NULL != pCur );
            return true;
        }

        bool TMdbNtcSplit::SplitString( const char *sSplitString, const char *sSplitSubString, bool bSkipEmptyField )
        {
            m_oTSplitField.m_uiFieldSize = 0;
            int iLength1 = 0, iLength2 = 0;
            if( ( NULL == sSplitString )
             || ( NULL == sSplitSubString )
             || ( ( iLength1 = (int)strlen( sSplitString ) ) < 1 )
             || ( ( iLength2 = (int)strlen( sSplitSubString ) ) < 1 ) )
            {
                return false;
            }
            if( iLength1 > (int)m_uiBufferLength )
            {
                if( !GrowBuffer( iLength1 ) )
                    return false;
            }
            m_pBuffer[ 0 ] = '\0';
            strcpy( m_pBuffer, sSplitString );
            char *pCur = m_pBuffer;
            char *pPre = pCur;
            do
            {
            	if( NULL != ( pCur = strstr( pCur, sSplitSubString ) ) )
            	{
                    *pCur = '\0';
//	                for(int i=0;i<iLength2;i++)
//	                    pCur[i] = '\0';
                	if( pCur != pPre || !bSkipEmptyField )
            		{
	                	m_oTSplitField.m_ppFieldValue[ m_oTSplitField.m_uiFieldSize ] = pPre;
	                	m_oTSplitField.m_piFieldValueLength[ m_oTSplitField.m_uiFieldSize++ ] = (int)(pCur - pPre);
        	        	if( m_oTSplitField.m_uiFieldCapacity == m_oTSplitField.m_uiFieldSize )
            	    	{
                	    	if( !GrowSplitField( ) )
                    		{
                        		m_oTSplitField.m_uiFieldSize = 0;
                        		return false;
                    		}
                		}
            		}
            		pCur += iLength2;
					pPre = pCur;
            	}
            	else
            	{
            		if( *pPre != '\0' || !bSkipEmptyField )
            		{
	                	m_oTSplitField.m_ppFieldValue[ m_oTSplitField.m_uiFieldSize ] = pPre;
	                	m_oTSplitField.m_piFieldValueLength[ m_oTSplitField.m_uiFieldSize++ ] = (int)strlen( pPre );
        	        	if( m_oTSplitField.m_uiFieldCapacity == m_oTSplitField.m_uiFieldSize )
            	    	{
                	    	if( !GrowSplitField( ) )
                    		{
                        		m_oTSplitField.m_uiFieldSize = 0;
                        		return false;
                    		}
                		}
                	}
            	}
           	} while( NULL != pCur );
            return true;
        }

        bool TMdbNtcSplit::SplitData( const char *sSplitString, int iLength, const char cSplitChar, bool bSkipEmptyField, bool bReplaceSplitChar )
        {
            m_oTSplitField.m_uiFieldSize = 0;
            if( ( NULL == sSplitString )
             || ( iLength < 1 ) )
            {
                return false;
            }
            if( iLength > (int)m_uiBufferLength )
            {
                if( !GrowBuffer( iLength ) )
                    return false;
            }
            m_cSplitChar = cSplitChar;
            memcpy( m_pBuffer, sSplitString, (MDB_UINT32)iLength );
            m_pBuffer[ iLength ] = '\0';

			char *pCur = m_pBuffer;
			char *pPre = pCur;
            int iOffset = 0;
            do
            {
                if( *pCur == cSplitChar )
                {
                	if( bReplaceSplitChar )
                		*pCur = '\0';
                	if( pCur != pPre || !bSkipEmptyField )
            		{
	                	m_oTSplitField.m_ppFieldValue[ m_oTSplitField.m_uiFieldSize ] = pPre;
	                	m_oTSplitField.m_piFieldValueLength[ m_oTSplitField.m_uiFieldSize++ ] = (int)(pCur - pPre);
        	        	if( m_oTSplitField.m_uiFieldCapacity == m_oTSplitField.m_uiFieldSize )
            	    	{
                	    	if( !GrowSplitField( ) )
                    		{
                        		m_oTSplitField.m_uiFieldSize = 0;
                        		return false;
                    		}
                		}
            		}
					pPre = ++pCur;
                }
                else
                {
                	pCur++;
            	}
            	iOffset++;
            } while( iOffset < iLength );
            if( pCur != pPre || !bSkipEmptyField )
            {
	            m_oTSplitField.m_ppFieldValue[ m_oTSplitField.m_uiFieldSize ] = pPre;
	            m_oTSplitField.m_piFieldValueLength[ m_oTSplitField.m_uiFieldSize++ ] = (int)(pCur - pPre);
        	    if( m_oTSplitField.m_uiFieldCapacity == m_oTSplitField.m_uiFieldSize )
            	{
            		if( !GrowSplitField( ) )
                    {
                    	m_oTSplitField.m_uiFieldSize = 0;
                        return false;
                    }
                }
            }
            return true;
        }

        unsigned int TMdbNtcSplit::GetFieldCount()
        {
            return m_oTSplitField.m_uiFieldSize;
        }

        const char * TMdbNtcSplit::Field( unsigned int iIndex )
        {
            if( iIndex > m_oTSplitField.m_uiFieldSize - 1 )
            {
                return NULL;
            }
            return m_oTSplitField.m_ppFieldValue[ iIndex ];
        }

        const char * TMdbNtcSplit::operator []( unsigned int iIndex )
        {
        	return Field( iIndex );
        }

        int TMdbNtcSplit::GetFieldValueLength( unsigned int iIndex )
        {
            if( iIndex > m_oTSplitField.m_uiFieldSize - 1 )
            {
                return -1;
            }
            return m_oTSplitField.m_piFieldValueLength[ iIndex ];
        }

        bool TMdbNtcSplit::GrowBuffer( int iLength )
        {
            int iBufferLength = 0;
            int iCount = iLength / MDB_NTC_ZS_SPLIT_INCR_BUFFER_LENGTH;
            if ( 0 == iLength % MDB_NTC_ZS_SPLIT_INCR_BUFFER_LENGTH )
            {
                iBufferLength = iCount * MDB_NTC_ZS_SPLIT_INCR_BUFFER_LENGTH;
            }
            else
            {
                iBufferLength = ( iCount + 1 ) * MDB_NTC_ZS_SPLIT_INCR_BUFFER_LENGTH;
            }
            if (NULL != m_pBuffer)
            {
                delete [] m_pBuffer;
                m_pBuffer = NULL;
            }
            m_pBuffer = new char [ iBufferLength + 1 ];
            if( NULL == m_pBuffer )
            {
                return false;
            }
            m_uiBufferLength = (MDB_UINT32)iBufferLength;
            return true;
        }
        bool TMdbNtcSplit::GrowSplitField( )
        {
            int iSumCount = (int)m_oTSplitField.m_uiFieldCapacity + MDB_NTC_ZS_SPLIT_INCR_FIELD_COUNT;
            char ** pTmp = new char* [iSumCount];
            if( NULL == pTmp )
            {
                return false;
            }
            memcpy( pTmp, m_oTSplitField.m_ppFieldValue, m_oTSplitField.m_uiFieldSize * sizeof(char *) );
//            memset( pTmp + m_oTSplitField.m_uiFieldSize, 0, MDB_NTC_ZS_SPLIT_INCR_FIELD_COUNT * sizeof(char *) );
            m_oTSplitField.m_uiFieldCapacity = (MDB_UINT32)iSumCount;
            delete [] m_oTSplitField.m_ppFieldValue;
            m_oTSplitField.m_ppFieldValue = pTmp;

            int* pIntTmp = new int [iSumCount];
            if( NULL == pIntTmp )
            {
                return false;
            }
            memcpy( pIntTmp, m_oTSplitField.m_piFieldValueLength, m_oTSplitField.m_uiFieldSize * sizeof(int) );
//            memset( pIntTmp + m_oTSplitField.m_uiFieldSize, 0, MDB_NTC_ZS_SPLIT_INCR_FIELD_COUNT * sizeof(int) );
//            m_oTSplitField.m_uiFieldCapacity = iSumCount;
            delete [] m_oTSplitField.m_piFieldValueLength;
            m_oTSplitField.m_piFieldValueLength = pIntTmp;

            return true;
        }

/////////////////////////////////////////////////////////////////////////////////
/*
        static mdb_ntc_thread_local(TMdbNtcStringBuffer*, gs_pSplitBuffer);
        TQuickSplit::TQuickSplit()
        {
            m_pszBuffer = NULL;
            m_cDelimiter = '\0';
            m_iFieldCount       =   0;
            m_iFieldArraySize   =    10;
            m_pFieldPosArray    =   new TFieldPos[m_iFieldArraySize];
        }

        TQuickSplit::~TQuickSplit()
        {
            if(m_pFieldPosArray == NULL)
            {
                delete []m_pFieldPosArray;
                m_pFieldPosArray = NULL;
            }
        }

        int TQuickSplit::Split(const char* pszBuffer, int iLength, char cDelimiter, bool bSkipEmptyField )
        {
            m_pszBuffer = pszBuffer;
            m_cDelimiter = cDelimiter;
            m_iFieldCount = 0;
            if(pszBuffer == NULL || iLength == 0 || (iLength == -1 && *pszBuffer == '\0')) return 0;
            const char* p = pszBuffer;
            int iStart = 0, iOffset = 0;
            do
            {
                if(*p == cDelimiter)
                {
                    if(iOffset != iStart || !bSkipEmptyField)
                    {
                        if(m_iFieldCount >= m_iFieldArraySize)
                        {
                            TFieldPos* pFieldPosArray = new TFieldPos[(m_iFieldCount+1)*2];
                            if(m_pFieldPosArray)
                            {
                                memcpy(pFieldPosArray, m_pFieldPosArray, m_iFieldArraySize);
                                delete []m_pFieldPosArray;
                                m_pFieldPosArray = NULL;
                            }
                            m_iFieldArraySize = (m_iFieldCount+1)*2;
                            m_pFieldPosArray = pFieldPosArray;
                        }
                        m_pFieldPosArray[m_iFieldCount].iStart  =   iStart;
                        m_pFieldPosArray[m_iFieldCount].iEnd    =   iOffset;
                        ++m_iFieldCount;
                    }
                    iStart = iOffset+1;
                }
                ++p;
                ++iOffset;
            } while ((iLength == -1 && *p != '\0') || iOffset<iLength);
            if(iOffset != iStart || !bSkipEmptyField)
            {
                if(m_iFieldCount >= m_iFieldArraySize)
                {
                    TFieldPos* pFieldPosArray = new TFieldPos[(m_iFieldCount+1)*2];
                    if(m_pFieldPosArray)
                    {
                        memcpy(pFieldPosArray, m_pFieldPosArray, m_iFieldArraySize);
                        delete []m_pFieldPosArray;
                        m_pFieldPosArray = NULL;
                    }
                    m_iFieldArraySize = (m_iFieldCount+1)*2;
                    m_pFieldPosArray = pFieldPosArray;
                }
                m_pFieldPosArray[m_iFieldCount].iStart  =   iStart;
                m_pFieldPosArray[m_iFieldCount].iEnd    =   iOffset;
                ++m_iFieldCount;
            }
            return m_iFieldCount;
        }

        TMdbNtcStringBuffer TQuickSplit::Field(unsigned int uiIndex)
        {
            MDB_NTC_ZF_ASSERT(uiIndex < m_iFieldCount);
            if(gs_pSplitBuffer == NULL)
            {
                gs_pSplitBuffer = new TMdbNtcStringBuffer;
                mdb_ntc_zthread_cleanup_push(gs_pSplitBuffer.Value());
            }
            return gs_pSplitBuffer->Assign(m_pszBuffer+m_pFieldPosArray[uiIndex].iStart, m_pFieldPosArray[uiIndex].iEnd-m_pFieldPosArray[uiIndex].iStart);
        }

        TQuickSplit::TFieldPos* TQuickSplit::FieldPos(unsigned int uiIndex)
        {
            if(m_pszBuffer == NULL || uiIndex >= m_iFieldCount)
            {
                return NULL;
            }
            else
            {
                return &m_pFieldPosArray[uiIndex];
            }
        }
*/
//}
