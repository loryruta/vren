#ifndef VREN_DEBUG_H_
#define VREN_DEBUG_H_

#include <common.glsl>

#define VREN_DEBUG_LINE_VERTICES_COUNT 2
#define VREN_DEBUG_ARROW_VERTICES_COUNT (VREN_DEBUG_LINE_VERTICES_COUNT + VREN_DEBUG_LINE_VERTICES_COUNT * 3)
#define VREN_DEBUG_AABB_VERTICES_COUNT (12 * VREN_DEBUG_LINE_VERTICES_COUNT)

#define VREN_DEBUG_WRITE_LINE(_buffer, _offset, _from, _to, _color) \
    _buffer[_offset].position = _from.xyz; \
    _buffer[_offset].color = _color; \
    \
    _buffer[_offset + 1].position = _to.xyz; \
    _buffer[_offset + 1].color = _color; \
    \
    _offset += 2;


#define VREN_DEBUG_WRITE_ARROW(_buffer, _offset, _from, _to, _color, _head_color) \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, _from, _to, _color); \
    \
    float _mvar_head_length = distance(_from, _to) * 0.1; \
    \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, (_to + vec3(-1,-1,-1) * _mvar_head_length), (_to + vec3(1,1,1) * _mvar_head_length), _head_color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, (_to + vec3(1,1,-1) * _mvar_head_length), (_to + vec3(-1,-1,1) * _mvar_head_length), _head_color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, (_to + vec3(1,-1,-1) * _mvar_head_length), (_to + vec3(-1,1,1) * _mvar_head_length), _head_color);


#define VREN_DEBUG_WRITE_AABB(_buffer, _offset, _m, _M, _color) \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, _m, vec3(_M.x, _m.y, _m.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_M.x, _m.y, _m.z), vec3(_M.x, _m.y, _M.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_M.x, _m.y, _M.z), vec3(_m.x, _m.y, _M.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_m.x, _m.y, _M.z), _m, _color); \
    \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_m.x, _M.y, _m.z), vec3(_M.x, _M.y, _m.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_M.x, _M.y, _m.z), vec3(_M.x, _M.y, _M.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_M.x, _M.y, _M.z), vec3(_m.x, _M.y, _M.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_m.x, _M.y, _M.z), vec3(_m.x, _M.y, _m.z), _color); \
    \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_m.x, _m.y, _m.z), vec3(_m.x, _M.y, _m.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_M.x, _m.y, _m.z), vec3(_M.x, _M.y, _m.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_M.x, _m.y, _M.z), vec3(_M.x, _M.y, _M.z), _color); \
    VREN_DEBUG_WRITE_LINE(_buffer, _offset, vec3(_m.x, _m.y, _M.z), vec3(_m.x, _M.y, _M.z), _color);

#endif// VREN_DEBUG_H_
