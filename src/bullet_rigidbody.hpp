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

	btVector3 localInertia(0, 0, 0);
	shape.calculateLocalInertia(mass, localInertia);

	btRigidBody::btRigidBodyConstructionInfo info(btScalar(mass), motion_state.get(), &shape, localInertia);
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
	// massが0だとまずい？
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
	// 参考：https://github.com/benikabocha/saba/blob/e64f8c9ada05a47bf89cb7fd79f05d16210a584b/src/Saba/Model/MMD/MMDPhysics.cpp#L736

	auto rigidbodyA = rs[j.rigidbody_a].rigidbody.get();
	auto rigidbodyB = rs[j.rigidbody_b].rigidbody.get();

	btMatrix3x3 rotMat;
	rotMat.setEulerZYX(j.rotation.y, j.rotation.x, -j.rotation.z);

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(
		j.position.x,
		j.position.y,
		j.position.z
	));
	transform.setBasis(rotMat);

	btTransform invA = rigidbodyA->getWorldTransform().inverse();
	btTransform invB = rigidbodyB->getWorldTransform().inverse();
	invA = invA * transform;
	invB = invB * transform;

	// 第三引数はrigitbodyA側につくジョイントをrigidbodyAをもとに計算する用かな
	// 第四引数も同様っぽい
	// 今回はj.posiitonとj.rotationをもとにピッタリ1点をそれぞれが示す形だが、ある程度幅を持たすことも可能かも？
	// ちゃんと読めば参考になりそう：https://gamedev.stackexchange.com/questions/54349/what-are-frame-a-and-frame-b-in-btgeneric6dofconstraints-constructor-for
	auto pGen6DOFSpring = std::make_unique<btGeneric6DofSpringConstraint>(*rigidbodyA, *rigidbodyB, invA, invB, true);

	pGen6DOFSpring->setLinearLowerLimit(btVector3(j.move_lower_limit.x, j.move_lower_limit.y, j.move_lower_limit.z));
	pGen6DOFSpring->setLinearUpperLimit(btVector3(j.move_upper_limit.x, j.move_upper_limit.y, j.move_upper_limit.z));

	pGen6DOFSpring->setAngularLowerLimit(btVector3(j.rotation_lower_limit.y, j.rotation_lower_limit.x, j.rotation_lower_limit.z));
	pGen6DOFSpring->setAngularUpperLimit(btVector3(j.rotation_upper_limit.y, j.rotation_upper_limit.x, j.rotation_upper_limit.z));

	pGen6DOFSpring->setAngularLowerLimit(btVector3(0.f, 0.f, 0.f));
	pGen6DOFSpring->setAngularUpperLimit(btVector3(0.1f, 0.1f, 0.1f));

	pGen6DOFSpring->setDbgDrawSize(btScalar(5.f));

	if (j.move_spring_constant.x != 0.f)
	{
		pGen6DOFSpring->enableSpring(0, true);
		pGen6DOFSpring->setStiffness(0, j.move_spring_constant.x);
	}
	if (j.move_spring_constant.y != 0.f)
	{
		pGen6DOFSpring->enableSpring(1, true);
		pGen6DOFSpring->setStiffness(1, j.move_spring_constant.y);
	}
	if (j.move_spring_constant.z != 0.f)
	{
		pGen6DOFSpring->enableSpring(2, true);
		pGen6DOFSpring->setStiffness(2, j.move_spring_constant.z);
	}
	
	// 3-5が回転
	if (j.rotation_spring_constant.y != 0.f)
	{
		pGen6DOFSpring->enableSpring(3, true);
		pGen6DOFSpring->setStiffness(3, j.rotation_spring_constant.y);
	}
	if (j.rotation_spring_constant.x != 0.f)
	{
		pGen6DOFSpring->enableSpring(4, true);
		pGen6DOFSpring->setStiffness(4, j.rotation_spring_constant.x);
	}
	if (j.rotation_spring_constant.z != 0.f)
	{
		pGen6DOFSpring->enableSpring(5, true);
		pGen6DOFSpring->setStiffness(5, j.rotation_spring_constant.z);
	}

	return { std::move(pGen6DOFSpring) };
}