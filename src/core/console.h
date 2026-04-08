#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <variant>
#include <cstdint>

using CVarValue = std::variant<int, float, bool, std::string>;

class CVar {
public:
    CVar() : value(0) {}
    CVar(int v)          : value(v) {}
    CVar(float v)        : value(v) {}
    CVar(bool v)         : value(v) {}
    CVar(const char* v)  : value(std::string(v)) {}
    CVar(const std::string& v) : value(v) {}

    bool IsInt()    const { return std::holds_alternative<int>(value); }
    bool IsFloat()  const { return std::holds_alternative<float>(value); }
    bool IsBool()   const { return std::holds_alternative<bool>(value); }
    bool IsString() const { return std::holds_alternative<std::string>(value); }

    int    GetInt()    const { return std::get<int>(value); }
    float  GetFloat()  const { 
        if (IsFloat()) return std::get<float>(value);
        if (IsInt()) return (float)std::get<int>(value);
        return 0.f;
    }
    bool   GetBool()   const { return std::get<bool>(value); }
    const std::string& GetString() const { return std::get<std::string>(value); }

    void Set(int v)               { value = v; }
    void Set(float v)             { value = v; }
    void Set(bool v)              { value = v; }
    void Set(const std::string& v){ value = v; }

    std::string ToString() const {
        if (IsInt())    return std::to_string(GetInt());
        if (IsFloat())  return std::to_string(GetFloat());
        if (IsBool())   return GetBool() ? "1" : "0";
        return GetString();
    }

    const CVarValue& GetValue() const { return value; }

private:
    CVarValue value;
};

class Console {
public:
    static Console& Get();

    void Toggle();
    bool IsOpen() const { return is_open; }

    void Draw();
    void HandleInput(int key);
    void HandleTextInput(int codepoint);

    CVar* FindCVar(const std::string& name);
    void SetCVar(const std::string& name, const std::string& value);
    void RegisterCVar(const std::string& name, CVar&& cvar);

    template<typename T>
    T GetCVarValue(const std::string& name, T default_val) {
        auto* cv = FindCVar(name);
        if (!cv) return default_val;
        if constexpr (std::is_same_v<T, int>)    return cv->GetInt();
        if constexpr (std::is_same_v<T, float>) return cv->GetFloat();
        if constexpr (std::is_same_v<T, bool>)  return cv->GetBool();
        return default_val;
    }

    using CommandCallback = std::function<void(const std::vector<std::string>&)>;
    void RegisterCommand(const std::string& name, CommandCallback cb);

private:
    Console();
    void ExecuteCommand(const std::string& input);
    void AddOutput(const std::string& msg);

    bool is_open = false;
    std::string input_buffer;
    std::vector<std::string> history;
    int history_index = -1;
    std::vector<std::string> output_lines;
    std::unordered_map<std::string, CVar> cvars;
    std::unordered_map<std::string, CommandCallback> commands;

    static constexpr int MAX_LINES = 100;
    static constexpr int MAX_HISTORY = 50;
};
