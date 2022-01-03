#include <concepts>
#include <memory>
#include <iostream>
#include <optional>
#include <exception>
#include <tuple>
#include <cmath>
#include <random>
#include <chrono>
#include <vector>
#include <stack>
#include <map>
#include <unordered_map>
#include <atomic>
#include <memory_resource>

//std::stack<std::tuple<int, bool>> remove_callstack;
//std::stack<int> DFS_callstack;
//std::pmr::monotonic_buffer_resource memory_buffer{ 1073741824 };

//#define private public

//concept std::like in C++23
template<typename T, typename U>
concept is_cvref_t_of = std::is_same_v<U, std::remove_cvref_t<T>>;

#define UNREACHABLE_BUILDIN_WRAPPER __assume(false)
#define assert(expression) do { if(!(expression)) { throw std::exception{ "WTF" }; } } while(false)

//#define PMR_ENABLE
#ifdef PMR_ENABLE
std::pmr::monotonic_buffer_resource memory_buffer{ 65536 };
#endif // PMR_ENABLE

template<typename _KeyTy, typename _ValTy>
requires std::totally_ordered<_KeyTy>
class AVL
{
public:
	using key_type = _KeyTy;
	using value_type = _ValTy;
private:
#define FWD(FORWARD_REF) std::forward<decltype(FORWARD_REF)>(FORWARD_REF)

	enum class is_height_updated : bool
	{
		HEIGHT_UPDATE_NO_NEED = false, HEIGHT_UPDATE_NEEDED = true
	};

	class _Node
	{
	public:
		value_type _value;
		std::unique_ptr<_Node> _child_left;
		std::unique_ptr<_Node> _child_right;
		key_type _key;
		long long _height = 1;

		_Node(is_cvref_t_of<key_type> auto&& _new_key,
			is_cvref_t_of<value_type> auto&& _new_value) : 
			_key(FWD(_new_key)),
			_value(FWD(_new_value)),
			_child_left(nullptr), _child_right(nullptr) {}

		_Node(_Node&& _old_node,
			is_cvref_t_of<value_type> auto&& _new_value) :
			_key(std::move(_old_node._key)), 
			_child_left(std::move(_old_node._child_left)), 
			_child_right(std::move(_old_node._child_right)), 
			_value(FWD(_new_value)), _height(_old_node._height) {}

		is_height_updated _update_height()
		{
			auto new_height = 1 + std::max(_get_height(_child_left), _get_height(_child_right));
			if (new_height == _height) {
				return is_height_updated::HEIGHT_UPDATE_NO_NEED;
			} else {
				_height = new_height;
				return is_height_updated::HEIGHT_UPDATE_NEEDED;
			}
		}

		long long _get_balance()
		{
			return _get_height(_child_right) - _get_height(_child_left);
		}

#ifdef PMR_ENABLE
		void* operator new(size_t size)
		{
			return memory_buffer.allocate(size);
		}

		void operator delete(void* memory, size_t size)
		{
			memory_buffer.deallocate(memory, size);
		}
#endif // PMR_ENABLE
	};

	static long long _get_height(std::unique_ptr<_Node>& child)
	{
		return child ? (child->_height) : 0;
	}

#define AVL_ROTATE_OPERATION(COUNTER_ROTATE_DIRECTION, ROTATE_DIRECTION)    \
    auto new_root = std::move(old_root->_child_##COUNTER_ROTATE_DIRECTION); \
    auto temp_hold = std::move(new_root->_child_##ROTATE_DIRECTION);        \
    old_root->_child_##COUNTER_ROTATE_DIRECTION = std::move(temp_hold);     \
    old_root->_update_height();                                             \
    new_root->_child_##ROTATE_DIRECTION = std::move(old_root);              \
    new_root->_update_height();                                             \
    return new_root

	static std::unique_ptr<_Node> _right_rotate(std::unique_ptr<_Node> old_root)
	{
		AVL_ROTATE_OPERATION(left, right);
	}

	static std::unique_ptr<_Node> _left_rotate(std::unique_ptr<_Node> old_root)
	{
		AVL_ROTATE_OPERATION(right, left);
	}

#undef AVL_ROTATE_OPERATION

	std::unique_ptr<_Node>& _find_impl(const key_type& key)
	{
		//Not yet got a smart pointer with 'view/reference semantic'
		//Also miss a rebindable & nullable reference_warpper
		//std::unique_ptr<_Node>* iter = &(this->_head);
		//while (*iter)
		//{
		//	const key_type& key_now = (*iter)->_key;
		//	if (key == key_now) { return *iter; }
		//	if (key > key_now) { iter = &((*iter)->_child_right); }
		//	else { iter = &((*iter)->_child_left); }
		//}
		//return *iter;

		//A weird way to make std::reference_warpper rebindable & nullable
		std::optional iter = std::ref(this->_head);
		auto deref = [](auto& i) -> std::unique_ptr<_Node>& { return i.value().get(); };
		while (deref(iter))
		{
			const key_type& key_now = deref(iter)->_key;
			if (key == key_now) { return deref(iter); }
			if (key > key_now) { iter.emplace(deref(iter)->_child_right); }
			else { iter.emplace(deref(iter)->_child_left); }
		}
		return deref(iter);
	}

	static void _rebalance(std::unique_ptr<_Node>& root)
	{
		auto balance = root->_get_balance();
		std::optional child = std::ref(root);
		auto deref = [](auto& i) -> std::unique_ptr<_Node>&{ return i.value().get(); };
		unsigned char _case = 0;
		constexpr char LL = 0b00000000;
		constexpr char LR = 0b00000001;
		constexpr char RL = 0b00000010;
		constexpr char RR = 0b00000011;

		if (balance > 1) {
			_case |= 0b00000010;
			child = std::ref(root->_child_right);
		} else {
			child = std::ref(root->_child_left);
		}

		auto other_balance = deref(child)->_get_balance();
		if (other_balance > 0) {
			_case |= 0b00000001;
		}

		switch (_case)
		{
		case LR:
			deref(child) = std::move(_left_rotate(std::move(deref(child))));
			[[fallthrough]];
		case LL:
			root = std::move(_right_rotate(std::move(root)));
			break;
		case RL:
			deref(child) = std::move(_right_rotate(std::move(deref(child))));
			[[fallthrough]];
		case RR:
			root = std::move(_left_rotate(std::move(root)));
			break;
		}

		root->_update_height();
	}

	static is_height_updated _further_update(std::unique_ptr<_Node>& root)
	{
		auto balance = root->_get_balance();
		if (balance > -2 && balance < 2) { //-1, 0, 1 rebalance no need
			return root->_update_height();
		}

		//need rebalance
		auto old_height = _get_height(root);
		_rebalance(root);
		//if height is changed, tell parent node that it also needs some check
		if (old_height != _get_height(root)) { return is_height_updated::HEIGHT_UPDATE_NEEDED; }
		else { return is_height_updated::HEIGHT_UPDATE_NO_NEED; }
	}

	is_height_updated _push_impl(
		std::unique_ptr<_Node>& root,
		is_cvref_t_of<key_type> auto&& key,
		is_cvref_t_of<value_type> auto&& value)
	{
		if (!root) { //no one is here, so here is the home of new node
			root = std::make_unique<_Node>(FWD(key), FWD(value));
			++_size;
			return is_height_updated::HEIGHT_UPDATE_NEEDED;
		}

		const key_type& key_now = root->_key;
		if (key == key_now) { //key has existed, reuse the node and give it new value
			root = std::make_unique<_Node>(std::move(*root), FWD(value));
			return is_height_updated::HEIGHT_UPDATE_NO_NEED;
		}

		[[likely]];
		//not yet find the right place
		{
			//go deeper to one of the children. if height of child remains the same, so does parent's
			is_height_updated is_further_update_needed = is_height_updated::HEIGHT_UPDATE_NO_NEED;
			if (key > key_now) {
				is_further_update_needed = _push_impl(root->_child_right, FWD(key), FWD(value));
			} else {
				is_further_update_needed = _push_impl(root->_child_left, FWD(key), FWD(value));
			}

			if (is_further_update_needed == is_height_updated::HEIGHT_UPDATE_NEEDED) {
				//height of child updated, need more update and check
				return _further_update(root);
			} else {
				return is_height_updated::HEIGHT_UPDATE_NO_NEED;
			}
		}
	}

	is_height_updated _remove_impl(
		std::unique_ptr<_Node>& root,
		is_cvref_t_of<key_type> auto&& key)
	{
		//DEBUG
		//bool marker = false;
		//struct Guard
		//{ 
		//	std::unique_ptr<_Node>& root;
		//	bool& marker;
		//	Guard(auto& root, auto& marker) : root(root), marker(marker) {}
		//	~Guard() { remove_callstack.push({ (root ? root->_key : 1919810), marker}); }
		//} guard(root, marker);


		if (!root) //fail to find a node to remove
		{
			[[unlikely]];
			return is_height_updated::HEIGHT_UPDATE_NO_NEED;
		}

		const key_type& key_now = root->_key;
		if (key == key_now) { //succeed to find the node needs removed
			std::unique_ptr<_Node> ready_to_release = std::move(root);
			--_size;
			//from now on, root should not be used until it get the new node to hold
			//old node will be released at the end of this if scope
			//use ready_to_release as former root

			//DEBUG
			//marker = true;
			//std::cout << ready_to_release->_key << std::endl;

			{
				if (!(ready_to_release->_child_right))
				{
					//if the removing node has no right child, replace it directly with its left child
					root = std::move(ready_to_release->_child_left);
				}
				else if (!(ready_to_release->_child_left))
				{
					//do the same thing with left child
					root = std::move(ready_to_release->_child_right);
				}
				else if (!(ready_to_release->_child_left->_child_right))
				{
					//deal with the situation when left child tree has no right child
					//this could not be handled by following part
					std::unique_ptr<_Node> temp = std::move(ready_to_release->_child_right);
					root = std::move(ready_to_release->_child_left);
					root->_child_right = std::move(temp);
				}
				else
				{
					//find the most right node on the left child tree and let it move to here
					//NOTE: in a well-maintained AVL tree, such node shall have a left child tree with 0 or 1 height
					std::unique_ptr<_Node> old_left_child_tree = std::move(ready_to_release->_child_left);
					std::unique_ptr<_Node> old_right_child_tree = std::move(ready_to_release->_child_right);
					std::unique_ptr<_Node> new_root;

					//lambda definition
					auto find_most_right_child_and_replace = 
						[&new_root]
					(auto&& self, std::unique_ptr<_Node>& iter_root) -> is_height_updated
					{
						auto is_further_update_needed = is_height_updated::HEIGHT_UPDATE_NO_NEED;
						if (iter_root->_child_right) //not yet the most right node
						{
							is_further_update_needed = self(self, iter_root->_child_right);
						}
						else
						{
							//the most right node is this one
							//according to the NOTE, only need to replace it with its left child (null or not both OK)
							//and then this node can be used to replace 'the root'
							//assert(_get_height(iter_root->_child_left) <= 1);
							std::unique_ptr<_Node> temp = std::move(iter_root);
							iter_root = std::move(temp->_child_left);
							new_root = std::move(temp);
							//always update this node since it just got into a new place
							//always inform parents since its child has been replaced
							if (iter_root) //root could be set to nullptr, which need no update itself, just inform former nodes
							{
								_further_update(iter_root);
							}
							return is_height_updated::HEIGHT_UPDATE_NEEDED;
						}

						if (is_further_update_needed == is_height_updated::HEIGHT_UPDATE_NEEDED) {
							return _further_update(iter_root);
						} else {
							return is_height_updated::HEIGHT_UPDATE_NO_NEED;
						}
					};
					//lambda end

					find_most_right_child_and_replace(find_most_right_child_and_replace, old_left_child_tree);
					//after execution of this lambda, root becomes valid and reachable again
					new_root->_child_left = std::move(old_left_child_tree);
					new_root->_child_right = std::move(old_right_child_tree);
					root = std::move(new_root);
				}
			}

			//always update this node since it just got into a new place
			//always inform parents since its child has been replaced
			if (root) //root could be set to nullptr, which need no update itself, just inform former nodes
			{
				_further_update(root);
			}
			return is_height_updated::HEIGHT_UPDATE_NEEDED;
		}

		//not yet find the node
		{
			is_height_updated is_further_update_needed = is_height_updated::HEIGHT_UPDATE_NO_NEED;
			if (key > key_now) {
				is_further_update_needed = _remove_impl(root->_child_right, FWD(key));
			} else {
				is_further_update_needed = _remove_impl(root->_child_left, FWD(key));
			}

			if (is_further_update_needed == is_height_updated::HEIGHT_UPDATE_NEEDED) {
				return _further_update(root);
			} else {
				return is_height_updated::HEIGHT_UPDATE_NO_NEED;
			}
		}
	}

	std::unique_ptr<_Node> _head;
	std::size_t _size;

public:
	AVL() : _head(), _size(0) {}
	std::size_t size() { return _size; }

	[[nodiscard]]
	std::optional<std::reference_wrapper<value_type>> find(const key_type& key)
	{
		auto& result = _find_impl(key);
		if (result) { return result->_value; }
		else { return std::nullopt; }
	}

	AVL& insert(
		is_cvref_t_of<key_type> auto&& key,
		is_cvref_t_of<value_type> auto&& value)
	{
		_push_impl(this->_head, FWD(key), FWD(value));
		return *this;
	}

	AVL& erase(is_cvref_t_of<key_type> auto&& key)
	{
		_remove_impl(this->_head, FWD(key));
		return *this;
	}

	//DEBUG
	void DFS_impl(std::unique_ptr<_Node>& root)
	{
		if (root == nullptr) return;
		//DFS_callstack.push(root->_key);
		DFS_impl(root->_child_left);
		DFS_impl(root->_child_right);
		//check height
		try
		{
			assert(root->_update_height() == is_height_updated::HEIGHT_UPDATE_NO_NEED);
		}
		catch (...)
		{
			/*auto& DFScs = DFS_callstack._Get_container();
			auto& removecs = remove_callstack._Get_container();
			while (!remove_callstack.empty())
			{
				auto& call = remove_callstack.top();
				std::cout << std::get<int>(call) << ' ' << std::get<bool>(call) << '\n';
				remove_callstack.pop();
			}
			std::cout << '\n';
			while (!DFS_callstack.empty())
			{
				auto DFScall = DFS_callstack.top();
				std::cout << DFScall << '\n';
				DFS_callstack.pop();
			}
			std::cout << std::flush;*/
			throw;
		}
		//DFS_callstack.pop();
	}

	//DEBUG
	void DFS_debug_check()
	{
		DFS_impl(this->_head);
	}

	void another_DFS_impl(std::unique_ptr<_Node>& root, long long depth)
	{
		static long long former_depth = -1;
		if (root == nullptr)
		{
			if (former_depth == -1)
			{
				[[unlikely]];
				former_depth = depth;
			}
			else
			{
				auto diff = depth - former_depth;
				assert(diff >= -1 && diff <= 1);
				former_depth = depth;
			}
			return;
		}
		another_DFS_impl(root->_child_left, depth + 1);
		another_DFS_impl(root->_child_right, depth + 1);
	}

	//DEBUG
	void yet_another_DFS_debug_check()
	{
		std::cout.sync_with_stdio(false);
		another_DFS_impl(_head, 1);
	}
};

#undef FWD

//#define TEST constexpr auto
//
//template<typename key_type, typename value_type>
//class TestNode
//{
//public:
//	value_type _value;
//	char _balance = 0;
//	std::unique_ptr<TestNode> _child_left;
//	std::unique_ptr<TestNode> _child_right;
//	key_type _key;
//
//	TestNode(is_cvref_t_of<key_type> auto&& _new_key, is_cvref_t_of<value_type> auto&& _new_value) :
//		_key(FWD(_new_key)),
//		_value(FWD(_new_value)),
//		_child_left(), _child_right() {}
//
//	TestNode(TestNode&& _old_node, is_cvref_t_of<value_type> auto&& _new_value) :
//		_key(std::move(_old_node._key)),
//		_child_left(std::move(_old_node._child_left)),
//		_child_right(std::move(_old_node._child_right)),
//		_value(FWD(_new_value)) {}
//};

//void DFSMF(auto&& root)
//{
//	if (root == nullptr) return;
//	std::cout << root->_key << ' ' << root->_value << std::endl;
//	DFSMF(root->_child_left);
//	DFSMF(root->_child_right);
//}
//
//void DFSLF(auto&& root)
//{
//	if (root == nullptr) return;
//	DFSLF(root->_child_left);
//	std::cout << root->_key << ' ' << root->_value << std::endl;
//	DFSLF(root->_child_right);
//}

std::vector<int> test_data_in;
std::vector<int> test_data_out;

void benchmark_init(int benchmark_size)
{
	test_data_in.reserve(benchmark_size);
	test_data_out.reserve(benchmark_size);

	std::default_random_engine e{ std::random_device{}() };
	std::uniform_int_distribution distribute(-benchmark_size, benchmark_size);

	for (int count = 0; count < benchmark_size; ++count)
	{
		test_data_in.push_back(distribute(e));
		test_data_out.push_back(distribute(e));
	}
}

#define BENCHMARK_START                                  \
    start = chrono::system_clock::now();                 \
    std::atomic_thread_fence(std::memory_order_seq_cst); \
    do {} while(false)

#define BENCHMARK_END                                               \
    std::atomic_thread_fence(std::memory_order_seq_cst);            \
    end = chrono::system_clock::now();                              \
    dur = chrono::duration_cast<chrono::microseconds>(end - start); \
    std::cout << dur << std::endl;                                  \
    total += dur;                                                   \
    do {} while(false)

template<template<class, class> class T, bool flag>
void benchmark(const char* name)
{
	namespace chrono = std::chrono;

	auto start = chrono::system_clock::now();
	std::vector<int>::iterator iter;
	auto end = chrono::system_clock::now();
	auto dur = chrono::duration_cast<chrono::microseconds>(end - start);
	decltype(dur) total{};

	std::cout << "Benchmark name: " << name << '\n';
	std::cout << "Benchmark start\n";

	std::cout << "Construct test: ";
	BENCHMARK_START;
	T<int, double> tree;
	BENCHMARK_END;
	
	std::cout << "Insert test: ";
	BENCHMARK_START;
	iter = test_data_in.begin();
	while (iter != test_data_in.end())
	{
		if constexpr (flag)
		{
			tree.insert(*iter, static_cast<double>(*iter));
		}
		else
		{
			tree.insert({ *iter, static_cast<double>(*iter) });
		}
		++iter;
	}
	BENCHMARK_END;

	std::cout << "Find test: ";
	BENCHMARK_START;
	iter = test_data_out.begin();
	while (iter != test_data_out.end())
	{
		tree.find(*iter);
		++iter;
	}
	BENCHMARK_END;

	std::cout << "Erase test: ";
	BENCHMARK_START;
	iter = test_data_out.begin();
	while (iter != test_data_out.end())
	{
		tree.erase(*iter);
		++iter;
	}
	BENCHMARK_END;

	std::cout << "Total: " << total << '\n';
	std::cout << "Benchmark end\n" << std::endl;
}

int main()
{
	benchmark_init(10000000);
	benchmark<std::unordered_map, false>("std::unordered_map");
	benchmark<std::map, false>("std::map");
	benchmark<AVL, true>("AVL");

	//AVL<int, double> tree;
	//auto iter = test_data_in.begin();
	//while (iter != test_data_in.end())
	//{
	//	tree.insert(*iter, static_cast<double>(*iter));
	//	++iter;
	//}

	//tree.yet_another_DFS_debug_check();
	
	//std::default_random_engine e{ std::random_device{}() };
	//std::uniform_int_distribution distribute(-100000, 100000);

	//AVL<int, double> tree;
	//for (int count = 0; count < 100000; ++count)
	//{
	//	tree.push(distribute(e), static_cast<double>(count));
	//	//tree.DFS_debug_check();
	//}

	//for (int count = 0; count < 100000; ++count)
	//{
	//	tree.remove(distribute(e));
	//	//tree.DFS_debug_check();
	//	//while (!remove_callstack.empty()) { remove_callstack.pop(); }
	//	//while (!DFS_callstack.empty()) { DFS_callstack.pop(); }
	//}

	//DFSLF(tree._head);
	//std::cout << std::endl;
	//DFSMF(tree._head);
	//std::cout << tree.size();

	//AVL<int, double> tree;
	//for (int count = 0; count < 7; ++count)
	//{
	//	tree.push(count, static_cast<double>(count));
	//}
	//DFSLF(tree._head);
	//std::cout << std::endl;
	//DFSMF(tree._head);
	//std::cout << std::endl;
	//tree.remove(2);
	//DFSLF(tree._head);
	//std::cout << std::endl;
	//DFSMF(tree._head);
	//std::cout << std::endl;
	//tree.remove(6);
	//DFSLF(tree._head);
	//std::cout << std::endl;
	//DFSMF(tree._head);
	//std::cout << std::endl;
	//tree.remove(1);
	//DFSLF(tree._head);
	//std::cout << std::endl;
	//DFSMF(tree._head);
	//std::cout << std::endl;

	return 0;
}