#pragma once
#include <concepts>
#include <memory>

template<std::size_t elem_size>
class MemoryBuffer
{
private:
	std::size_t elem_count;

	union MemoryBlock
	{
		MemoryBlock* next;
		std::byte buffer[elem_size];
	};

	MemoryBlock* begin{ nullptr };
	MemoryBlock* end{ nullptr };

	struct AllocBlock
	{
		AllocBlock* next;
		const std::size_t size;
		MemoryBlock* memory_block_array;
	private:
		AllocBlock(std::size_t size) : next(nullptr), size(size), memory_block_array(new MemoryBlock[size])
		{
			MemoryBlock* iter = memory_block_array;
			MemoryBlock* last = get_last();
			while (iter != last)
			{
				iter->next = iter + 1;
				++iter;
			}
			iter->next = nullptr;
		}

	public:
		~AllocBlock()
		{
			delete[] memory_block_array;
			delete next;
		}

		MemoryBlock* get_last()
		{
			return memory_block_array + size - 1;
		}
		
		static AllocBlock* allocate_new_block(
			std::size_t size,
			MemoryBlock** prev_next)
		{
			AllocBlock* new_block = new AllocBlock(size);
			*prev_next = new_block->memory_block_array;
			return new_block;
		}
	};

	AllocBlock* head{ nullptr };

public:
	MemoryBuffer() : elem_count(0)
	{

	}

	MemoryBuffer(std::size_t init_elem_count) : elem_count(init_elem_count)
	{
		head = AllocBlock::allocate_new_block(init_elem_count, &begin);
		end = head->get_last();
	}
};

int main()
{
	MemoryBuffer<8> b{ 8 };
}