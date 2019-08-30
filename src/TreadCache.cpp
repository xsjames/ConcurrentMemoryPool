#include "ThreadCache.h"

//����ռ�
void* ThreadCache::Allocate(size_t size)
{
	//Ԥ����ʩ����ֹ��Ҫ�������ڴ���ڿ��Ը���������С
	assert(size <= MAXBYTES);

	//�����û���Ҫsize�Ĵ�С����ThreadCacheӦ�ø������ڴ���С
	size = ClassSize::Roundup(size);

	//�����ڴ��Ĵ�С������ڴ�����������������±�
	size_t index = ClassSize::Index(size);
	FreeList& freelist = _freelist[index];
	if (!freelist.Empty())
	{
		//��ʾ�ô������������º��п��õ��ڴ��
		return freelist.Pop();
	}

	//�ߵ��ô���˵������������û�п��õ��ڴ��
	return FetchFromCentralCache(index, size);
}

//��Centralcache����һ��������Ŀ���ڴ��
void* ThreadCache::FetchFromCentralCache(size_t index, size_t byte)
{
	assert(byte <= MAXBYTES);
	FreeList& freelist = _freelist[index];	//�õ����ĸ�freelist��Ҫ��ȡ�ڴ��
	size_t num = 10;	//��Ҫ��CentralCache�õ����ڴ��ĸ���

	void *start, *end;	//����õ����ڴ�	fetchnum��ʾ��ʵ�õ����ڴ�����
	size_t fetchnum = CentralCache::GetInstance()->FetchRangeObj(start, end, num, byte);
	if (fetchnum == 1)
	{
		//���ֻ��CentralCache�õ�һ��Ͳ��ý�ʣ����ڴ�����������������
		return start;
	}

	freelist.PushRange(NEXT_OBJ(start), end, fetchnum - 1);
	return start;
}

//�ͷ��ڴ��
void ThreadCache::Deallocate(void* ptr)
{
	//�õ�����ptrָ���span
	Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);

	if (span->_objsize > MAXBYTES)
	{
		//���Ҫ������Span��������ֽڴ�С����ֱ�ӷ�����PageCache
		PageCache::GetInstance()->TakeSpanToPageCache(span);
		return;
	}
	FreeList& freelist = _freelist[ClassSize::Index(span->_objsize)];

	//���ڴ��ͷ��
	freelist.Push(ptr);

	//����10�������� ��ʼ���ڴ淵��������CentralCache
	if (freelist.Size() >= freelist.MaxSize())
	{
		ReturnToCentralCache(freelist);
	}
}

void ThreadCache::ReturnToCentralCache(FreeList &freelist)
{
	void* start = freelist.Clear();
	CentralCache::GetInstance()->ReturnToCentralCache(start);
}