# Unity-API (Beta)

> **This project is currently in Beta.** It is actively under development, and the architecture, APIs, and features are subject to change in future updates.


An advanced, high-performance C++ framework for deep integration, runtime manipulation, and interoperability with Unity engine internals. Designed as a superior alternative and evolution of **BNM (By Name Modding)**, it delivers a cleaner standard C++ architecture, faster execution, and significantly enhanced capabilities.

Built with true cross-platform compatibility in mind, it provides robust out-of-the-box support for **Windows (x64/x86), Android (ARM64/ARMv7), Linux**, and more. Whether you are dealing with IL2CPP or Mono scripting backends, Unity-API offers seamless access to native Unity components, memory structures, and complex networking layers.

## Architecture & Structure

The SDK is logically divided into distinct modules, which can be found in their respective subdirectories:

### 1. `unity` (Engine Wrappers)
Contains C++ representations of Unity's core engine classes. 
- Wrappers for essential components: `CGameObject`, `CTransform`, `CCamera`, `CComponent`, `CRigidbody`, `CMaterial`, etc.
- Provides native memory interaction with game objects, allowing you to manipulate their state, physics, and rendering natively.

### 2. `il2cpp` (IL2CPP Backend)
Direct integration with the IL2CPP runtime environment.
- Native C++ wrappers mimicking IL2CPP internals: `CClass`, `CMethod`, `CField`, `CImage`, `CAssembly`, `CType`.
- Support for internal calls (`ICall`).

### 3. `mono` (Scripting Data Types)
Implements standard C# / Mono data structures natively in C++.
- Includes generic collections: `SArray`, `SList`, `SDictionary`, `SHashset`.
- Core C# types: `SString` (System.String mapping), `SDelegate`, and `SNullable`.

### 4. `photon` (Networking)
Built-in integration for Photon network engines.
- Includes support modules for multiple Photon architectures: **PUN** (Photon Unity Networking), **Fusion**, and **Voice**.
- **Configuration:** You can easily toggle and configure the active Photon SDK modules inside `photon/include.hpp`.

### 5. `game` (Game-Specific Logic)
A dedicated module for implementing hooks, controllers, and features for specific games.

## Core Headers

The root of the `sdk` directory contains the foundational macros, memory offsets, and settings.

- **`sdk_core.hpp`**: The main configuration and entry point header. Provides:
  - Dynamic resolution macros: `RESOLVE_SDK_IL2CPP_CLASS`, `RESOLVE_SDK_MONO_TYPE`, `RESOLVE_SDK_METHOD`.
  - Configurable settings via macros: **String Obfuscation** (powered by `oxorany`), and **Safe Calls** with optional built-in logging.
- **`sdk_fwd.hpp`**: Fast forward declarations of core IL2CPP and Mono types to minimize header inclusion overhead.
- **`sdk_il2cpp_exports.hpp`**: Defines standard exported symbols from `GameAssembly.dll` / `libil2cpp.so.`
- **`sdk_il2cpp_offsets.hpp`**: Contains exact memory offsets for IL2CPP internal structures (`CClass`, `CMethod`, etc.) with support for both **32-bit** and **64-bit (x64/ARM64)** architectures.

## Usage Guide

Unity-API provides powerful native classes and macros to simplify reverse engineering and modding.

### 1. Resolving Classes
To interact with an IL2CPP object, you must first find its class. You can use the native API or the provided macro (which caches the result for better performance).

```cpp
// Native API approach
il2cpp::CClass* pClass = il2cpp::CClass::FromName("PlayerController", "GameNamespace", "GameAssembly.dll");
```

### 2. Resolving Methods
Once you have a class pointer, you can resolve methods by their name and argument count.

```cpp
// Native API approach
il2cpp::CMethod* pMethodInfo = pClass->GetMethodFromName("set_Health", 1);

// 1. Call via IL2CPP Invoker (automatically packs arguments)
int returnValue;
int newHealth = 100;
pMethodInfo->Invoke(pPlayerInstance, &returnValue, &newHealth);

// 2. Call directly via Native Pointer (Fastest, bypasses Invoker)
pMethodInfo->InvokeViaPtr<void, int>(pPlayerInstance, 100);
```

### 3. Reading/Writing Fields
You can find fields by name and get their memory offset to read or write values directly from/to an instance pointer.

```cpp
// 1. Get the field info from the class
il2cpp::CField* pHealthField = pClass->GetFieldFromName("health");

// 2. Read or Write the value safely using the templated methods
int currentHealth = pHealthField->GetValue<int>(pPlayerInstance);
pHealthField->SetValue<int>(pPlayerInstance, 999);
```

## Example Component Wrapper

Here is a complete example demonstrating how to wrap a game component, utilizing dynamic runtime resolution and the SDK's macro system:

```cpp
#include "sdk/sdk_core.hpp"

namespace gamesdk
{
    class CMyPlayer : public usdk::CComponent
    {
    public:
        // Dynamically resolve the class at runtime
        static il2cpp::CClass* GetIl2cppClass()
        {
            RESOLVE_SDK_IL2CPP_CLASS("MyPlayer", "GameAssembly", "GameAssembly.dll");
        }
        
        // Resolve a Mono Type
        static mono::CType* GetMonoType()
        {
            RESOLVE_SDK_MONO_TYPE("MyPlayer");
        }
        
        // Resolve a method and call it safely
        int GetHealth()
        {
            RESOLVE_SDK_METHOD(int (*)(void*), GetIl2cppClass(), "get_Health", 0);
            return func(this); // Handled safely via SDK_SAFE_CALLS macro
        }
        
        void SetHealth(int value)
        {
            RESOLVE_SDK_METHOD(void (*)(void*, int), GetIl2cppClass(), "set_Health", 1);
            func(this, value);
        }
    };
}
```
