
/*
 *=============================================================================
 * Copyright (C) 2008 Alon Zakai ('Kripken') kripkensteiner@gmail.com
 *
 * This file is part of the Intensity Engine project,
 *    http://www.intensityengine.com
 *
 * The Intensity Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * The Intensity Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Intensity Engine.  If not, see
 *     http://www.gnu.org/licenses/
 *     http://www.gnu.org/licenses/agpl-3.0.html
 *=============================================================================
 */

#include "cube.h"
#include "engine.h"
#include "game.h"

#include "intensity_physics.h"
#include "intensity_physics_realistic.h"
#include "intensity_physics_bullet.h"


// Sauer coordinates are in 'cubes', not metres as in Bullet
#define SAUER_FACTOR 17.0
// Reverse y and z axes
#define FROM_SAUER_VEC(sauervec) ( btVector3(sauervec.x/SAUER_FACTOR, sauervec.z/SAUER_FACTOR, sauervec.y/SAUER_FACTOR) )
#define FROM_SAUER_VEC_NORM(sauervec) ( btVector3(sauervec.x, sauervec.z, sauervec.y) )
#define TO_SAUER_VEC(sauervec, btvec) { sauervec.x = btvec.x()*SAUER_FACTOR; sauervec.y = btvec.z()*SAUER_FACTOR; sauervec.z = btvec.y()*SAUER_FACTOR; }
#define TO_SAUER_QUAT(sauerquat, btquat) { sauerquat.x = -btquat.x(); sauerquat.y = -btquat.z(); sauerquat.z = -btquat.y(); sauerquat.w = btquat.w(); }
#define FROM_SAUER_QUAT(sauerquat) ( btQuaternion(-sauerquat.x, -sauerquat.z, -sauerquat.y, sauerquat.w) )
#define FROM_SAUER_SCALAR(value) ( value/SAUER_FACTOR )


//! Our class for Bullet dynamic entities. Holds everything we need
class IntensityBulletBody : public btRigidBody
{
public:
    btVector3 interpolatedPosition, interpolatedVelocity, interpolatedAngularVelocity;
    btQuaternion interpolatedRotation;
    bool isWorldGeometry;

    IntensityBulletBody(btScalar mass, btMotionState *motionState, btCollisionShape *collisionShape, const btVector3 &localInertia, bool _isWorldGeometry) : btRigidBody(mass, motionState, collisionShape, localInertia)
    {
        isWorldGeometry = _isWorldGeometry;
        interpolatedRotation.setEuler(0,0,0);
    };
};

//! Interface to modify our interpolated values
class IntensityBulletMotionState : public btMotionState
{
public:
    IntensityBulletBody* parent;

    virtual void getWorldTransform (btTransform &worldTrans) const
    { /* printf("getWorldTransform\r\n"); */ } // No need - we set the position/velocity manually

    virtual void setWorldTransform (const btTransform &worldTrans)
    {
        parent->interpolatedPosition = worldTrans.getOrigin();
        parent->interpolatedRotation = worldTrans.getRotation();
        parent->interpolatedVelocity = parent->getLinearVelocity();
        parent->interpolatedAngularVelocity = parent->getAngularVelocity();
    }
};

typedef std::map<physicsHandle, IntensityBulletBody*> handleBodyMap_t;
handleBodyMap_t handleBodyMap;
int handleBodyCounter = 0;


#ifdef CLIENT
    //! The green axis is 'forward'
    class SauerDebugDrawer : public btIDebugDraw
    {
        int m_debugMode;

    public:
        SauerDebugDrawer() : m_debugMode(0) { };

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &fromColor, const btVector3 &toColor)
        {
            vec sauerFrom, sauerTo;
            TO_SAUER_VEC( sauerFrom, from );
            TO_SAUER_VEC( sauerTo, to );

            #define VEC_TO_COLOR(it) \
                ((int((it.x()*0.5+0.5)*255)<<16) + (int((it.y()*0.5+0.5)*255)<<8) + int((it.z()*0.5+0.5)*255))
            particle_flare(sauerFrom, sauerTo, 0, PART_STREAK, VEC_TO_COLOR(fromColor));
//            particle_flare(sauerTo, sauerFrom, 0, PART_STREAK, VEC_TO_COLOR(toColor));
        }

        virtual void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
        {
            drawLine(from, to, color, color);
        }

        virtual void draw3dText (const btVector3 &location, const char *textString)
        {
            vec sauerLocation;
            TO_SAUER_VEC( sauerLocation, location );
            particle_textcopy(sauerLocation, textString, PART_TEXT, 0);
        }

        virtual void drawContactPoint (const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) { }
        virtual void reportErrorWarning (const char *warningString) { printf("Bullet warning: %s\r\n", warningString); }
        virtual void setDebugMode(int debugMode) { m_debugMode = debugMode; }
        virtual int getDebugMode() const { return m_debugMode; }
    };
#endif

void BulletPhysicsEngine::init()
{
    m_collisionConfiguration = new btDefaultCollisionConfiguration();

    m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

    m_broadPhase = new btDbvtBroadphase();

    m_constraintSolver = new btSequentialImpulseConstraintSolver();

    m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher, m_broadPhase, m_constraintSolver, m_collisionConfiguration);

//    m_dynamicsWorld->setGravity(btVector3(0,-10,0));

    // Debug
    #ifdef CLIENT
        m_debugDrawer = new SauerDebugDrawer();
        m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
        m_dynamicsWorld->setDebugDrawer(m_debugDrawer);
    #endif

    m_staticTriangleVertices = NULL;
    m_staticTriangleIndexes = NULL;
    m_indexVertexArrays = NULL;
    m_globalStaticGeometry = NULL;
}

void BulletPhysicsEngine::destroy()
{
    std::vector<physicsHandle> toErase;
    for(handleBodyMap_t::iterator iter = handleBodyMap.begin(); iter != handleBodyMap.end(); iter++)
        toErase.push_back(iter->first);
    for (unsigned int i = 0; i < toErase.size(); i++)
        removeBody(toErase[i]);

    delete m_dynamicsWorld;
    delete m_dispatcher;
    delete m_collisionConfiguration;
    delete m_broadPhase;
    delete m_constraintSolver;

    DELETEP(m_globalStaticGeometry);
    DELETEP(m_indexVertexArrays);
    DELETEP(m_staticTriangleVertices);
    DELETEP(m_staticTriangleIndexes);

    #ifdef CLIENT
        delete m_debugDrawer;
    #endif
}

void BulletPhysicsEngine::setGravity(float g)
{
    m_dynamicsWorld->setGravity(btVector3(0,-FROM_SAUER_SCALAR(g),0));
}

void BulletPhysicsEngine::clearStaticGeometry()
{
    std::vector<physicsHandle> toErase;
    for(handleBodyMap_t::iterator iter = handleBodyMap.begin(); iter != handleBodyMap.end(); iter++)
    {
        IntensityBulletBody* body = iter->second;
        if (body->isWorldGeometry)
            toErase.push_back(iter->first);
    }

    for (unsigned int i = 0; i < toErase.size(); i++)
        removeBody(toErase[i]);

    // Prepare global static
    if (requiresStaticPolygons())
    {
        m_staticTriangles.clear();
    }
}

void BulletPhysicsEngine::addStaticPolygon(std::vector<vec> vertexes)
{
// XXX "Avoid huge or degenerate triangles in a triangle mesh Keep the size of triangles reasonable, say below 10 units/meters."
// - from PDF
// Perhaps we should subdivide triangles that are too big?
// "Also degenerate triangles with large size ratios between each sides or close to zero area can better be avoided."

    // Simple, naive method: a convex hull for each polygon.
    /*
    btConvexHullShape* shape = new btConvexHullShape();
    for (unsigned int i = 0; i < vertexes.size(); i++)
    {
        vec& currVec = vertexes[i];
        btVector3 currBtVec = FROM_SAUER_VEC(currVec);
        shape->addPoint(currBtVec);
    }
    addBody(shape, 0);
    */

    // btBvhTriangleMeshShape method
    assert(vertexes.size() == 3);
    for (unsigned int i = 0; i < vertexes.size(); i++)
    {
        vec& currVec = vertexes[i];
        btVector3 currBtVec = FROM_SAUER_VEC(currVec);
        m_staticTriangles.push_back(currBtVec);
    }
}

#define CREATE_MESH(staticTriangles, staticTriangleVertices, staticTriangleIndexes, indexVertexArrays, shape) \
{ \
    assert(staticTriangles.size() % 3 == 0); \
    int num3 = staticTriangles.size(); \
    int num = num3/3; \
 \
    staticTriangleVertices = new btVector3[num3]; \
    staticTriangleIndexes = new int[num3]; \
 \
    for (int i = 0; i < num3; i++) \
    { \
        staticTriangleVertices[i] = staticTriangles[i]; \
        staticTriangleIndexes[i] = i; \
    } \
 \
    int vertStride = sizeof(btVector3); \
    int indexStride = 3*sizeof(int); \
    indexVertexArrays = new btTriangleIndexVertexArray( \
        num, \
        staticTriangleIndexes, \
        indexStride, \
        num3, \
        (btScalar*) &staticTriangleVertices[0].x(), \
        vertStride \
    ); \
 \
    shape = new btBvhTriangleMeshShape(indexVertexArrays, true); \
}

void BulletPhysicsEngine::finalizeStaticGeometry()
{
    if (!requiresStaticPolygons()) return;

    DELETEP(m_staticTriangleVertices);
    DELETEP(m_staticTriangleIndexes);
    DELETEP(m_indexVertexArrays);
    DELETEP(m_globalStaticGeometry);

    CREATE_MESH(m_staticTriangles, m_staticTriangleVertices, m_staticTriangleIndexes, m_indexVertexArrays, m_globalStaticGeometry);
    addBody(m_globalStaticGeometry, 0, true); // We rely on removal of static bodies elsewhere in the code
}

physicsHandle BulletPhysicsEngine::addBody(btCollisionShape *shape, float mass, bool isWorldGeometry)
{
    btVector3 localInertia(0, 0, 0);
    if (mass > 0)
        shape->calculateLocalInertia(mass, localInertia);
    IntensityBulletMotionState* motionState = new IntensityBulletMotionState();
    IntensityBulletBody* body = new IntensityBulletBody(mass, motionState, shape, localInertia, isWorldGeometry);
    motionState->parent = body;

// body->setCollisionFlags( body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT); 
// body->setActivationState(DISABLE_DEACTIVATION);

    m_dynamicsWorld->addRigidBody(body);

    physicsHandle handle = handleBodyCounter;
    handleBodyMap[handle] = body;
    handleBodyCounter += 1; // TODO: Handle overflow etc. etc. etc.

    Logging::log(Logging::DEBUG, "Physics: Created body: %d\r\n", handle);

    return handle; // garbage collect ***shape***. Also body also motionstate in previous func, etc.}

void BulletPhysicsEngine::removeBody(physicsHandle handle)
{
    Logging::log(Logging::DEBUG, "Physics: Removing body: %d\r\n", handle);

    assert(handleBodyMap.count(handle) == 1);
    IntensityBulletBody* body = handleBodyMap[handle];
    assert(body);
    m_dynamicsWorld->removeRigidBody(body);
    handleBodyMap.erase(handle);
}

physicsHandle BulletPhysicsEngine::addConstraint(btTypedConstraint *constraint)
{
// TODO: a map for these, so can remove them etc.
    m_dynamicsWorld->addConstraint(constraint);
    //constraint->setDbgDrawSize(btScalar(5.f));
    return 0; // XXX
}

void BulletPhysicsEngine::removeConstraint(physicsHandle handle)
{
    assert(0);
}

void BulletPhysicsEngine::addStaticCube(vec o, vec r)
{
    btVector3 halfExtents = FROM_SAUER_VEC(r);
    physicsHandle handle = addBody(new btBoxShape(halfExtents), 0, true);
    setBodyPosition(handle, o);
}

void BulletPhysicsEngine::addStaticConvex(std::vector<vec>& vecs)
{
    unsigned int i;
    vec center(0);
    for (i = 0; i < vecs.size(); i++)
        center.add(vecs[i]);
    center.mul(1.0/vecs.size());

    btConvexHullShape *convex = new btConvexHullShape();
    for (i = 0; i < vecs.size(); i++)
    {
        vec rel = vecs[i];
        rel.sub(center);
        btVector3 btRel = FROM_SAUER_VEC(rel);
        convex->addPoint(btRel);
    }
    physicsHandle handle = addBody(convex, 0, true);
    setBodyPosition(handle, center);
}

physicsHandle BulletPhysicsEngine::addSphere(float mass, float radius)
{
    return addBody(new btSphereShape( FROM_SAUER_SCALAR(radius) ), mass);
}

physicsHandle BulletPhysicsEngine::addBox(float mass, float rx, float ry, float rz)
{
    vec sauerVec(rx/2, ry/2, rz/2);
    btVector3 halfExtents = FROM_SAUER_VEC( sauerVec );
    return addBody(new btBoxShape(halfExtents), mass);
}

physicsHandle BulletPhysicsEngine::addCapsule(float mass, float radius, float height)
{
    return addBody(new btCapsuleShape(FROM_SAUER_SCALAR(radius), FROM_SAUER_SCALAR(height)), mass);
}

physicsHandle BulletPhysicsEngine::addMesh(float mass, std::vector<triangle> &tris)
{
    btAlignedObjectArray<btVector3> staticTriangles;
    btVector3* staticTriangleVertices;
    int* staticTriangleIndexes;
    btStridingMeshInterface* indexVertexArrays;
    btCollisionShape* shape;

    for (unsigned int i = 0; i < tris.size(); i++)
    {
        btVector3 currBtVec;
        #define ADD_VEC(curr) \
            currBtVec = FROM_SAUER_VEC(curr); \
            staticTriangles.push_back(currBtVec);
        ADD_VEC(tris[i].a);
        ADD_VEC(tris[i].b);
        ADD_VEC(tris[i].c);
    }

    CREATE_MESH(staticTriangles, staticTriangleVertices, staticTriangleIndexes, indexVertexArrays, shape);

    return addBody(shape, mass);

    // XXX: We need to dealloc all the structures just allocated
}

#define GET_BODY(handle, body) \
    Logging::log(Logging::DEBUG, "Physics: Accessing body: %d (line %d)\r\n", handle, __LINE__); \
    IntensityBulletBody* body = handleBodyMap[handle]; \
    assert(body);

void BulletPhysicsEngine::setBodyEntity(physicsHandle handle, CLogicEntity* entity)
{
    GET_BODY(handle, body);
    body->setUserPointer(entity);
}

void BulletPhysicsEngine::setBodyPosition(physicsHandle handle, const vec& position)
{
    GET_BODY(handle, body);

    btTransform transform;
    
    transform.setIdentity();
    transform.setOrigin( FROM_SAUER_VEC(position) );
    transform.setRotation(body->getOrientation());
//    transform.setRotation(body->interpolatedRotation); // XXX?
    body->setCenterOfMassTransform(transform);

    // Save in interpolated values, since we might read them soon
    body->interpolatedPosition = transform.getOrigin();

    body->activate();
}

void BulletPhysicsEngine::setBodyRotation(physicsHandle handle, const quat& rotation)
{
    GET_BODY(handle, body);

    btQuaternion btRotation = FROM_SAUER_QUAT(rotation);
    btTransform transform;
    
    transform.setIdentity();
    transform.setOrigin(body->getCenterOfMassTransform().getOrigin());
    transform.setRotation( btRotation );
    body->setCenterOfMassTransform(transform);

    // Save in interpolated values, since we might read them soon
    body->interpolatedRotation = btRotation;

    body->activate();
}

void BulletPhysicsEngine::setBodyVelocity(physicsHandle handle, const vec& velocity)
{
    GET_BODY(handle, body);

    btVector3 btVelocity =  FROM_SAUER_VEC(velocity);
    body->setLinearVelocity( btVelocity );

    // Save in interpolated values, since we might read them soon
    body->interpolatedVelocity = btVelocity;

    body->activate();
}

void BulletPhysicsEngine::setBodyAngularVelocity(physicsHandle handle, const vec& angularVelocity)
{
    GET_BODY(handle, body);

    btVector3 btAngularVelocity =  FROM_SAUER_VEC(angularVelocity);
    body->setAngularVelocity( btAngularVelocity );

    // Save in interpolated values, since we might read them soon
    body->interpolatedAngularVelocity = btAngularVelocity;

    body->activate();
}

void BulletPhysicsEngine::addBodyImpulse(physicsHandle handle, const vec& impulse)
{
    GET_BODY(handle, body);

    body->applyCentralImpulse(FROM_SAUER_VEC(impulse));

    body->activate();
}

void BulletPhysicsEngine::getBodyPosition(physicsHandle handle, vec& position)
{
    GET_BODY(handle, body);

    btTransform trans = body->getWorldTransform();
    btVector3 btPosition = trans.getOrigin();
    TO_SAUER_VEC( position, btPosition );
}

void BulletPhysicsEngine::getBodyRotation(physicsHandle handle, quat& rotation)
{
    GET_BODY(handle, body);

    btTransform trans = body->getWorldTransform();
    btQuaternion btRotation = trans.getRotation();
    TO_SAUER_QUAT( rotation, btRotation );
}

void BulletPhysicsEngine::getBodyVelocity(physicsHandle handle, vec& velocity)
{
    GET_BODY(handle, body);

    btVector3 btVelocity = body->getLinearVelocity();
    TO_SAUER_VEC( velocity, btVelocity );
}

void BulletPhysicsEngine::getBodyAngularVelocity(physicsHandle handle, vec& angularVelocity)
{
    GET_BODY(handle, body);

    btVector3 btAngularVelocity = body->getAngularVelocity();
    TO_SAUER_VEC( angularVelocity, btAngularVelocity );
}

void BulletPhysicsEngine::setLinearFactor(physicsHandle handle, vec& factor)
{
    GET_BODY(handle, body);

    body->setLinearFactor(FROM_SAUER_VEC_NORM(factor));
}

void BulletPhysicsEngine::setAngularFactor(physicsHandle handle, vec& factor)
{
    GET_BODY(handle, body);

    body->setAngularFactor(FROM_SAUER_VEC_NORM(factor));
}

physicsHandle BulletPhysicsEngine::addConstraintP2P(physicsHandle handleA, physicsHandle handleB, vec& pivotA, vec& pivotB)
{
    GET_BODY(handleA, bodyA);
    GET_BODY(handleB, bodyB);

    return addConstraint(new btPoint2PointConstraint(
        *bodyA,
        *bodyB,
        FROM_SAUER_VEC(pivotA),
        FROM_SAUER_VEC(pivotB)
    ));
}

VAR(bulletdebug, 0, 0, 1);

namespace game
{
    extern int smoothmove;
    void predictplayer(fpsent *d, bool move);
}

#define DUMP_DATA \
    for(handleBodyMap_t::iterator iter = handleBodyMap.begin(); iter != handleBodyMap.end(); iter++) \
    { \
        IntensityBulletBody* body = iter->second; \
        if (fabs(body->getInvMass() - 1/40.0) < 1/60.0) \
        { \
            vec pos, vel; \
            getBodyPosition(iter->first, pos); \
            getBodyVelocity(iter->first, vel); \
            printf("   %d : %f,%f,%f , %f,%f,%f\r\n", iter->first, pos.x, pos.y, pos.z, vel.x, vel.y, vel.z); \
        } \
    }

void BulletPhysicsEngine::simulate(float seconds)
{
    if (Logging::shouldShow(Logging::DEBUG)) { printf("PRE\r\n"); DUMP_DATA; }

    m_dynamicsWorld->stepSimulation(seconds, 3); // We may run at 30fps, so sometimes need ~2 frames

    if (Logging::shouldShow(Logging::DEBUG)) { printf("POST\r\n"); DUMP_DATA; }

    #ifdef CLIENT
        if (bulletdebug)
        {
            m_debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
            m_dynamicsWorld->debugDrawWorld();
        } else
            m_debugDrawer->setDebugMode(0);

        // Smooth interp, and roll normalization
        loopv(game::players)
        {
            fpsent* fpsEntity = game::players[i];
            LogicEntityPtr entity = LogicSystem::getLogicEntity(fpsEntity);
            if (!entity.get() || entity->isNone()) continue;

            fpsEntity->roll = fpsEntity->roll/(1+(float)sqrtf((float)curtime)/25);

            if(fpsEntity != player)
            {
                if (game::smoothmove && fpsEntity->smoothmillis>0)
                    game::predictplayer(fpsEntity, false);
            }
        }
    #endif

/*
This will happen in predictplayer: So need to counter it?
        d->o = d->newpos;
        d->yaw = d->newyaw;
        d->pitch = d->newpitch;
*/
// XXX TODO:    game::predictplayer(*, false); - interpolate remote players
}


class IgnoringContactResultCallback : public btCollisionWorld::ContactResultCallback
{
    void* _ignore;
public:
    IgnoringContactResultCallback(void* ignore) : _ignore(ignore), hit(false) { };

    bool hit;
    bool hasHit() { return hit; };

    virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObject* colObj0, int partId0, int index0, const btCollisionObject* colObj1, int partId1, int index1)
        {
            if (dynamic_cast<const IntensityBulletBody*>(colObj0) && colObj0->getUserPointer() == _ignore) return 0;
            if (dynamic_cast<const IntensityBulletBody*>(colObj1) && colObj1->getUserPointer() == _ignore) return 0;
            hit = true;
            return 0;
        }
};

bool BulletPhysicsEngine::isColliding(vec& position, float radius, CLogicEntity *ignore)
{
    btSphereShape sphere(FROM_SAUER_SCALAR(radius));
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin( FROM_SAUER_VEC(position) );
    btCollisionObject collider;
    collider.setCollisionShape(&sphere);
    collider.setWorldTransform(transform);
    IgnoringContactResultCallback cb(ignore);
    m_dynamicsWorld->contactTest(&collider, cb);
    return cb.hasHit();
}

void BulletPhysicsEngine::rayCastClosest(vec &from, vec &to, float& hitDist, LogicEntityPtr& hitEntity, CLogicEntity* ignore)
{
    btVector3 btFrom = FROM_SAUER_VEC(from),
              btTo   = FROM_SAUER_VEC(to);
    btCollisionWorld::ClosestRayResultCallback cb(btFrom, btTo);
    m_dynamicsWorld->rayTest(btFrom, btTo, cb);
    if (cb.m_closestHitFraction < 1.0)
    {
        vec temp(to);
        temp.sub(from);
        hitDist = cb.m_closestHitFraction * temp.magnitude();
    } else {
        hitDist = -1;
    }
    hitEntity.reset(); // XXX TODO, also ignore
}


// Utilities

void jiggle()
{
    player->o.z += 5;
}

COMMAND(jiggle, "");

