#ifndef BITS_VECTOR_H_VO7JQ9YK
#define BITS_VECTOR_H_VO7JQ9YK
	
class bits_vector
{
public:
	typedef unsigned long long bits_t;
	static size_t const bits_sz;
private:
	std::vector<bits_t> _backing;
	int path_len;
	size_t sizes_index;
	size_t bit_index (int size) const { return size % bits_sz; }
	size_t container_index (int size) const { return size / bits_sz; }
public:
	bits_vector (int path_len) : _backing(5, 0L), path_len(path_len), sizes_index(0) { }
	size_t size () const { return sizes_index; }
	
	void add (bits_t bits);
	void reserve (size_t size);
	bits_t at (size_t index) const;
};
#endif /* end of include guard: BITS_VECTOR_H_VO7JQ9YK */
