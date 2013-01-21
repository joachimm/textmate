#include "bits_vector.h"
size_t const bits_vector::bits_sz = sizeof(bits_vector::bits_t)*CHAR_BIT;

void bits_vector::add (bits_vector::bits_t bits)
{
	size_t cursor = sizes_index * path_len;
	size_t ci = container_index(path_len + cursor);
	size_t cio = container_index(cursor);
	size_t bit_idx = bit_index(cursor);
	
	if (ci != cio)
	{
		_backing[cio] |= bits >> (path_len - bits_vector::bits_sz + bit_idx);
		_backing[ci] |= bits << bits_vector::bits_sz - bit_index(path_len + cursor);
	}
	else
	{
		_backing[ci] |= bits << (bits_vector::bits_sz - path_len - bit_idx);
	}
	cursor+=path_len;
	sizes_index++;
}

bits_vector::bits_t bits_vector::at(size_t index) const
{
	size_t cursor = index * path_len;
	size_t ci = container_index(path_len + cursor);
	size_t cio = container_index(cursor);
	size_t bit_idx = bit_index(cursor);

	bits_vector::bits_t bits;
	if (ci != cio)
	{
		bits = _backing[cio] << bit_idx;
		bits = bits >> (bits_vector::bits_sz - path_len);
		bits |= _backing[ci] >> bits_vector::bits_sz - bit_index(path_len + cursor);
	}
	else
	{
		bits = _backing[ci] << bit_idx;
		bits = bits >> (bits_vector::bits_sz - path_len);
	}
	return bits;
}

void bits_vector::reserve (size_t size)
{
	sizes_index = 0;
	size_t p = container_index(size);
	if(p > _backing.size())
		_backing.resize(p);
	std::fill_n(_backing.begin(), p, 0L); // necessary because we OR in bits
}
