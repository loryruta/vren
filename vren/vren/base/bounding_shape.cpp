#include "base.hpp"

bool vren::bounding_sphere::intersects(const bounding_sphere& other) const
{
	return glm::length(m_center - other.m_center) <= m_radius + other.m_radius;
}

bool vren::bounding_sphere::is_degenerate() const
{
	return m_radius == 0;
}

bool vren::bounding_sphere::contains(glm::vec3 const& point) const
{
	return glm::length(point - m_center) <= m_radius;
}

bool vren::bounding_sphere::contains(bounding_sphere const& other) const
{
	return glm::length(m_center - other.m_center) + other.m_radius <= m_radius;
}

void vren::bounding_sphere::operator+=(glm::vec3 const& point)
{
	m_radius = glm::max(glm::length(m_center - point), m_radius);
}

void vren::bounding_sphere::operator+=(bounding_sphere const& other)
{
	if (is_degenerate() || other.contains(*this))
	{
		m_center = other.m_center;
		m_radius = other.m_radius;
	}
	else if (contains(other))
	{
		return;
	}
	else
	{
		float r = m_radius + other.m_radius + glm::length(m_center - other.m_center) / 2.0f;
		m_center = m_center + (other.m_center - m_center) * (r - m_radius) / glm::length(m_center - other.m_center);
		m_radius = r;
	}
}
