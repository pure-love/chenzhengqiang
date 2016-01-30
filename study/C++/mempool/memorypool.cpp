/*
*@learner:chenzhengqiang
*@learning date:2016/1/24
*@desc:about the memory pool
*@origin:<<Efficient C++;Performance Programming Technique>>
*/


template<class T>
class MemoryPool
{
	public:
		MemoryPool(size_t size = EXPANSION_SIZE);
		~MemoryPool();

		inline void *alloc(size_t size);
		inline void free(void *someElement);

	private:
		MemoryPool<T> *next;
		enum {EXPANSION_SIZE = 32};
		void expandTheFreeList(int howMany = EXPANSION_SIZE);
};


template<class T>
MemoryPool<T>::MemoryPool (size_t size)
{
	expandTheFreeList(size);
}


template<class T>
MemoryPool<T>::~MemoryPool ()
{
	MemoryPool<T> *nextPtr = next;
	for (nextPtr = next; nextPtr != 0; nextPtr= next)
	{
		next = next->next;
		delete [] nextPtr;
	}
}


template<class T>
inline void *MemoryPool<T> ::alloc(size_t size)
{
	if ( ! next )
	{
		expandTheFreeList();
	}

	MemoryPool<T> * head = next;
	next = next->next;
	return head;
}


template<class T>
inline void MemoryPool<T>:: free(void *doomed)
{
	MemoryPool<T> * head = static_cast<MemoryPool *>(doomed);
	head->next = next;
	next =head;
}


template<class T>
void MemoryPool<T>::expandTheFreeList(int howMany)
{
	size_t size = (sizeof(T) > sizeof(MemoryPool<T>*)) ? sizeof(T):sizeof(MemoryPool<T> *);
	MemoryPool<T> * runner = static_cast<MemoryPool *>(new char[size]);
	next = runner;
	for ( int index = 0; index < howMany; ++index )
	{
		runner->next = static_cast<MemoryPool *>(new char[size]);
		runner = runner->next;
	}

	runner->next = 0;
}



class Rational
{
	public:
		Rational(int a =0, int b = 1):n(a), d(b){;}
		void *operator new(size_t size) { return memPool->alloc(size);}
		void operator delete(void *doomed, size_t size) { memPool->free(doomed);}
		static void newMemPool() { memPool = new MemoryPool<Rational>;}
		static void deleteMemPool() {delete memPool;}
	private:
		int n;
		int d;
		static MemoryPool<Rational> *memPool;
};