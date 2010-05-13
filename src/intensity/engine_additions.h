
// Copyright 2010 Alon Zakai ('kripken'). All rights reserved.
// This file is part of Syntensity/the Intensity Engine, an open source project. See COPYING.txt for licensing.

#include <vector>
#include <map>

#define MAX_ATTACHMENTS 20

//! An entity in the scenario, something that can act or be acted upon. Note that most of the
//! logic occurs on the server; LogicEntity is just for minimal client-side logic.
//!
//! LogicEntity wraps around the Sauer types, so in practice a LogicEntity is either a dynamic entity
//! (PC/NPC - TODO: Make this fpsent? Or do we need movables also?) or a mapmodel. This is completely
//! transparent to users of the LogicEntity class, but they can query the type if they need to.
//!
//! LogicEntities have unique IDs. These are unique in a module (but not a map - entities can
//! move between maps).
struct CLogicEntity
{
    enum { LE_NONE, LE_DYNAMIC, LE_STATIC, LE_NONSAUER}; //!< Possible types for a logic entity, correspond to Sauer types

    ScriptValuePtr scriptEntity; //!< The Scripting logic entity parallel to this one. Set on registration.

    physent*   dynamicEntity;      //!< Only one of dynamicEntity and staticEntity should be not null, corresponding to the type
    extentity* staticEntity;       //!< Only one of dynamicEntity and staticEntity should be not null, corresponding to the type

    bool nonSauer; //!< Whether this is a Sauer (dynamic or static), or a non-Sauer (something non-Sauer related) entity
    int uniqueId; //!< Only used for nonSauer

    //! Used by entities that have their own collision box sizes, i.e., do not use the model's.
    float collisionRadiusWidth;

    //! Used by entities that have their own collision box sizes, i.e., do not use the model's.
    float collisionRadiusHeight;

    //! The model (mesh) for this entity
    model* theModel;

    //! The attachments for this entity
    modelattach attachments[MAX_ATTACHMENTS+2];

    //! For attachments that are position markers, the positions go here XXX: Note that current these are readable only clientside
    //! as they require a call to rendering
    vec attachmentPositions[MAX_ATTACHMENTS];

    //! The current animation for this entity
    int animation;

    //! The start time of the current animation for this entity
    int startTime;

    //! Whether this entity can move on its own volition
    bool canMove;

    //! Sound name of this entity
    std::string soundName;

//    int currAnimationFrame; //!< Saved from sauer's rendering system, used so we know which bounding box to use, for per-frame models
//    int                    lastBIHFrame;       // So we know if we need a new BIH or not, when frames change BUGGY, TODO: Implement fix

    CLogicEntity()                           : dynamicEntity(NULL),           staticEntity(NULL),          nonSauer(false), uniqueId(-8),
        collisionRadiusWidth(-1), collisionRadiusHeight(-1), theModel(NULL), animation(0), startTime(0), lastActualRenderMillis(0)
        { attachments[0].tag = attachments[0].name = NULL; };
    CLogicEntity(physent*    _dynamicEntity) : dynamicEntity(_dynamicEntity), staticEntity(NULL),          nonSauer(false), uniqueId(-8),
        collisionRadiusWidth(-1), collisionRadiusHeight(-1), theModel(NULL), animation(0), startTime(0), lastActualRenderMillis(0)
        { attachments[0].tag = attachments[0].name = NULL; };
    CLogicEntity(extentity* _staticEntity)  : dynamicEntity(NULL),           staticEntity(_staticEntity), nonSauer(false), uniqueId(-8),
        collisionRadiusWidth(-1), collisionRadiusHeight(-1), theModel(NULL), animation(0), startTime(0), lastActualRenderMillis(0)
        { attachments[0].tag = attachments[0].name = NULL; };
    CLogicEntity(int _uniqueId)             : dynamicEntity(NULL),           staticEntity(NULL),          nonSauer(true),
        uniqueId(_uniqueId),
        collisionRadiusWidth(-1), collisionRadiusHeight(-1), theModel(NULL), animation(0), startTime(0), lastActualRenderMillis(0)
        { attachments[0].tag = attachments[0].name = NULL; }; // This is a non-Sauer LE

    //! Returns the unique ID for this entity
    int   getUniqueId();

    //! Returns the type, i.e., dynamic (player, NPC - physent/fpsent), or static (mapmodel). In the future, also lights, etc.
    int   getType();

    //! The location of the entity in the world
    vec   getOrigin();

    //! The radius of the entity. *Not* the bounding box. This is the radius used for visual purposes, e.g., when marked by mouseover
    float getRadius();

    //! Deprecated until we get it to work
    void  setOrigin(vec &newOrigin);

    bool  isNone()    { return getType() == LE_NONE;    };
    bool  isDynamic() { return getType() == LE_DYNAMIC; };
    bool  isStatic()  { return getType() == LE_STATIC;  };

    //! The sauer code for the current running animation
    int  getAnimation();

    //! When the current animation started
    int getStartTime();

    //! The current index to the relevant animation frame for the entity. Needed both for rendering and for bounding-box
    //! calculation of per-frame models, which is also needed on the server
    int getAnimationFrame();

    //! Gets the Scripting class name of a logic entity (e.g., Mapmodel, Player, Door)
    std::string getClass();

    //! Returns the model used to render this entity
    model* getModel();

    //! returns the sound used for this entity
    const char *getSound();

    //! Updates the model based on Scripting information. Refreshes what is needed in Sauer
    void setModel(std::string name);

    //! Updates the sound based on Scripting information. Refreshes what is needed in Sauer
    void setSound(std::string _sound);

    //! Updates the attachments based on Scripting information. Refreshes what is needed in Sauer
    void setAttachments(std::string _attachments);

    //! Updates the animation based on Scripting information. Refreshes what is needed in Sauer. In particular sets the start time.
    void setAnimation(int _animation);

    bool getCanMove() { return canMove; };
    void setCanMove(bool value) { canMove = value; };

    vec& getAttachmentPosition(std::string tag);

    //! Called when we actually render the model. Only such actual renders will calculate
    //! attachment positions, for example. This does not occur if not facing the entity.
    void noteActualRender();

private:
    int lastActualRenderMillis;
};

typedef boost::shared_ptr<CLogicEntity> LogicEntityPtr;


//! The main storage for LogicEntities and management of them. All entities appear in the central list
//! of logic entities here, as well as other scenario-wide data.

#define EMPTY_STATEDATA "-"

struct LogicSystem
{
    typedef std::map<int, LogicEntityPtr> LogicEntityMap;

    static LogicEntityMap logicEntities; //!< All the entities in the scenario

    //! Called before a map loads. Empties list of entities, and unloads the PC logic entity. Removes the scripting engine
    static void clear();

    //! Calls clear(), and creates a new scripting engine
    static void init();

    //! Register a logic entity in the LogicSystem system. Must be done so that entities are accessible and are managed.
    static void          registerLogicEntity(LogicEntityPtr newEntity);

    static LogicEntityPtr registerLogicEntity(physent* entity);
    static LogicEntityPtr registerLogicEntity(extentity* entity);

    //! Register a Logic Entity that is not based on a Sauer type, i.e., is not a physent or an extent
    static void           registerLogicEntityNonSauer(int uniqueId);

    //! Unregisters a C++ GE, removes it from the set of currently running entities
    static void          unregisterLogicEntity(LogicEntityPtr entity);

    //! Unregisters a C++ GE, removes it from the set of currently running entities. Needs to not overload the other,
    //! but have a different name, because we expose this in the scripting embedding
    static void          unregisterLogicEntityByUniqueId(int uniqueId);

    //! Tells the ActionSystems of all of our logic entities to manage themselves, i.e., to run their actions accordingly
    //! This is done both on the client and the server, even on entities not controlled by each. The reason is that we
    //! may e.g. have visual effects on an NPC running on the client, and so forth
    static void          manageActions(long millis);

    static LogicEntityPtr getLogicEntity(int uniqueId);
    static LogicEntityPtr getLogicEntity(const extentity &extent);
    static LogicEntityPtr getLogicEntity(physent* entity);

    static int           getUniqueId(extentity* staticEntity);
    static int           getUniqueId(physent*    dynamicEntity);

    //! Done only in initial preparation of an entity - never afterwards. Note: This is a member of LogicSystem because it would be
    //! invalid as a member of LogicEntity - a LogicEntity, if it exists, must have a valid Id! (i.e., >= 0)
    static void          setUniqueId(extentity* staticEntity, int uniqueId);

    //! Done only in initial preparation of an entity - never afterwards
    static void          setUniqueId(physent* dynamicEntity, int uniqueId);

    static void setupExtent(ScriptValuePtr scriptEntity, int type, float x, float y, float z, int attr1, int attr2, int attr3, int attr4);

    static void setupCharacter(ScriptValuePtr scriptEntity);

    static void setupNonSauer(ScriptValuePtr scriptEntity);

    static void dismantleExtent(ScriptValuePtr scriptEntity);
    static void dismantleCharacter(ScriptValuePtr scriptEntity);
};

