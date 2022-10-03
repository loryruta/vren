#ifndef VREN_CLUSTERED_SHADING_H_
#define VREN_CLUSTERED_SHADING_H_

/**
 * Discretize and pack cluster's normal in 6 bits. Return UINT32_MAX if the normal is null
 */
uint clustered_shading_discretize_normal(vec3 normal)
{
	if (normal == vec3(0))
	{
		return UINT32_MAX;
	}

	// Search for the nearest intersecting cube's face
	float min_t = INF;
	uint axis;
	uint face_idx;
	for (uint i = 0; i < 3; i++)
	{
		float t = sign(normal[i]) / normal[i];
		if (t < min_t)
		{
			min_t = t;
			axis = i;
			face_idx = (normal[i] > 0 ? 1 : 0) * 3 + i;
		}
	}

	// Find normal's intersection point and get UV coordinates on the face
	vec3 p = normal * min_t;
	vec2 uv;
	uv.x = p[(axis + 1) % 3];
	uv.y = p[(axis + 2) % 3];

	// Encode normal by packing the face index and face's grid index in 6 bits
	//uvec2 disc_uv = uvec2(round(uv + 1)); TODO
	uvec2 disc_uv = uvec2(floor((uv + 1) / 2.0 * 3.0));
	uint disc_normal = face_idx * (3 * 3) + disc_uv.x * 3 + disc_uv.y;

	disc_normal &= 0x3F;

	return disc_normal;
}

uvec3 clustered_shading_decode_normal_idx(uint normal_idx)
{
	uint face_idx = normal_idx / (3 * 3);
	
	uvec2 uv;
	uv.x = (normal_idx / 3) % 3;
	uv.y = normal_idx % 3;

	return uvec3(face_idx, uv.x, uv.y);
}

// void clustered_shading_encode_cluster_key(...)

void clustered_shading_decode_cluster_key(
	uint cluster_key,
	out uvec3 cluster_ijk,
	out uint cluster_normal_idx
)
{
	cluster_ijk = uvec3(
		cluster_key & 0xFF,
		(cluster_key >> 8) & 0xFF,
		(cluster_key >> 16) & 0x3FF
	);
	cluster_normal_idx = cluster_key >> 26;
}

void clustered_shading_calc_cluster_aabb(
	uvec3 cluster_ijk,
	uvec2 num_tiles,
	float camera_near,
	float camera_half_fov,
	mat4 camera_proj,
	out vec4 cluster_min,
	out vec4 cluster_max
)
{
	// Clip space tile min and tile max
	cluster_min = vec4(cluster_ijk.xy / vec2(num_tiles), 0.0, 1.0);
	cluster_min.xy = vec2(cluster_min.x, 1.0 - cluster_min.y) * 2.0 - 1.0;

	cluster_max = vec4((cluster_ijk.xy + uvec2(1.0)) / vec2(num_tiles), 0.0, 1.0);
	cluster_max.xy = vec2(cluster_max.x, 1.0 - cluster_max.y) * 2.0 - 1.0;

	// Convert to view space
	mat4 inverse_proj = inverse(camera_proj);

	cluster_min = inverse_proj * cluster_min;
	cluster_min /= cluster_min.w;

	cluster_max = inverse_proj * cluster_max;
	cluster_max /= cluster_max.w;

	// Apply cluster's depth in view space
	float a = 2 * tan(camera_half_fov) / num_tiles.y + 1.0;
	float cluster_near = camera_near * pow(a, cluster_ijk.z);
	float cluster_far = cluster_near * a;

	vec3 d1 = normalize(cluster_min.xyz);
	vec3 d2 = normalize(cluster_max.xyz);

	cluster_min = vec4(cluster_near / d1.z * d1, 1);
	cluster_max = vec4(cluster_far / d2.z * d2, 1);

	// TODO cluster_min = min( ... ), cluster_max = max( ... )
}

float clustered_shading_get_light_intensity(
	vec3 point_light_pos,
	vec3 cluster_min,
	vec3 cluster_max,
	uint cluster_normal_idx
)
{
	vec3 v;

	uvec3 decoded_normal_idx = clustered_shading_decode_normal_idx(cluster_normal_idx);

	// Build cluster cone
	vec3 cluster_pos = (cluster_max + cluster_min) / 2.0;

	uint face_axis = decoded_normal_idx.x % 3;
	uint u_component = (face_axis + 1) % 3;
	uint v_component = (face_axis + 2) % 3;

	uvec2 uv = decoded_normal_idx.yz;

	v[face_axis] = (decoded_normal_idx.x / 3) == 1 ? 1 : -1;
	v[u_component] = ((uv.x + 0.5) / 3.0) * 2.0 - 1.0;
	v[v_component] = ((uv.y + 0.5) / 3.0) * 2.0 - 1.0;

	vec3 cluster_normal = normalize(v);

	float alpha = -INF;

	for (uint i = 0; i < 4; i++)
	{
		v[u_component] = ((uv.x + (i & 1)) / 3.0) * 2.0 - 1.0;
		v[v_component] = ((uv.y + ((i >> 1) & 1)) / 3.0) * 2.0 - 1.0;

		alpha = max(alpha, acos(dot(v, cluster_normal) / length(v)));
	}
	
	// Build point light cone
	vec3 point_light_dir = normalize(cluster_pos - point_light_pos);

	float delta = -INF;
	for (uint i = 0; i < 8; i++)
	{
		v = vec3(
			(i & 1) == 0 ? cluster_min.x : cluster_max.x,
			((i >> 1) & 1) == 0 ? cluster_min.y : cluster_max.y,
			((i >> 2) & 1) == 0 ? cluster_min.z : cluster_max.z 
		);

		v = normalize(v - point_light_pos);

		delta = max(delta, acos(dot(v, point_light_dir)));
	}

	// Check angle
	float omega = acos(dot(cluster_normal, -point_light_dir)); // [0, pi]
	float thresold = (PI / 2.0) + alpha + delta;
	return 1.0;
}

#endif // VREN_CLUSTERED_SHADING_H_
