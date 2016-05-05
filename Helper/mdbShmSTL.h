/****************************************************************************************
*@Copyrights  2013�����������Ͼ�����������޹�˾ �����ܹ�--QuickMDBС��
*@            All rights reserved.
*@Name��	    mdbShmSTL.h		
*@Description�� mdb�Ĺ����ڴ�����
*@Author:			jin.shaohua
*@Date��	    2013.3
*@History:
******************************************************************************************/
#ifndef _MDB_SHM_STL_H_
#define _MDB_SHM_STL_H_
#include "Helper/mdbShm.h"
#include "Helper/TMutex.h"

//namespace QuickMDB{
//�ڴ����ͷ
class TShmAllocHead
{
public:
	int Init()
	{
		m_iFreeOffSet = 0;
		m_iTotalSize   = 0;
              m_tMutex.Create();
		return 0;
	}
public:
	size_t m_iFreeOffSet;//����ƫ��
       size_t m_iTotalSize;//�ܴ�С
	TMutex m_tMutex;//������
	size_t m_iFirstInfoOffset;//��һ����Ϣ���offset;
};
//shm�ڴ����
class TShmAlloc
{
public:
	TShmAlloc():m_pShmHead(NULL){}
	//����
	int CreateShm(SMKey tKey,size_t iShmSize,SHAMEM_T & iShmID)
	{
		int iRet = 0;
		CHECK_RET(TMdbShm::Create(tKey,iShmSize,iShmID),"Create failed.");
		char * pAddr = NULL;
		CHECK_RET(TMdbShm::AttachByID(iShmID,pAddr),"AttachByID failed.");
		m_pShmHead = (TShmAllocHead *)pAddr;
		m_pShmHead->Init();//
		m_pShmHead->m_iFreeOffSet = sizeof(TShmAllocHead);
		m_pShmHead->m_iTotalSize  = iShmSize;
		return iRet;
	}
        int AttachByID(SHAMEM_T iShmID,char *&pAddr)
        {
            int iRet = 0;
            CHECK_RET(TMdbShm::AttachByID(iShmID,pAddr),"AttachByID failed.");
            m_pShmHead = (TShmAllocHead *)pAddr;
            return iRet;
        }
	//attach
    int AttachByKey(SMKey tKey,char *&pAddr)
	{
		int iRet = 0;
		CHECK_RET(TMdbShm::AttachByKey(tKey,pAddr),"AttachByKey failed.");
		m_pShmHead = (TShmAllocHead *)pAddr;
		return iRet;
	}
	//�����ڴ�
    void  * Allocate(size_t iSize,size_t & iOffSet)
	{
		if(NULL == m_pShmHead){return NULL;}
		if(m_pShmHead->m_iFreeOffSet + iSize > m_pShmHead->m_iTotalSize)
		{//�ռ䲻��
			return NULL;
		}
		iOffSet = m_pShmHead->m_iFreeOffSet;
		m_pShmHead->m_iFreeOffSet += iSize;
		return ((char *)m_pShmHead) +iOffSet;
	}
    char * GetStartAddr(){
		return (char *)m_pShmHead;
	}
    //���ṩdeallocate
public:
	TShmAllocHead * m_pShmHead;
};

//��¼�ڵ�
template <class T>
class TShmListNode
{
public:
    typedef TShmListNode<T> list_node;
    typedef list_node * link_type;
public:
    TShmListNode(){}
	void Init(){
		m_iPrevPos = 0;
		m_iNextPos = 0;
		m_iCurPos  = 0;
	}
    link_type GetNext(char * pStartAddr){//��ȡ��һ��
        if(m_iNextPos <= 0){
            return NULL;
       }else{
            return (link_type)(pStartAddr + m_iNextPos);
       }
    }
    link_type GetPrev(char * pStartAddr){//��ȡǰһ��
       if(m_iPrevPos <= 0){
            return NULL;
       }else{
            return (link_type)(pStartAddr + m_iPrevPos);
       }
    }
    void SetNext(link_type  pNext){//����next ֵ
        if(NULL == pNext){
            m_iNextPos = 0;
        }else{
            m_iNextPos = pNext->m_iCurPos;
        }
        
    }
    void SetPrev(link_type  pPrev){//����prevֵ
        if(NULL == pPrev){
            m_iPrevPos = 0;
        }else{
            m_iPrevPos = pPrev->m_iCurPos;
        }
    }
public:
    size_t m_iPrevPos;//ǰһλ��
    size_t m_iNextPos;
    size_t m_iCurPos;//��ǰλ��
    T m_data;
};

//list ,interator
template<class T,class Ref,class Ptr>
class TShmListIterator
{
public:
    typedef TShmListIterator<T,T&,T*> iterator;
    typedef TShmListIterator<T, const T&, const T*> const_iterator;
    typedef TShmListIterator<T,Ref,Ptr> self;
    
    typedef T value_type;
    typedef Ptr pointer;
    typedef Ref reference;
    typedef TShmListNode<T>* link_type;
    typedef size_t size_type;

    link_type node;
    char * m_pStartAddr;//��ʼ��ַ
    TShmListIterator(link_type x,char * pStartAddr) : node(x),m_pStartAddr(pStartAddr) {}
    TShmListIterator(const iterator& x) : node(x.node),m_pStartAddr(x.m_pStartAddr) {}

    bool operator==(const self& x) const { return node == x.node; }
    bool operator!=(const self& x) const { return node != x.node; }
    reference operator*() const { return node->m_data; }
    pointer operator->() const { return &(operator*()); }
    self& operator++() { 
        node = (*node).GetNext(m_pStartAddr);
        return *this;
    }
    self operator++(int) { 
    self tmp = *this;
    ++*this;
    return tmp;
    }
    self& operator--() { 
        node = (*node).GetPrev(m_pStartAddr);
        return *this;
    }
    self operator--(int) { 
    self tmp = *this;
    --*this;
    return tmp;
    }
    //�Ƿ��ǿ�ֵ
    bool IsNULL()
    {
        return NULL == node;
    }
};
//�����ڴ��list
template <class T>
class TShmList
{
public:
    typedef TShmListNode<T> list_node;
public:      
    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef list_node* link_type;
public:
    typedef TShmListIterator<T,T&,T*> iterator;
    typedef TShmListIterator<T,const T&,const T*> const_iterator;
public:
	//���ӹ����ڴ�
	int Attach(TShmAlloc tAlloc,size_t & iOffset )
	{
	int iRet = 0;
	if(NULL == tAlloc.GetStartAddr())
	{
		CHECK_RET(ERR_OS_SHM_NOT_EXIST,"Attach failed.");
	}
	m_tAlloc = tAlloc;
	m_pStartAddr = tAlloc.GetStartAddr();
        if(0 == iOffset)
        {//�״�
            m_tAlloc.Allocate(sizeof(list_node)*2,iOffset);
            m_HeadNode = (link_type)(m_pStartAddr+iOffset); 
            m_HeadNode->Init();
            m_HeadNode->m_iCurPos = iOffset;
            m_HeadNode->SetNext(m_HeadNode);
            m_HeadNode->SetPrev(m_HeadNode);
			//free��
            m_FreeNode  = (link_type)(m_pStartAddr+iOffset + sizeof(list_node));
            m_FreeNode->Init();
            m_FreeNode->m_iCurPos  = iOffset + sizeof(list_node);
        }
        if(iOffset <= 0)
        {//error
            return -1;
        }
        m_HeadNode = (link_type)(m_pStartAddr+iOffset); 
        m_FreeNode  = (link_type)(m_pStartAddr+iOffset + sizeof(list_node));
	return iRet;
	}
protected:
    link_type m_HeadNode;//ͷ�ڵ�
    link_type m_FreeNode;//������
    TShmAlloc m_tAlloc;//�ڴ����
    char * m_pStartAddr;//��ʼ��ַ
protected:
	//��ȡ�½ڵ�
    link_type get_node()
	{
        //���ȴӿ�������ȡ
        link_type pFreeNode = m_FreeNode->GetNext(m_pStartAddr);
        if(NULL != pFreeNode)
        {//find
            m_FreeNode->SetNext(pFreeNode->GetNext(m_pStartAddr));
            pFreeNode->SetNext(NULL);
	     pFreeNode->SetPrev(NULL);
        }
        else
        {//not find
            size_t iOffset = 0;
            pFreeNode = (link_type)m_tAlloc.Allocate(sizeof(list_node),iOffset);
            if(NULL == pFreeNode)
            {
                return NULL;
            }
            pFreeNode->Init();
            pFreeNode->m_iCurPos = iOffset;
        }
        return pFreeNode;
    }
	//���õ���������
    void put_node(link_type nodeToPut)
	{
        nodeToPut->SetPrev(NULL);
		nodeToPut->SetNext(m_FreeNode->GetNext(m_pStartAddr));
		m_FreeNode->SetNext(nodeToPut);
    }
    link_type create_node(const T& x)
    {
        link_type p = get_node();
        if(NULL == p){return NULL;}
        p->m_data = x;
        return p;
    }
    //destroy_node
protected:
public:
	TShmList()
	{
		m_HeadNode = NULL;
		m_FreeNode = NULL;
		m_pStartAddr = NULL;
	}
	//��ʼ
    iterator begin()
	{
		return iterator(m_HeadNode->GetNext(m_pStartAddr),m_pStartAddr);
	}
	//����
    iterator end()
	{
		return iterator(m_HeadNode,m_pStartAddr);
	}
	//list��С
	size_t size()
	{
		size_t result = 0;
		iterator itorB = begin();
		iterator itorE = end();
		for(;itorB != itorE; ++itorB)
		{
			result ++;
		}
		return result;
	}
	//�Ƿ�Ϊ��
    bool empty()const{
		return m_HeadNode->GetNext(m_pStartAddr) == m_HeadNode;
	}
	//���
	void clear()
	{
		iterator itorB = begin();
		iterator itorE = end();
		for(;itorB != itorE;)
		{
			itorB = erase(itorB);
		}
	}
	//������ֵ
    iterator insert(iterator iPosition,const T& x)
    {
        link_type tmp = create_node(x);
        if(NULL == tmp){return end();}//���Ϊ�վͷ���end
        tmp->SetNext(iPosition.node);
        tmp->SetPrev(iPosition.node->GetPrev(m_pStartAddr));
        link_type tmpPrev = iPosition.node->GetPrev(m_pStartAddr);
        tmpPrev->SetNext(tmp);
	 iPosition.node->SetPrev(tmp);
        return iterator(tmp,m_pStartAddr);
    }
	
    iterator insert(iterator position) { return insert(position, T()); }

    void push_front(const T& x) { insert(begin(), x); }
    void push_back(const T& x) { insert(end(), x); }
    iterator erase(iterator position) {
		if(position == end())
		{
			return position;
		}
        link_type next_node = position.node->GetNext(m_pStartAddr);
        link_type prev_node = position.node->GetPrev(m_pStartAddr);
        prev_node->SetNext(next_node);
        next_node->SetPrev(prev_node);
        put_node(position.node);//�黹node
        return iterator(next_node,m_pStartAddr);
  }
  void pop_front() { erase(begin()); }
  void pop_back() { 
    iterator tmp = end();
    erase(--tmp);
  }
};

//}

#endif
