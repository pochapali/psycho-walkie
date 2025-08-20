#pragma once

#include <stdint.h>

#include "gameobject.hh"
#include "psyqo-lua/lua.hh"
#include "psyqo/xprintf.h"
#include "typestring.h"

namespace psxsplash {

struct LuaFile {
    union {
        uint32_t luaCodeOffset;
        const char* luaCode;
    };
    uint32_t length;
};

class Lua {
  public:
    void Init();

    void LoadLuaFile(const char* code, size_t len, int index);
    void RegisterSceneScripts(int index);
    void RegisterGameObject(GameObject* go);

    void OnSceneCreationStart() {
        onSceneCreationStartFunctionWrapper.callFunction(*this);
    }
    void OnSceneCreationEnd() {
        onSceneCreationEndFunctionWrapper.callFunction(*this);
    }
    void OnCollision(GameObject* self, GameObject* other);
    void OnInteract(GameObject* self);

  private:
    template <int methodId, typename methodName>
    struct FunctionWrapper;
    template <int methodId, char... C>
    struct FunctionWrapper<methodId, irqus::typestring<C...>> {
        typedef irqus::typestring<C...> methodName;
        // Needs the methods table at index 1, and the script environment table at index 3
        static void resolveGlobal(psyqo::Lua L) {
            // Push the method name string to access the environment table
            L.push(methodName::data(), methodName::size());  
            L.getTable(3);  
            
            if (L.isFunction(-1)) {
                // Store the function in methods table using numeric ID as key
                L.pushNumber(methodId);     // Push numeric key for methods table
                L.copy(-2);                 // Push the function (copy from top -2)
                L.setTable(1);              // methodsTable[methodId] = function
            } else {
                L.pop();  // Pop the non-function value
            }
        }
        template <typename... Args>
        static void pushArgs(psxsplash::Lua& lua, Args... args) {
            (push(lua, args), ...);
        }
        static void push(psxsplash::Lua& lua, GameObject* go) { lua.PushGameObject(go); }
        template <typename... Args>
        static void callMethod(psxsplash::Lua& lua, GameObject* go, Args... args) {
            auto L = lua.m_state;
            uint8_t* ptr = reinterpret_cast<uint8_t*>(go);
            L.push(ptr + 1);
            L.rawGet(LUA_REGISTRYINDEX);
            L.rawGetI(-1, methodId);
            if (!L.isFunction(-1)) {
                L.clearStack();
                return;
            }
            lua.PushGameObject(go);
            pushArgs(lua, args...);
            if (L.pcall(sizeof...(Args) + 1, 0) != LUA_OK) {
                printf("Lua error: %s\n", L.toString(-1));
            }
            L.clearStack();
        }
        template <typename... Args>
        static void callFunction(psxsplash::Lua& lua, Args... args) {
            auto L = lua.m_state;
            L.push(methodName::data(), methodName::size());
            L.rawGetI(LUA_REGISTRYINDEX, lua.m_metatableReference);
            if (!L.isFunction(-1)) {
                L.clearStack();
                return;
            }
            pushArgs(lua, args...);
            if (L.pcall(sizeof...(Args), 0) != LUA_OK) {
                printf("Lua error: %s\n", L.toString(-1));
            }
            L.clearStack();
        }
    };

    [[no_unique_address]] FunctionWrapper<1, typestring_is("onSceneCreationStart")> onSceneCreationStartFunctionWrapper;
    [[no_unique_address]] FunctionWrapper<2, typestring_is("onSceneCreationEnd")> onSceneCreationEndFunctionWrapper;
    [[no_unique_address]] FunctionWrapper<1, typestring_is("onCreate")> onCreateMethodWrapper;
    [[no_unique_address]] FunctionWrapper<2, typestring_is("onCollision")> onCollisionMethodWrapper;
    [[no_unique_address]] FunctionWrapper<3, typestring_is("onInteract")> onInteractMethodWrapper;
    void PushGameObject(GameObject* go);
    psyqo::Lua m_state;

    int m_metatableReference;
    int m_luascriptsReference;
    int m_luaSceneScriptsReference;

    template <int methodId, typename methodName>
    friend struct FunctionWrapper;
};
}  // namespace psxsplash
