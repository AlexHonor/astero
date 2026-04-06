#pragma once

class GameState;

class SaveSystem {
public:
    static void Save(const GameState& state, const char* path = "save.dat");
    static bool Load(GameState& state, const char* path = "save.dat");
};
