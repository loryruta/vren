struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 texcoord;
};

struct Meshlet
{
	uint vertex_offset;
	uint vertex_count;
	uint triangle_offset;
	uint triangle_count;
};
