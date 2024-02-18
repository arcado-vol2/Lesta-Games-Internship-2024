
#include <cassert>
#include <cmath>
#include <array>

#include "../framework/scene.hpp"
#include "../framework/game.hpp"
#include "../framework/engine.hpp"

//delete
#include <iostream>
#include <vector>
#include <algorithm>
#include <typeinfo>
//-------------------------------------------------------
//	Basic Vector2 class
//-------------------------------------------------------

class Vector2
{
public:
	float x = 0.f;
	float y = 0.f;

	constexpr Vector2() = default;
	constexpr Vector2( float vx, float vy );
	constexpr Vector2( Vector2 const &other ) = default;

	float DistanceTo(const Vector2& other) {
		return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2));
	}
	
	//get normal vector 
	Vector2 DirectionTo(const Vector2& other) {
		float distance = DistanceTo(other);
		Vector2 dirVec;
		dirVec.x = (other.x - x) / distance;
		dirVec.y = (other.y - y) / distance;
		return dirVec;
	}

	//get magnitude of vector
	float Length() {
		return std::sqrt(std::pow(x, 2) + std::pow(y, 2));
	}

	//get ||vec||^2 
	float LengthPow2() {
		return std::pow(x, 2) + std::pow(y, 2);
	}

	/* 
	Dot product
	*/
	float Dot(const Vector2& other) {
		return x * other.x + y * other.y;
	}

	//vector normalization with a litle round (cos very small values can be treaky)
	Vector2 Normalized() {
		float mag = Length();
		if (mag != 0) {
			return Vector2(round(x / mag * 1000) /1000, round(y / mag * 1000) / 1000);
		}
		return Vector2(0, 0);
	}

	//different operators

	Vector2 operator+(const Vector2& other) const {
		return Vector2(x + other.x, y + other.y);
	}

	Vector2 operator-(const Vector2& other) const {
		return Vector2(x - other.x, y - other.y);
	}
	
	Vector2 operator*(const float scalar) const {
		return Vector2(x * scalar, y * scalar);
	}

	Vector2 operator/(const float scalar) const {
		if (scalar != 0)
			return Vector2(x / scalar, y / scalar);
		return Vector2(x, y);
	}
	
	Vector2 operator*(const Vector2& other) const {
		return Vector2(x * other.x, y * other.y);
	}

	Vector2& operator+=(const Vector2& other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	Vector2& operator-=(const Vector2& other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	Vector2& operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		return *this;
	}
};

Vector2 operator*(float scalar, const Vector2& vec) {
	return vec * scalar;
}

constexpr Vector2::Vector2( float vx, float vy ) :
	x( vx ),
	y( vy )
{
}

//lineal interpolation
float Lerp(float x, float y, float t) {
	return t * x + (1 - t) * y;
}

Vector2 Lerp(const Vector2& pos1, const Vector2& pos2, float t) {
	return Vector2(Lerp(pos1.x, pos2.x, t), Lerp(pos1.y, pos2.y, t));
}



//-------------------------------------------------------
//	game parameters
//-------------------------------------------------------

namespace Params
{
	namespace System
	{
		constexpr int targetFPS = 60;
	}

	namespace Table
	{
		constexpr float width = 15.f; //border from -7.5f to 7.5f
		constexpr float height = 8.f; //border from -4 to 4
		constexpr float pocketRadius = 0.4f;

		static constexpr std::array< Vector2, 6 > pocketsPositions =
		{
			Vector2{ -0.5f * width, -0.5f * height },
			Vector2{ 0.f, -0.5f * height },
			Vector2{ 0.5f * width, -0.5f * height },
			Vector2{ -0.5f * width, 0.5f * height },
			Vector2{ 0.f, 0.5f * height },
			Vector2{ 0.5f * width, 0.5f * height }
		};

		static constexpr std::array< Vector2, 7 > ballsPositions =
		{
			// player ball
			Vector2( -0.3f * width, 0.f ),
			// other balls
			Vector2( 0.2f * width, 0.f ),
			Vector2( 0.25f * width, 0.05f * height ),
			Vector2( 0.25f * width, -0.05f * height ),
			Vector2( 0.3f * width, 0.1f * height ),
			Vector2( 0.3f * width, 0.f ),
			Vector2( 0.3f * width, -0.1f * height )
		};
	}

	namespace Ball
	{
		constexpr float radius = 0.3f;
		//maximum ball speed
		constexpr float maxSpeed = 19.f;
		//1 - doesn't slow down, 0 - doesn't move at all
		constexpr float speedModificatory = 0.98f;
	}

	namespace Shot
	{
		constexpr float chargeTime = 1.f;
	}
}


//-------------------------------------------------------
//	Table logic
//-------------------------------------------------------

class Table
{
public:
	Table() = default;
	Table( Table const& ) = delete;

	void init();
	void deinit();
	
	Scene::Mesh* getBall(int index) const {
		return balls[index];
	}
	
	Scene::Mesh* getPocket(int index) const {
		return pockets[index];
	}
	
	//there was a dilemma here, I decided that I shouldn’t change the "balls" signature and
	// so I had to add a method so that there would be no errors when restarting the scene


	void removeBall(Scene::Mesh* ballToRemove) {
		auto it = std::find(balls.begin(), balls.end(), ballToRemove);
		if (it != balls.end()) {
			for (auto shift_it = it; shift_it != balls.end() - 1; ++shift_it) {
				*shift_it = *(shift_it + 1);
			}
			balls[balls.size() - 1] = nullptr;
		}
	}
private:
	std::array< Scene::Mesh*, 7 > balls = {};
	std::array< Scene::Mesh*, 6 > pockets = {};
};


void Table::init()
{
	for ( int i = 0; i < 6; i++ )
	{
		assert( !pockets[ i ] );
		pockets[ i ] = Scene::createPocketMesh( Params::Table::pocketRadius );
		Scene::placeMesh( pockets[ i ], Params::Table::pocketsPositions[ i ].x, Params::Table::pocketsPositions[ i ].y, 0.f );
	}

	for ( int i = 0; i < 7; i++ )
	{
		assert( !balls[ i ] );
		balls[ i ] = Scene::createBallMesh( Params::Ball::radius );
		Scene::placeMesh( balls[ i ], Params::Table::ballsPositions[ i ].x, Params::Table::ballsPositions[ i ].y, 0.f );
	}
}


void Table::deinit()
{
	for ( Scene::Mesh* mesh : pockets )
		Scene::destroyMesh( mesh );
	for (Scene::Mesh* mesh : balls) {
		if (mesh != nullptr)
			Scene::destroyMesh(mesh);
	}	
	pockets = {};
	balls = {};
}


class PhysicBody2D {
public:
	Vector2 position;
	float radius;
	Scene::Mesh* mesh;

	PhysicBody2D() : position(Vector2()), radius(0), mesh(nullptr) {}

	PhysicBody2D(Vector2 _position, float _radius, Scene::Mesh* _mesh) {
		position = _position;
		radius = _radius;
		mesh = _mesh;

	}

	bool CheckCollision(PhysicBody2D* other) {

		if (position.DistanceTo(other->position) <= radius + other->radius)
			return true;
		return false;
	}
	
	void Deinit() {
		Scene::destroyMesh(mesh);
	}

	
};


//basic 2d rigid body class targeted to circle
class RigidBody2D : public PhysicBody2D{
public:
	Vector2 velocity;
	
	RigidBody2D() : PhysicBody2D(), velocity(Vector2(0,0)){}

	RigidBody2D(Vector2 _position, float _radius, Scene::Mesh* _mesh)
		: PhysicBody2D(_position, _radius, _mesh), velocity(Vector2(0, 0)) {}
	
	//1 step continuous collision detection
	//handle only basic move and collision with walls
	void Update(float deltaTime) {
		float box[4] = { Params::Table::width * -0.5f, Params::Table::height * 0.5, Params::Table::width * 0.5, Params::Table::height * -0.5 };
		// 0 - left, 1 - top, 2 - right, 3- bottom
		Vector2 newPos = GetNextFramePos(deltaTime);
		
		if (newPos.y - radius <= box[3]) {
			float collideTime = (box[3] + radius - newPos.y) / (position.y - newPos.y);
			Vector2 collisionPoint = Lerp(position, newPos, collideTime);
			velocity.y *= -1;
			newPos.y = collisionPoint.y + std::abs(newPos.y - collisionPoint.y);
			Move(newPos);
			return;
		}
		
		if (newPos.y + radius >= box[1]) {
			float collideTime = (box[1] + radius - newPos.y) / (position.y - newPos.y);
			Vector2 collisionPoint = Lerp(position, newPos, collideTime);
			velocity.y *= -1;
			newPos.y = collisionPoint.y - std::abs(newPos.y - collisionPoint.y);
			Move(newPos);
			return;
		}
		
		if (newPos.x - radius <= box[0]) {
			float collideTime = (box[3] + radius - newPos.x) / (position.x - newPos.x);
			Vector2 collisionPoint = Lerp(position, newPos, collideTime);
			velocity.x *= -1;
			newPos.x = collisionPoint.x -  std::abs(newPos.x - collisionPoint.x);
			Move(newPos);
			return;
		}

		if (newPos.x + radius >= box[2]) {
			float collideTime = (box[1] + radius - newPos.x) / (position.x - newPos.x);
			Vector2 collisionPoint = Lerp(position, newPos, collideTime);
			velocity.x *= -1;
			newPos.x = collisionPoint.x + std::abs(newPos.x - collisionPoint.x);
			Move(newPos);
			return;
		}

		Move(deltaTime);
		
		
	}
private:
	Vector2 GetNextFramePos(float deltaTime) {
		return position + velocity * deltaTime;
	}
	void Move(float deltaTime) {
		position = GetNextFramePos(deltaTime);
		velocity *= Params::Ball::speedModificatory;
		Scene::placeMesh(mesh, position.x, position.y, 0);
	}

	void Move(Vector2 newPosition) {
		position = newPosition;
		velocity *= Params::Ball::speedModificatory;
		Scene::placeMesh(mesh, position.x, position.y, 0);
	}

};


bool CompareByX(RigidBody2D* rb1, RigidBody2D* rb2) {
	return rb1->position.x < rb2->position.x;
}

//-------------------------------------------------------
//	game public interface
//-------------------------------------------------------
namespace Game
{


	Table table;
	bool isChargingShot = false;
	float shotChargeProgress = 0.f;

	RigidBody2D* playerBall;
	std::vector<RigidBody2D*> ballsObjects;
	std::vector<PhysicBody2D*> pocketObjects;
	void init()
	{
		Engine::setTargetFPS( Params::System::targetFPS );
		Scene::setupBackground( Params::Table::width, Params::Table::height );
		table.init();

		for (size_t i = 0; i < Params::Table::ballsPositions.size(); i++) {
			ballsObjects.push_back(new RigidBody2D( Params::Table::ballsPositions[i], Params::Ball::radius, table.getBall(i)));
		}
		for (size_t i = 0; i < Params::Table::pocketsPositions.size(); i++) {
			pocketObjects.push_back(new PhysicBody2D(Params::Table::pocketsPositions[i], Params::Table::pocketRadius, table.getPocket(i))); 
		}
		playerBall = ballsObjects[0];

	}


	void deinit()
	{
		ballsObjects = {};
		pocketObjects = {};
		delete playerBall;
		table.deinit();
	}

	//cos we have limited (and very low) num of pockets, witch must collide only with balls we can use basic algorithm
	void HandlePocketCollision(RigidBody2D* ball) {
		for (PhysicBody2D* pocket : pocketObjects) {
			bool collision = ball->CheckCollision(pocket);
			if (collision) {
				table.removeBall(ball->mesh);
				ball->Deinit();
				ballsObjects.erase(std::remove(ballsObjects.begin(), ballsObjects.end(), ball), ballsObjects.end());
			}
		}
	}
	
	//basic elastic collision
	void UpdateVelocity(RigidBody2D* rb1, RigidBody2D* rb2) {
		Vector2 v12 = rb1->velocity - rb2->velocity;
		Vector2 x12 = rb1->position - rb2->position;

		Vector2 v21 = rb2->velocity - rb1->velocity;
		Vector2 x21 = rb2->position - rb1->position;

		rb1->velocity -= v12.Dot(x12) / x12.LengthPow2() * x12;
		rb2->velocity -= v21.Dot(x21) / x21.LengthPow2() * x21;
	}
	
	void update( float dt )
	{
		if ( isChargingShot )
			shotChargeProgress = std::min( shotChargeProgress + dt / Params::Shot::chargeTime, 1.f );
		Scene::updateProgressBar( shotChargeProgress );
		for (RigidBody2D* ball : ballsObjects) {
			ball->Update(dt);
			HandlePocketCollision(ball);
		}

		//ball collision
		//Sweep and prune algorithm
		//look along OX cos it wider
		std::vector<RigidBody2D*> snp = ballsObjects;
		std::sort(snp.begin(), snp.end(), CompareByX);
		float start = snp[0]->position.x - snp[0]->radius;
		float end = snp[0]->position.x + snp[0]->radius;
		std::vector<std::vector<RigidBody2D*> > active = { { snp[0] } };
		int globalIt = 0;


		for (int i = 1; i < snp.size(); i++) {
			if (snp[i]->position.x - snp[i]->radius <= end) {
				active[globalIt].push_back(snp[i]);
				end = snp[i]->position.x + snp[i]->radius;
			}
			else
			{
				start = snp[i]->position.x - snp[i]->radius;
				end = snp[i]->position.x + snp[i]->radius;
				globalIt++;
				active.push_back({ snp[i] });
			}
		}
		for (std::vector<RigidBody2D*> act : active) {
			for (int i = 0; i < act.size(); i++) {
				for (int j = i + 1; j < act.size(); j++) {
					if (act[i]->CheckCollision(act[j])) {
						UpdateVelocity(act[i], act[j]);
					}
				}
			}
		}
		active = { {} };
	}


	void mouseButtonPressed( float x, float y )
	{
		isChargingShot = true;
	}


	void mouseButtonReleased( float x, float y )
	{
		playerBall->velocity = Vector2(0, 0);
		playerBall->velocity = playerBall->position.DirectionTo(Vector2(x, y)).Normalized() * shotChargeProgress * Params::Ball::maxSpeed;;
		isChargingShot = false;
		shotChargeProgress = 0.f;
	}
}
