/*
*@learner:chenzhengqiang
*@learning date:2016/1/30
*@desc:about the memory pool
*@origin:<<Efficient C++;Performance Programming Technique>>
*/


class MemoryChunk
{
	public:
		MemoryChunk(MemoryChunk *next, size_t chunkSize);
		~MemoryChunk() { delete [] mem_; }

		inline void *alloc(size_t requestSize);
		inline void free(void * doomed);

		//retrieve the pointer of next memory chunk
		MemoryChunk * nextMemChunk() { return next_; }

		//the available size of current memory chunk
		size_t spaceAvailable() { return chunkSize_ - bytesAlreadyAllocated_; }

		//the default memory chunk's size
		enum{DEFAULT_CHUNK_SIZE = 4096};
	private:
		MemoryChunk *next_;
		void * mem_;

		//the default memory chunk's size 
		size_t chunkSize_;

		//the current allocated chunk size
		size_t bytesAlreadyAllocated_;
};



MemoryChunk :: MemoryChunk(MemoryChunk * next,size_t chunkSize)
{
	chunkSize_ = ( DEFAULT_CHUNK_SIZE > chunkSize ) ? DEFAULT_CHUNK_SIZE:chunkSize;
	next_ = next;
	bytesAlreadyAllocated_ = 0;
	mem_ = new char[chunkSize_];
}



MemoryChunk :: ~MemoryChunk()
{
	delete [] mem_;
}



void * MemoryChunk :: alloc(size_t requestSize)
{
	void *address = static_cast<void *>(static_cast<size_t>(mem_+bytesAlreadyAllocated_));
	bytesAlreadyAllocated_ += requestSize;
	return address;
}


inline void MemoryChunk :: free( void *doomed) {}



class ByteMemoryPool
{
	public:
		ByteMemoryPool (size_t initSize = MemoryChunk::DEFAULT_CHUNK_SIZE);
		~ByteMemoryPool();

		//allocate memory from private memory pool
		inline void * alloc(size_t size);

		inline void free(void *doomed);
	private:
		//our private memory pool list
		MemoryChunk *listOfMemoryChunks_;
		//expand our private memory pool
		void expandStorage(size_t requestSize);
};


ByteMemoryPool :: ByteMemoryPool(size_t initSize)
{
	expandStorage(initSize);
}


ByteMemoryPool :: ~ByteMemoryPool()
{
	MemoryChunk *memoryChunk = listOfMemoryChunks_;
	while (memoryChunk)
	{
		listOfMemoryChunks_ = listOfMemoryChunks_->next;
		delete memoryChunk;
		memoryChunk = listOfMemoryChunks_;
	}
}


void * ByteMemoryPool :: alloc(size_t requestSize)
{
	size_t space = listOfMemoryChunks_->spaceAvailable();
	if ( space < requestSize )
	{
		expandStorage(requestSize);
	}
	return listOfMemoryChunks_->alloc(requestSize);
}


inline void ByteMemoryPool :: free(void *doomed)
{
	listOfMemoryChunks_->free(doomed);
}

void ByteMemoryPool :: expandStorage(size_t requestSize)
{
	listOfMemoryChunks_ = new MemoryChunk(listOfMemoryChunks_, requestSize);
}