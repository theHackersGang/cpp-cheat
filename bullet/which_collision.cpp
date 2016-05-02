/*
Determine which object has collided with which object:
http://gamedev.stackexchange.com/questions/22442/how-get-collision-callback-of-two-specific-objects-using-bullet-physics

Derived from drop_object.cpp
*/

#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>

#include <btBulletDynamicsCommon.h>

#define PRINTF_FLOAT "%7.3f"

constexpr float gravity = -10.0f;
constexpr float timeStep = 1.0f / 60.0f;
constexpr float groundRestitution = 0.9f;
constexpr float objectRestitution = 0.9f;
constexpr int maxNPoints = 500;
constexpr float initialXs[] = {  0.0f, 5.0f };
constexpr float initialYs[] = { 10.0f, 5.0f };
constexpr size_t nObjects = sizeof(initialYs) / sizeof(*initialYs);

std::map<const btCollisionObject*,std::vector<btManifoldPoint*>> objectsCollisions;
void myTickCallback(btDynamicsWorld *dynamicsWorld, btScalar timeStep) {
    objectsCollisions.clear();
    int numManifolds = dynamicsWorld->getDispatcher()->getNumManifolds();
    for (int i = 0; i < numManifolds; i++) {
        btPersistentManifold *contactManifold = dynamicsWorld->getDispatcher()->getManifoldByIndexInternal(i);
        auto *objA = contactManifold->getBody0();
        auto *objB = contactManifold->getBody1();
        auto& collisionsA = objectsCollisions[objA];
        auto& collisionsB = objectsCollisions[objB];
        int numContacts = contactManifold->getNumContacts();
        for (int j = 0; j < numContacts; j++) {
            btManifoldPoint& pt = contactManifold->getContactPoint(j);
            collisionsA.push_back(&pt);
            collisionsB.push_back(&pt);
        }
    }
}

int main() {
    int i, j;

    btDefaultCollisionConfiguration *collisionConfiguration
            = new btDefaultCollisionConfiguration();
    btCollisionDispatcher *dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btBroadphaseInterface *overlappingPairCache = new btDbvtBroadphase();
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;
    btDiscreteDynamicsWorld *dynamicsWorld = new btDiscreteDynamicsWorld(
            dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, gravity, 0));
    dynamicsWorld->setInternalTickCallback(myTickCallback);
    btAlignedObjectArray<btCollisionShape*> collisionShapes;

    // Ground.
    {
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0, 0, 0));
        btCollisionShape* groundShape;
        groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), -1);
        collisionShapes.push_back(groundShape);
		btDefaultMotionState* myMotionState = new btDefaultMotionState(groundTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(0, myMotionState, groundShape, btVector3(0, 0, 0));
		btRigidBody* body = new btRigidBody(rbInfo);
		body->setRestitution(groundRestitution);
		dynamicsWorld->addRigidBody(body);
    }

    // Objects.
    std::vector<btRidigBody*> objects;
    for (size_t i = 0; i < nObjects; ++i) {
        btCollisionShape *colShape;
        colShape = new btSphereShape(btScalar(1.0));
        collisionShapes.push_back(colShape);
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(initialXs[i], initialYs[i], 0));
        btVector3 localInertia(0, 0, 0);
        btScalar mass(1.0f);
        colShape->calculateLocalInertia(mass, localInertia);
        btDefaultMotionState *myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody *body = new btRigidBody(btRigidBody::btRigidBodyConstructionInfo(
                mass, myMotionState, colShape, localInertia));
		body->setRestitution(objectRestitution);
        dynamicsWorld->addRigidBody(body);
        objects.push_back(body);
    }

    // Main loop.
    std::printf("step body x y z collision2 collision1\n");
    for (i = 0; i < maxNPoints; ++i) {
        dynamicsWorld->stepSimulation(timeStep);
        for (j = dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; --j) {
            btCollisionObject *obj = dynamicsWorld->getCollisionObjectArray()[j];
            btRigidBody *body = btRigidBody::upcast(obj);
            btTransform trans;
            if (body && body->getMotionState()) {
                body->getMotionState()->getWorldTransform(trans);
            } else {
                trans = obj->getWorldTransform();
            }
            btVector3 origin = trans.getOrigin();
            std::printf("%d %d " PRINTF_FLOAT " " PRINTF_FLOAT " " PRINTF_FLOAT " ",
                    i, j, origin.getX(), origin.getY(), origin.getZ());
            // We could use objects[i] here to check for one of the objects we've created.
            auto& manifoldPoints = objectsCollisions[body];
            if (manifoldPoints.empty()) {
                std::printf("0");
            } else {
                std::printf("1");
            }
            puts("");
        }
    }

    // Cleanup.
    for (i = dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
        btCollisionObject* obj = dynamicsWorld->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState()) {
            delete body->getMotionState();
        }
        dynamicsWorld->removeCollisionObject(obj);
        delete obj;
    }
    for (i = 0; i < collisionShapes.size(); ++i) {
        delete collisionShapes[i];
    }
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
    collisionShapes.clear();
}
