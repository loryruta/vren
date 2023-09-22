#ifndef PRIMITIVES_H

#define VREN_WORKGROUP_REDUCE(_buffer, _length, _offset, _stride) \
 for (uint level = 0; level < uint(log2(_length)); level++) \
    { \
        uint level_mask = (1 << (level + 1)) - 1; \
        if ((gl_LocalInvocationIndex & level_mask) == level_mask) \
        { \
            uint b_idx = (gl_LocalInvocationIndex + 1) * _stride - 1 + _offset; \
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
        if ((gl_LocalInvocationIndex & level_mask) == level_mask) \
        { \
            uint b_idx = (gl_LocalInvocationIndex + 1) * _stride - 1 + _offset; \
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

// https://en.wikipedia.org/wiki/Bitonic_sorter
#define VREN_WORKGROUP_BITONIC_SORT(_inout_array, _in_length) \
 for (uint _k = 2; _k <= _in_length; _k *= 2) \
    { \
 for (uint _j = _k / 2; _j > 0; _j /= 2) \
        { \
            uint _i = gl_LocalInvocationIndex; \
            uint _l = _i ^ _j; \
            if (_l > _i) \
            { \
                if ((((_i & _k) == 0) && _inout_array[_i].x > _inout_array[_l].x) || (((_i & _k) != 0) && _inout_array[_i].x < _inout_array[_l].x)) \
                { \
                    uvec2 _tmp = _inout_array[_i]; \
                    _inout_array[_i] = _inout_array[_l]; \
                    _inout_array[_l] = _tmp; \
 } \
 } \
            \
            barrier(); \
 } \
 }

#endif// PRIMITIVES_H
