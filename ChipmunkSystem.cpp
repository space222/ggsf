#include "pch.h"
#include "ChipmunkSystem.h"
#include <chipmunk\chipmunk.h>

void update_world(cpBody* body, GGScene* scene)
{   // passed as the iterator function to cpSpaceEachBody
	auto ud = cpBodyGetUserData(body);
	entt::entity E = (entt::entity)(u64)ud;
	cpVect p = cpBodyGetPosition(body);
	float angle =(float) cpBodyGetAngle(body);
	Transform2D t((float)p.x, (float)p.y, angle);
	scene->entities.assign_or_replace<Transform2D>(E, t);
	return;
}

void ChipmunkSystem::update(GGScene* scene)
{
	cpSpaceEachBody(world,(cpSpaceBodyIteratorFunc) update_world, (void*)scene);
	return;
}

ChipmunkSystem::ChipmunkSystem()
{
	world = cpSpaceNew();
	cpSpaceSetGravity(world, cpVect{ 0, -100 });
	return;
}

void ChipmunkSystem::register_components(GGScene* scene)
{
	scene->components.insert(std::make_pair("physics2d", new Physics2DComponent(this)));
	return;
}

bool Physics2DComponent::create_instance(GGScene* scene, entt::entity E, nlohmann::json& J)
{
	int type = 0; //default dynamic
	cpBody* body = nullptr;
	float moment = 0.0f;
	float total_mass = 0.0f;
	int total_shapes = 0;
	std::vector<float> segments;
	std::vector<float> circles;

	if (J.contains("type") && J["type"].is_string())
	{
		std::string t = J["type"].get<std::string>();
		if (t == "static")
		{
			type = 1;
		} else if (t == "kinematic") {
			type = 2;
		} else if (t == "dynamic") {
			type = 0;
		}
		//todo: log error for something else (other than "dynamic"
	}

	if (J.contains("segments") && J["segments"].is_array())
	{
		segments = J["segments"].get<std::vector<float>>();
		// looks like moment of inertia for a segment is zero
		// so might not actually need this.
		total_shapes = (int)segments.size() / 6;
		for (int i = 0; i < segments.size() / 6; ++i)
		{
			cpVect a{ segments[i * 6 + 2], segments[i * 6 + 3] };
			cpVect b{ segments[i * 6 + 4], segments[i * 6 + 5] };
			moment +=(float) cpMomentForSegment(total_mass += segments[i * 6], a, b, 0);
		}
	}

	if (J.contains("circles") && J["circles"].is_array())
	{
		circles = J["circles"].get<std::vector<float>>();
		total_shapes = (int)circles.size() / 6;
		for (int i = 0; i < circles.size() / 6; ++i)
		{
			cpVect p{ segments[i * 6 + 3], segments[i * 6 + 4] };
			moment +=(float) cpMomentForCircle(total_mass += segments[i * 6], 0, segments[i * 6 + 2], p);
		}
	}

	if (type > 0)
	{
		body = cpSpaceGetStaticBody(chip->space());
	} else {
		float mass = total_mass;
		if (J.contains("mass"))
		{
			mass = J["mass"].get<float>();
		}

		body = cpBodyNew(mass, moment);
	}

	for (int i = 0; i < segments.size() / 6; ++i)
	{
		cpVect a{ segments[i * 6 + 2], segments[i * 6 + 3] };
		cpVect b{ segments[i * 6 + 4], segments[i * 6 + 5] };

		cpShape* s = cpSegmentShapeNew(body, a, b, 0);
		cpShapeSetFriction(s, segments[i * 6 + 1]);
	}

	for (int i = 0; i < circles.size() / 6; ++i)
	{
		cpVect p{ segments[i * 6 + 3], segments[i * 6 + 4] };
		cpShape* s = cpCircleShapeNew(body, segments[i * 6 + 2], p);
		cpShapeSetFriction(s, segments[i * 6 + 1]);
	}

	Transform2D t;
	if (scene->entities.has<Transform2D>(E)) t = scene->entities.get<Transform2D>(E);

	if (type == 0)
	{
		cpSpaceAddBody(chip->space(), body);
		cpBodySetUserData(body, (cpDataPointer)E);
	}

	cpBodySetPosition(body, cpVect{ t.x, t.y });
	cpBodySetAngle(body, t.angle);
	
	return true;
}

SystemInterface* ChipmunkSystem_factory()
{
	return new ChipmunkSystem;
}

REGISTER_SYSTEM(Chipmunk)
