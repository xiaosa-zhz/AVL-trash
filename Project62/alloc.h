#pragma once
#include <concepts>
//#include <memory>

template<std::size_t elem_size>
class MemoryBuffer
{
public:
	MemoryBuffer() : elem_count(0) {}
	MemoryBuffer(std::size_t init_elem_count) : elem_count(init_elem_count) {}

private:
	std::size_t elem_count;

	union MemoryBlock
	{
		MemoryBlock* next;
		std::byte[elem_size];
	};

	MemoryBlock* begin;
	MemoryBlock* end;

	struct AllocBlock
	{
		AllocBlock* next;
		MemoryBlock* memory_block_array;
	private:
		AllocBlock(std::size_t size) : next(nullptr), memory_block_array(new MemoryBlock[size])
		{
			MemoryBlock* iter = memory_block_array;
			MemoryBlock* end = memory_block_array + size;
			while (iter != end)
			{
				
				++iter;
			}
		}
	public:
		
		static void allocate_new_block(AllocBlock& prev, std::size_t size)
		{

		}
	};

	
};