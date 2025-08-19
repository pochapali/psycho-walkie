#include "lua.h"

#include <psyqo-lua/lua.hh>

#include <psyqo/xprintf.h>

#include "gameobject.hh"

constexpr const char GAMEOBJECT_SCRIPT[] = R"(
return function(metatable)
    local get_position = metatable.get_position
    local set_position = metatable.set_position

    metatable.get_position = nil
    metatable.set_position = nil

    function metatable.__index(self, key)
        if key == "position" then
            return get_position(self.__cpp_ptr)
        end
        return nil
    end

    function metatable.__newindex(self, key, value)
        if key == "position" then
            set_position(self.__cpp_ptr, value)
            return
        end
        rawset(self, key, value)
    end
end
)";

// Lua helpers

static int gameobjectSetPosition(psyqo::Lua L) {
    auto go = L.toUserdata<psxsplash::GameObject>(1);
    L.getField(2, "x");
    go->position.x = L.toFixedPoint(3);
    L.pop();
    L.getField(2, "y");
    go->position.y = L.toFixedPoint(3);
    L.pop();
    L.getField(2, "z");
    go->position.z = L.toFixedPoint(3);
    L.pop();
    return 0;
}

static int gameobjectGetPosition(psyqo::Lua L) {
    auto go = L.toUserdata<psxsplash::GameObject>(1);
    L.newTable();
    L.push(go->position.x);
    L.setField(2, "x");
    L.push(go->position.y);
    L.setField(2, "y");
    L.push(go->position.z);
    L.setField(2, "z");
    return 1;
}

void psxsplash::Lua::Init() {
    auto L = m_state;
    // Load and run the game objects script
    if (L.loadBuffer(GAMEOBJECT_SCRIPT, "buffer:gameObjects") == 0) {
        if (L.pcall(0, 1) == 0) {
            // This will be our metatable
            L.newTable();

            L.push(gameobjectGetPosition);
            L.setField(-2, "get_position");

            L.push(gameobjectSetPosition);
            L.setField(-2, "set_position");

            L.copy(-1);
            m_metatableReference = L.ref();

            if (L.pcall(1, 0) == 0) {
                printf("Lua script 'gameObjects' executed successfully");
            } else {
                printf("Error registering Lua script: %s\n", L.optString(-1, "Unknown error"));
                L.clearStack();
                return;
            }
        } else {
            // Print Lua error if script execution fails
            printf("Error executing Lua script: %s\n", L.optString(-1, "Unknown error"));
            L.clearStack();
            return;
        }
    } else {
        // Print Lua error if script loading fails
        printf("Error loading Lua script: %s\n", L.optString(-1, "Unknown error"));
        L.clearStack();
        return;
    }

    L.newTable();
    m_luascriptsReference = L.ref();
}

void psxsplash::Lua::LoadLuaFile(const char* code, size_t len, int index) {
    auto L = m_state;
    char filename[32];
    snprintf(filename, sizeof(filename), "lua_asset:%d", index);
    if (L.loadBuffer(code, len, filename) != LUA_OK) {
        printf("Lua error: %s\n", L.toString(-1));
        L.pop();
    }
    // (1) script func
    L.rawGetI(LUA_REGISTRYINDEX, m_luascriptsReference);
    // (1) script func (2) scripts table
    L.newTable();
    // (1) script func (2) scripts table (3) {}
    L.pushNumber(index);
    // (1) script func (2) scripts table (3) {} (4) index
    L.copy(-2);
    // (1) script func (2) scripts table (3) {} (4) index (5) {}
    L.setTable(-4);
    // (1) script func (2) scripts table (3) {}
    lua_setupvalue(L.getState(), -3, 1);
    // (1) script func (2) scripts table
    L.pop();
    // (1) script func
    if (L.pcall(0, 0)) {
        printf("Lua error: %s\n", L.toString(-1));
        L.pop();
    }
}

void psxsplash::Lua::RegisterSceneScripts(int index) {
    if (index < 0) return;
    auto L = m_state;
    L.newTable();
    // (1) {}
    L.copy(1);
    // (1) {} (2) {}
    m_luaSceneScriptsReference = L.ref();
    // (1) {}
    L.rawGetI(LUA_REGISTRYINDEX, m_luascriptsReference);
    // (1) {} (2) scripts table
    L.pushNumber(index);
    // (1) {} (2) script environments table (2) index
    L.getTable(-2);
    // (1) {} (2) script environments table (3) script environment table for the scene
    onSceneCreationStartFunctionWrapper.resolveGlobal(L);
    onSceneCreationEndFunctionWrapper.resolveGlobal(L);
    L.pop(3);
    // empty stack
}

// We're going to store the Lua table for the object at the address of the object,
// and the table for its methods at the address of the object + 1 byte.
void psxsplash::Lua::RegisterGameObject(GameObject* go) {
    uint8_t* ptr = reinterpret_cast<uint8_t*>(go);
    auto L = m_state;
    L.push(ptr);
    // (1) go
    L.newTable();
    // (1) go (2) {}
    L.push(ptr);
    // (1) go (2) {} (3) go
    L.setField(-2, "__cpp_ptr");
    // (1) go (2) { __cpp_ptr = go }
    L.rawGetI(LUA_REGISTRYINDEX, m_metatableReference);
    // (1) go (2) { __cpp_ptr = go } (3) metatable
    if (L.isTable(-1)) {
        L.setMetatable(-2);
    } else {
        printf("Warning: metatableForAllGameObjects not found\n");
        L.pop();
    }
    // (1) go (2) { __cpp_ptr = go + metatable }
    L.rawSet(LUA_REGISTRYINDEX);
    // empty stack
    L.newTable();
    // (1) {}
    L.push(ptr + 1);
    // (1) {} (2) go + 1
    L.copy(1);
    // (1) {} (2) go + 1 (3) {}
    L.rawSet(LUA_REGISTRYINDEX);
    // (1) {}
    if (go->luaFileIndex != -1) {
        L.rawGetI(LUA_REGISTRYINDEX, m_luascriptsReference);
        // (1) {} (2) script environments table
        L.rawGetI(-1, go->luaFileIndex);
        // (1) {} (2) script environments table (3) script environment table for this object
        onCollisionMethodWrapper.resolveGlobal(L);
        onInteractMethodWrapper.resolveGlobal(L);
        L.pop(2);
        // (1) {}
    }
    L.pop();
    // empty stack
    printf("GameObject registered in Lua registry: %p\n", ptr);
}

void psxsplash::Lua::OnCollision(GameObject* self, GameObject* other) {
    onCollisionMethodWrapper.callMethod(*this, self, other);
}

void psxsplash::Lua::OnInteract(GameObject* self) {
    onInteractMethodWrapper.callMethod(*this, self);
}

void psxsplash::Lua::PushGameObject(GameObject* go) {
    auto L = m_state;
    L.push(go);
    L.rawGet(LUA_REGISTRYINDEX);

    if (!L.isTable(-1)) {
        printf("Warning: GameObject not found in Lua registry\n");
        L.pop();
    }
}
