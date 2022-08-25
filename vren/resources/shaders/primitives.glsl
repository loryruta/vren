#ifndef PRIMITIVES_H

#define VREN_WORKGROUP_REDUCE(_buffer, _length, _offset, _stride) \
	for (uint level = 0; level < uint(log2(_length)); level++) \
	{ \
		uint level_mask = (1 << (level + 1)) - 1; \
		if ((gl_LocalInvocationID.x & level_mask) == level_mask) \
		{ \
			uint b_idx = (gl_LocalInvocationID.x + 1) * _stride - 1 + _offset; \
			uint a_idx = b_idx - (1 << level) * _stride; \
			\
			_buffer[b_idx] = _buffer[b_idx] + _buffer[a_idx]; \
		} \
		\
		barrier(); \
	}

#define VREN_WORKGROUP_DOWNSWEEP(_buffer, _length, _offset, _stride) \
	for (int level = int(log2(_length)) - 1; level >= 0; level--) \
	{ \
		uint level_mask = (1 << (level + 1)) - 1;  \
		if ((gl_LocalInvocationID.x & level_mask) == level_mask) \
		{ \
			uint b_idx = (gl_LocalInvocationID.x + 1) * _stride - 1 + _offset; \
			uint a_idx = b_idx - (1 << level) * _stride; \
			\
			uint tmp = _buffer[b_idx]; \
            _buffer[b_idx] = _buffer[a_idx] + tmp; \
			_buffer[a_idx] = tmp; \
		} \
		\
		barrier(); \
	}

#define VREN_WORKGROUP_EXCLUSIVE_SCAN(_buffer, _length, _offset, _stride) \
	VREN_WORKGROUP_REDUCE(_buffer, _length, _offset, _stride); \
	\
	_buffer[_length - 1] = 0; \
	\
	barrier(); \
	\
	VREN_WORKGROUP_DOWNSWEEP(_buffer, _length, _offset, _stride);

/*
	VREN_WORKGROUP_INCLUSIVE_SCAN (TODO TEST)

	- _inout_array : The array on which the inclusive scan has to be done
	- _in_length : The length of the array
	- _in_offset : Initial offset of the relevant data
	- _in_stride : Stride of the relevant data
*/
#define VREN_WORKGROUP_INCLUSIVE_SCAN(_inout_array, _in_length, _in_offset, _in_stride) \
	for (uint _i = 0; _i < uint(ceil(log2(float(_in_length)))); _i++) \
	{ \
		uint _step = uint(exp2(_i)); \
		\
		uint _a = _in_offset + gl_LocalInvocationIndex * _in_stride; \
		uint _b = _in_offset + gl_LocalInvocationIndex * _in_stride + _step * _in_stride; \
		\
		if (_b < _in_length) \
		{ \
			_inout_array[_b] = _inout_array[_a] + _inout_array[_b]; \
		} \
		\
		barrier(); \
	}

/*
	VREN_WORKGROUP_RADIX_SORT (TODO TEST)
	2-bit radix sort

	- _inout_array : The array to sort (cardinality is the size of the workgroup)
	- _in_length : The length of the input array
	- _inout_scratch_buffer_1 : Used to store temporary results of sorting. Needs to be at least _in_length
	- _inout_scratch_buffer_2 : Used to store occurencies of a certain digit and to perform exclusive scan. Needs to be at least max(_in_length, 16)
	- _inout_scratch_buffer_3 : Used to hold exclusive scan results. Needs to be at least _in_length
*/
#define VREN_WORKGROUP_RADIX_SORT(_inout_array, _in_length, _inout_scratch_buffer_1, _inout_scratch_buffer_2, _inout_scratch_buffer_3) \
	\
	uint _bit_count = 2; \
	\
	for (uint _digit_position = 0; _digit_position < uint(ceil(32 / float(_bit_count))); _digit_position++) \
	{ \
		uint _digit_mask = uint(exp2(_bit_count) - 1) << (_digit_position * _bit_count); \
		uint _current_digit = (_digit_position % 2 == 0 ? _inout_array[gl_LocalInvocationIndex] : _inout_scratch_buffer_1[gl_LocalInvocationIndex]) & _digit_mask; \
		\
		for (uint _digit = 0; _digit < exp2(_bit_count); _digit++) \
		{ \
			if (gl_LocalInvocationIndex < _in_length) \
			{ \
				_inout_scratch_buffer_2[gl_LocalInvocationIndex] = _current_digit == _digit ? 1 : 0; \
			} \
			\
			barrier(); \
			\
			VREN_WORKGROUP_EXCLUSIVE_SCAN(_inout_scratch_buffer_2, _in_length, gl_LocalInvocationIndex, 1); \
			\
			barrier(); \
			\
			if (_current_digit == _digit) \
			{ \
				_inout_scratch_buffer_3[gl_LocalInvocationIndex] = _inout_scratch_buffer_2[gl_LocalInvocationIndex]; \
			} \
			\
			barrier(); \
		} \
		\
		if (gl_LocalInvocationIndex < 16) \
		{ \
			_inout_scratch_buffer_2[gl_LocalInvocationIndex] = 0; \
		} \
		\
		barrier(); \
		\
		atomicAdd(_inout_scratch_buffer_2[_current_digit], 1); \
		\
		barrier(); \
		\
		uint _local_offset = _inout_scratch_buffer_3[gl_LocalInvocationIndex]; \
		uint _global_offset = _inout_scratch_buffer_2[_current_digit]; \
		\
		if (_digit_position % 2 == 0) \
		{ \
			_inout_scratch_buffer_1[gl_LocalInvocationIndex] = _inout_array[gl_LocalInvocationIndex]; \
		} \
		else \
		{ \
			_inout_array[_global_offset + _local_offset] = _inout_scratch_buffer_1[gl_LocalInvocationIndex]; \
		} \
		\
		barrier(); \
	}

#endif
