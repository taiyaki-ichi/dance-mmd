#pragma once
#include"../external/bullet3/src/btBulletCollisionCommon.h"
#include"../external/bullet3/src/btBulletDynamicsCommon.h"
#include<DirectXMath.h>
#include<tuple>
#include<memory>
#include"struct.hpp"

using namespace DirectX;

template<typename Shape>
decltype(auto) create_bullet_rigidbody(Shape& shape, XMFLOAT3 position, XMFLOAT3 rotation,
	float mass, float liner_damping, float angular_damping, float restitution, float friction)
{
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(position.x, position.y, position.z));
	btQuaternion q{};
	q.setEuler(rotation.y, rotation.x, rotation.z);
	transform.setRotation(q);

	auto motion_state = std::make_unique<btDefaultMotionState>(transform);

	btRigidBody::btRigidBodyConstructionInfo info(btScalar(mass), motion_state.get(), &shape);
	info.m_linearDamping = liner_damping;
	info.m_angularDamping = angular_damping;
	info.m_restitution = restitution;
	info.m_friction = friction;

	auto body = std::make_unique<btRigidBody>(info);

	return std::make_tuple(std::move(motion_state), std::move(body));
}

struct bullet_rigidbody
{
	std::unique_ptr<btCollisionShape> shape{};
	std::unique_ptr<btMotionState> motion_state{};
	std::unique_ptr<btRigidBody> rigidbody{};
};

inline bullet_rigidbody create_shape_bullet_rigidbody(rigidbody r)
{
	auto s = [&r]() ->std::unique_ptr<btCollisionShape>{
		if (r.shape == 0)
			return std::make_unique<btSphereShape>(btScalar(r.size.x));
		else if (r.shape == 1)
			return std::make_unique<btBoxShape>(btVector3(btScalar(r.size.x), btScalar(r.size.y), btScalar(r.size.z)));
		else
			return std::make_unique<btCapsuleShape>(btScalar(r.size.x), btScalar(r.size.y));
	}();


	auto [motion_state, body] = create_bullet_rigidbody(*s, r.position, r.rotation, r.mass, r.liner_damping, r.angular_damping, r.restitution, r.friction);
	return { std::move(s),std::move(motion_state),std::move(body) };
}
