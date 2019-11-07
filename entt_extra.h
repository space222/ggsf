#pragma once
#include <entt/entt.hpp>

template <typename CT, typename VT>
const VT& get_member_or(entt::registry& reg, entt::entity E, VT CT::* mptr, const VT& val)
{
	if (auto P = reg.try_get<CT>(E); P)
	{
		return P->*mptr;
	}
	return val;
}



