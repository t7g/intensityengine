
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include "engine.h"

#include "utility.h"
#include "script_engine_manager.h"

#include "intensity_physics.h"
#include "intensity_physics_realistic.h"

#include "intensity_physics_sauer.h" // Just for line with creation - nothing else
#ifdef INTENSITY_BULLET
    #include "intensity_physics_bullet.h" // Just for line with creation - nothing else
#endif
#ifdef CLIENT
    #include "client_system.h"
#else // SERVER
    #include "server_system.h"
#endif

namespace PhysicsManager
{

PhysicsEngine *engine = NULL;

//

void createEngine(std::string type)
{
    destroyEngine();

    Logging::log(Logging::DEBUG, "Creating physics engine: %s\r\n", type.c_str());

    if (type == "sauer")
    {
        Logging::log(Logging::DEBUG, "Using sauer physics engine\r\n");
        engine = new SauerPhysicsEngine();
    }
#ifdef INTENSITY_BULLET
    else if (type == "bullet")
    {
        Logging::log(Logging::DEBUG, "Using bullet physics engine\r\n");
        engine = new BulletPhysicsEngine();
    }
#endif
    else
    {
        #ifdef CLIENT
            Logging::log(Logging::ERROR, "Invalid physics engine: %s, disconnecting\r\n", type.c_str());
            EXEC_PYTHON(
                "def do_disconnect():\n"
                "    CModule.disconnect()\n"
                "main_actionqueue.add_action(do_disconnect)\n"
            ); // We are loading a map now - must disconnect after that is complete
            return;
        #else // SERVER
            Logging::log(Logging::ERROR, "Invalid physics engine: %s, quitting\r\n", type.c_str());
            ServerSystem::fatalMessageToClients("Invalid physics engine, quitting");
            assert(0);
        #endif
    }

    engine->init();
}

void destroyEngine()
{
    if (engine != NULL)
    {
        Logging::log(Logging::DEBUG, "Destroying physics engine\r\n");

        engine->destroy();
        delete engine;
        engine = NULL;
    }
}

bool hasEngine()
{
    return (engine != NULL);
}

PhysicsEngine* getEngine()
{
    assert(engine);
    return engine;
}

#define REQUIRE_ENGINE if (!hasEngine()) return; // Sauer calls physics before the first map even loads

void clearWorldGeometry()
{
    REQUIRE_ENGINE

    Logging::log(Logging::DEBUG, "*** Clear world geometry ***\r\n");

    engine->clearStaticGeometry();
}

vec* currVecs = NULL;

void setupWorldGeometryVerts(vector<vertex>& verts)
{
    REQUIRE_ENGINE

    if (!engine->requiresStaticPolygons()) return;

    renderprogress(0, "generating physics vertexes");

    unsigned int nVertices = verts.length();
    currVecs = new vec[nVertices];

    for (unsigned int i = 0; i < nVertices; i++)
    {
        currVecs[i] = verts[i].pos;
    }
}


void setupWorldGeometryTriGroup(vector<ushort>& data, int tex, int lightmapTex, int orientation)
{
    REQUIRE_ENGINE

    if (!engine->requiresStaticPolygons()) return;

    static int counter = 0;
    renderprogress(-float(counter)/1000, "generating physics simulation verts");
    counter++;
    if (counter == 1000) counter = 0;

    // Count indexes (three indexes to a triangle)
    unsigned int ibufCount = data.length();
    if (ibufCount == 0)
        return;
    assert(ibufCount % 3 == 0);
    unsigned int numTris = ibufCount/3;

    Logging::log(Logging::DEBUG, "IO: setupWorldGeometryTriGroup: %d\r\n", ibufCount);

    std::vector<vec> currPolygon;
    int base;
    for (unsigned int i = 0; i < numTris; i++)
    {
        base = i*3;
        currPolygon.clear();
        currPolygon.push_back( currVecs[data[base+0]] );
        currPolygon.push_back( currVecs[data[base+1]] );
        currPolygon.push_back( currVecs[data[base+2]] );
        engine->addStaticPolygon(currPolygon);
    }
}


void finishWorldGeometryVerts()
{
    REQUIRE_ENGINE

    delete[] currVecs;
    currVecs = NULL;
}

// Cube world processing utilities

    void loopOctree(cube* c, int size, ivec o);

    void processOctanode(cube* c, int size, ivec o)
    {
        if (!c->children)
        {
            renderprogress(float(o.x)/getworldsize(), "processing octree for physics...");

            Logging::log(Logging::DEBUG, "processOctanode: %4d,%4d,%4d : %4d,%4d,%4d   (%.8x,%.8x,%.8x)\r\n", o.x, o.y, o.z, o.x+size, o.y+size, o.z+size, c->faces[0], c->faces[1], c->faces[2]);

            if (isentirelysolid(*c))
                engine->addStaticCube(vec(o.x+size/2, o.y+size/2, o.z+size/2), vec(size/2));
            else if (!isempty(*c))
            {
                // Not fully solid, create convex shape with the verts
                // TODO: Optimize, use addStaticCube when rectangular
                Logging::log(Logging::DEBUG, "Not fully solid nor empty\r\n");
                vec verts[8];
                bool usefaces[8];
                int vertused = 0;
                loopi(8) if((usefaces[i] = visibletris(*c, i, o.x, o.y, o.z, size))) vertused |= fvmasks[1<<i];
                std::vector<vec> vecs;
                loopi(8) if(vertused&(1<<i))
                {
					vec t = verts[i].mul(8);
                    Logging::log(Logging::INFO, "vv: %f,%f,%f\r\n", t.x, t.y, t.z);
                    vecs.push_back(t);
                }
                assert(vecs.size() > 0);

                // Test for simple rectangular objects, which we sent as Cubes, not Convexes
                std::set<int> dimensionValues[3];
                int dimensionMins[3], dimensionMaxes[3];
                for (unsigned int j = 0; j < vecs.size(); j++)
                {
                    for (int k = 0; k < 3; k++)
                    {
                        dimensionValues[k].insert(vecs[j][k]);
                        if (j > 0)
                        {
                            dimensionMins[k] = min(dimensionMins[k], int(vecs[j][k]));
                            dimensionMaxes[k] = max(dimensionMaxes[k], int(vecs[j][k]));
                        } else {
                            dimensionMins[k] = vecs[j][k];
                            dimensionMaxes[k] = vecs[j][k];
                        }
                    }
                }
                for (int k = 0; k < 3; k++)
                    if (dimensionValues[k].size() > 2)
                    {
                        Logging::log(Logging::DEBUG, "Adding as Convex\r\n");
                        engine->addStaticConvex(vecs);
                        return;
                    }
                // This might be a cube - check that it fills the bounded space
                int found[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
                for (int i = 0; i < 2; i++)
                    for (int j = 0; j < 2; j++)
                        for (int k = 0; k < 2; k++)
                            for (unsigned int l = 0; l < vecs.size(); l++)
                            {
                                vec target(
                                    i ? dimensionMaxes[0] : dimensionMins[0],
                                    j ? dimensionMaxes[1] : dimensionMins[1],
                                    k ? dimensionMaxes[2] : dimensionMins[2]
                                );
                                if (vecs[l] == target)
                                    found[i+j*2+k*4]=1;
                            }
                for (int i = 0; i < 8; i++)
                    if (!found[i])
                    {
                        Logging::log(Logging::DEBUG, "In the end, adding as Convex\r\n");
                        engine->addStaticConvex(vecs);
                        return;
                    }

                Logging::log(Logging::DEBUG, "Adding as Cube\r\n");
                engine->addStaticCube(vec(
                    (dimensionMins[0]+dimensionMaxes[0])/2,
                    (dimensionMins[1]+dimensionMaxes[1])/2,
                    (dimensionMins[2]+dimensionMaxes[2])/2
                ), vec(
                    (dimensionMaxes[0]-dimensionMins[0])/2,
                    (dimensionMaxes[1]-dimensionMins[1])/2,
                    (dimensionMaxes[2]-dimensionMins[2])/2
                ));
            }
        } else {
            loopOctree(c->children, size, o);
        }
    }

    void loopOctree(cube* c, int size, ivec o)
    {
        for (int z = 0; z <= 1; z++)
            for (int y = 0; y <= 1; y++)
                for (int x = 0; x <= 1; x++)
                    processOctanode(&c[x+y*2+z*4], size/2, ivec(o.x + x*size/2, o.y + y*size/2, o.z + z*size/2));
    }

void finalizeWorldGeometry()
{
    REQUIRE_ENGINE

    Logging::log(Logging::DEBUG, "*** Finalize world geometry ***\r\n");

    if (engine->requiresStaticCubes())
    {
        renderprogress(0, "generating physics geometries");

        // Loop the octree and provide the physics engine with the cube info
        loopOctree(worldroot, worldsize, vec(0));
    }

    engine->finalizeStaticGeometry();
}


// Simulation

void simulate(float seconds)
{
    REQUIRE_ENGINE

    engine->simulate(seconds);
}

};

