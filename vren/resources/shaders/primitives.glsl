#ifndef PRIMITIVES_H

#define _VREN_WORKGROUP_REDUCE(_buffer, _length, _offset, _stride) \
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

#define _VREN_WORKGROUP_DOWNSWEEP(_buffer, _length, _offset, _stride) \
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

#define _VREN_WORKGROUP_EXCLUSIVE_SCAN(_buffer, _length, _offset, _stride) \
	_VREN_WORKGROUP_REDUCE(_buffer, _length, _offset, _stride); \
	\
	_buffer[_length - 1] = 0; \
	barrier(); \
	\
	_VREN_WORKGROUP_DOWNSWEEP(_buffer, _length, _offset, _stride);

#endif
