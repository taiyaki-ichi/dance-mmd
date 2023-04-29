#pragma once
#include"../external/bullet3/src/btBulletCollisionCommon.h"
#include"../external/bullet3/src/btBulletDynamicsCommon.h"

struct bullet_world
{
	btDefaultCollisionConfiguration collision_configuration;
	btCollisionDispatcher dispatcher;
	btDbvtBroadphase broadphase;
	btSequentialImpulseConstraintSolver solver;
	btDiscreteDynamicsWorld dynamics_world;

	bullet_world()
		:collision_configuration()
		, dispatcher(&collision_configuration)
		, broadphase()
		, solver()
		, dynamics_world(&dispatcher, &broadphase, &solver, &collision_configuration)
	{
		dynamics_world.setGravity(btVector3(0, -9.8, 0));
	}

};