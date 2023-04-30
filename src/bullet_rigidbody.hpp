#pragma once
#include"../external/bullet3/src/btBulletCollisionCommon.h"
#include"../external/bullet3/src/btBulletDynamicsCommon.h"
#include<DirectXMath.h>
#include<tuple>
#include<memory>
#include<vector>
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

	// ボーン追従の場合は質量を0にし重力の計算を行わないようにする
	auto m = r.rigidbody_type == 0 ? 0.f : r.mass;

	auto [motion_state, body] = create_bullet_rigidbody(*s, r.position, r.rotation, m, r.liner_damping, r.angular_damping, r.restitution, r.friction);
	return { std::move(s),std::move(motion_state),std::move(body) };
}


struct bullet_joint
{
	// とりあえずばね付き6dof
	std::unique_ptr<btGeneric6DofSpringConstraint> spring;
};

// とりあえずばね付き6DOF
inline bullet_joint create_bullet_joint(joint const& j,std::vector<bullet_rigidbody> const& rs)
{
	auto rigidbodyA = rs[j.rigidbody_a].rigidbody.get();
	auto rigidbodyB = rs[j.rigidbody_b].rigidbody.get();

	// 相対位置を計算
	btTransform trans{};
	rigidbodyA->getMotionState()->getWorldTransform(trans);
	auto a_pos = btVector3(j.position.x, j.position.y, j.position.z) - trans.getOrigin();
	rigidbodyB->getMotionState()->getWorldTransform(trans);
	auto b_pos = btVector3(j.position.x, j.position.y, j.position.z) - trans.getOrigin();

	btTransform frameInA, frameInB;
	frameInA = btTransform::getIdentity();
	frameInA.setOrigin(a_pos);
	btQuaternion q{};
	q.setEuler(j.rotation.y, j.rotation.x, j.rotation.z);
	frameInA.setRotation(q);

	frameInB = btTransform::getIdentity();
	frameInB.setOrigin(b_pos);
	frameInB.setRotation(q);

	auto pGen6DOFSpring = std::make_unique<btGeneric6DofSpringConstraint>(*rigidbodyA, *rigidbodyB, frameInA, frameInB, true);

	
	pGen6DOFSpring->setLinearLowerLimit(btVector3(j.move_lower_limit.x, j.move_lower_limit.y, j.move_lower_limit.z));
	pGen6DOFSpring->setLinearUpperLimit(btVector3(j.move_upper_limit.x, j.move_upper_limit.y, j.move_upper_limit.z));
	
	pGen6DOFSpring->setAngularLowerLimit(btVector3(j.rotation_lower_limit.y, j.rotation_lower_limit.x, j.rotation_lower_limit.z));
	pGen6DOFSpring->setAngularUpperLimit(btVector3(j.rotation_upper_limit.y, j.rotation_upper_limit.x, j.rotation_upper_limit.z));

	pGen6DOFSpring->setDbgDrawSize(btScalar(5.f));

	pGen6DOFSpring->enableSpring(0, true);

	// とりあえず、Stiffnessをばね回転、Dampimgをばね移動で設定してみる
	pGen6DOFSpring->setDamping(0, j.move_spring_constant.x);
	pGen6DOFSpring->setDamping(1, j.move_spring_constant.y);
	pGen6DOFSpring->setDamping(2, j.move_spring_constant.z);
	pGen6DOFSpring->setStiffness(0, j.rotation_spring_constant.x);
	pGen6DOFSpring->setStiffness(1, j.rotation_spring_constant.y);
	pGen6DOFSpring->setStiffness(2, j.rotation_spring_constant.z);

	pGen6DOFSpring->setEquilibriumPoint();

	return { std::move(pGen6DOFSpring) };
}