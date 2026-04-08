#include "console.h"
#include <algorithm>
#include <cctype>
#include <sstream>

static Console* s_console = nullptr;

Console& Console::Get() {
    if (!s_console) s_console = new Console();
    return *s_console;
}

Console::Console() {
    RegisterCVar("fps", CVar(0));
    RegisterCommand("help", [](const std::vector<std::string>&) {
        Console::Get().AddOutput("Commands: help, clear, echo, set");
        Console::Get().AddOutput("Usage: set <cvar> <value>");
    });
    RegisterCommand("clear", [this](const std::vector<std::string>&) {
        (void)this;
        output_lines.clear();
    });
    RegisterCommand("echo", [this](const std::vector<std::string>& args) {
        std::string msg;
        for (size_t i = 1; i < args.size(); i++) {
            if (i > 1) msg += " ";
            msg += args[i];
        }
        AddOutput(msg);
    });
    AddOutput("Console ready. Press ~ to toggle. Type 'help' for commands.");
}

void Console::Toggle() {
    is_open = !is_open;
    input_buffer.clear();
    history_index = -1;
}

void Console::AddOutput(const std::string& msg) {
    if (!msg.empty()) {
        output_lines.push_back(msg);
        if ((int)output_lines.size() > MAX_LINES) {
            output_lines.erase(output_lines.begin());
        }
    }
}

void Console::HandleTextInput(int codepoint) {
    if (codepoint > 0 && codepoint < 256 && std::isprint(static_cast<unsigned char>(codepoint))) {
        input_buffer += static_cast<char>(codepoint);
    }
}

void Console::HandleInput(int key) {
    if (key == KEY_ESCAPE) {
        is_open = false;
        return;
    }
    if (key == KEY_TAB) {
        return;
    }
    if (key == KEY_ENTER) {
        if (!input_buffer.empty()) {
            AddOutput("> " + input_buffer);
            ExecuteCommand(input_buffer);
            history.push_back(input_buffer);
            if ((int)history.size() > MAX_HISTORY) {
                history.erase(history.begin());
            }
            input_buffer.clear();
            history_index = -1;
        }
        return;
    }
    if (key == KEY_BACKSPACE) {
        if (!input_buffer.empty()) {
            input_buffer.pop_back();
        }
        return;
    }
    if (key == KEY_UP) {
        if (!history.empty()) {
            if (history_index < (int)history.size() - 1) {
                history_index++;
            }
            if (history_index >= 0 && history_index < (int)history.size()) {
                input_buffer = history[history.size() - 1 - history_index];
            }
        }
        return;
    }
    if (key == KEY_DOWN) {
        if (history_index > 0) {
            history_index--;
            input_buffer = history[history.size() - 1 - history_index];
        } else {
            history_index = -1;
            input_buffer.clear();
        }
        return;
    }
}

CVar* Console::FindCVar(const std::string& name) {
    auto it = cvars.find(name);
    return it != cvars.end() ? &it->second : nullptr;
}

void Console::RegisterCVar(const std::string& name, CVar&& cvar) {
    cvars[name] = std::move(cvar);
}

void Console::RegisterCommand(const std::string& name, CommandCallback cb) {
    commands[name] = std::move(cb);
}

void Console::SetCVar(const std::string& name, const std::string& value) {
    auto* cv = FindCVar(name);
    if (!cv) {
        AddOutput("Unknown cvar: " + name);
        return;
    }

    if (cv->IsInt()) {
        try {
            cv->Set(std::stoi(value));
            AddOutput(name + " = " + value);
        } catch (...) {
            AddOutput("Invalid integer: " + value);
        }
    } else if (cv->IsFloat()) {
        try {
            cv->Set(std::stof(value));
            AddOutput(name + " = " + value);
        } catch (...) {
            AddOutput("Invalid float: " + value);
        }
    } else if (cv->IsBool()) {
        cv->Set(value == "1" || value == "true" || value == "yes");
        AddOutput(name + " = " + (cv->GetBool() ? "1" : "0"));
    } else {
        cv->Set(value);
        AddOutput(name + " = " + value);
    }
}

void Console::ExecuteCommand(const std::string& input) {
    if (input.empty()) return;

    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;

    if (cmd == "set") {
        std::string name, value;
        iss >> name >> value;
        if (name.empty() || value.empty()) {
            AddOutput("Usage: set <cvar> <value>");
        } else {
            SetCVar(name, value);
        }
        return;
    }

    auto it = commands.find(cmd);
    if (it != commands.end()) {
        std::vector<std::string> args;
        std::string arg;
        while (iss >> arg) args.push_back(arg);
        args.insert(args.begin(), cmd);
        it->second(args);
    } else {
        AddOutput("Unknown command: " + cmd);
    }
}

void Console::Draw() {
    if (!is_open) return;

    const int W = GetScreenWidth();
    const int H = GetScreenHeight();
    const int CONSOLE_H = H / 3;
    const int PAD = 10;
    const int LINE_H = 18;
    const int INPUT_H = 28;

    DrawRectangle(0, 0, W, CONSOLE_H, Color{15, 15, 20, 240});
    DrawRectangleLines(0, 0, W, CONSOLE_H, Color{80, 80, 100, 200});

    DrawText("Console", PAD, PAD, 14, Color{180, 180, 255, 255});

    int visible_lines = (CONSOLE_H - PAD * 2 - INPUT_H - 24) / LINE_H;
    int start_idx = (int)output_lines.size() - visible_lines;
    if (start_idx < 0) start_idx = 0;

    for (int i = start_idx; i < (int)output_lines.size(); i++) {
        int y = PAD + 24 + (i - start_idx) * LINE_H;
        DrawText(output_lines[i].c_str(), PAD, y, 12, WHITE);
    }

    int input_y = CONSOLE_H - INPUT_H - PAD;
    DrawRectangle(PAD, input_y, W - PAD * 2, INPUT_H, Color{30, 30, 40, 255});
    DrawText(("> " + input_buffer + "_").c_str(), PAD + 4, input_y + 6, 14, GREEN);
}
