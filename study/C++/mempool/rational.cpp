/*
*@learner:chenzhengqiang
*@learning date:2016/1/24
*@desc:about the memory pool
*@origin:<<Efficient C++;Performance Programming Technique>>
*/

class NextOnFreeList
{
	public:
		NextOnFreeList *next;
};

class Rational
{
	public:
		Rational (int a=0, int b=1):n(a),d(b){}
		inline void *operator new(size_t size);
		inline void operator delete(void *doomed, size_t size);
		static void newMemPool(){ expandTheFreeList();}
		static void deleteMemPool();
	private:
		static NextOnFreeList *freeList;
		static void expandTheFreeList();
		enum {EXPANSION_SIZE=32};

		int n;
		int d;
};


NextOnFreeList * Rational::freeList = 0;
inline void * Rational::operator new(size_t size)
{
	if ( 0 == freeList )
	{
		expandTheFreeList();
	}

	NextOnFreeList *head = freeList;
	freeList = freeList->next;
	return head;
}


inline void Rational::operator delete(void *doomed, size_t size)
{
	NextOnFreeList *head = static_cast<NextOnFreeList *>(doomed);
	head->next = freeList;
	freeList = head;
}


void Rational::expandTheFreeList()
{
	size_t size = (sizeof(Rational) > sizeof(NextOnFreeList *)) ?
				sizeof(Rational) : sizeof(NextOnFreeList *);
	NextOnFreeList *runner = static_cast<NextOnFreeList *> (new char[size]);
	freeList = runner;

	for ( int index = 0; index < EXPANSION_SIZE; ++index )
	{
		runner->next = static_cast<NextOnFreeList *>(new char [size]);
		runner = runner->next;
	}

	runner->next = 0;
}


void Rational::deleteMemPool()
{
	NextOnFreeList *nextPtr;
	for (nextPtr = freeList; nextPtr != 0; nextPtr = freeList)
	{
		freeList = freeList->next;
		delete [] nextPtr;
	}
}