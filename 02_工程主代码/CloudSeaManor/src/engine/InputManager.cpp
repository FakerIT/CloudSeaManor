#include "CloudSeamanor/InputManager.hpp"
#include "CloudSeamanor/Logger.hpp"
#include "CloudSeamanor/Profiling.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace CloudSeamanor::engine {

namespace {

constexpr const char* K_A = "A";
constexpr const char* K_B = "B";
constexpr const char* K_C = "C";
constexpr const char* K_D = "D";
constexpr const char* K_E = "E";
constexpr const char* K_F = "F";
constexpr const char* K_G = "G";
constexpr const char* K_H = "H";
constexpr const char* K_I = "I";
constexpr const char* K_J = "J";
constexpr const char* K_K = "K";
constexpr const char* K_L = "L";
constexpr const char* K_M = "M";
constexpr const char* K_N = "N";
constexpr const char* K_O = "O";
constexpr const char* K_P = "P";
constexpr const char* K_Q = "Q";
constexpr const char* K_R = "R";
constexpr const char* K_S = "S";
constexpr const char* K_T = "T";
constexpr const char* K_U = "U";
constexpr const char* K_V = "V";
constexpr const char* K_W = "W";
constexpr const char* K_X = "X";
constexpr const char* K_Y = "Y";
constexpr const char* K_Z = "Z";
constexpr const char* K_Space = "Space";
constexpr const char* K_Enter = "Enter";
constexpr const char* K_Escape = "Escape";
constexpr const char* K_Tab = "Tab";
constexpr const char* K_Backspace = "Backspace";
constexpr const char* K_Delete = "Delete";
constexpr const char* K_Up = "Up";
constexpr const char* K_Down = "Down";
constexpr const char* K_Left = "Left";
constexpr const char* K_Right = "Right";
constexpr const char* K_LShift = "LShift";
constexpr const char* K_RShift = "RShift";
constexpr const char* K_LControl = "LControl";
constexpr const char* K_RControl = "RControl";
constexpr const char* K_LAlt = "LAlt";
constexpr const char* K_RAlt = "RAlt";
constexpr const char* K_Num0 = "Num0";
constexpr const char* K_Num1 = "Num1";
constexpr const char* K_Num2 = "Num2";
constexpr const char* K_Num3 = "Num3";
constexpr const char* K_Num4 = "Num4";
constexpr const char* K_Num5 = "Num5";
constexpr const char* K_Num6 = "Num6";
constexpr const char* K_Num7 = "Num7";
constexpr const char* K_Num8 = "Num8";
constexpr const char* K_Num9 = "Num9";
constexpr const char* K_None = "(None)";
constexpr const char* K_Unknown = "?";

constexpr const char* A_MoveUp = "MoveUp";
constexpr const char* A_MoveDown = "MoveDown";
constexpr const char* A_MoveLeft = "MoveLeft";
constexpr const char* A_MoveRight = "MoveRight";
constexpr const char* A_Interact = "Interact";
constexpr const char* A_OpenInventory = "Inventory";
constexpr const char* A_OpenMenu = "Menu";
constexpr const char* A_Confirm = "Confirm";
constexpr const char* A_Cancel = "Cancel";
constexpr const char* A_Sprint = "Sprint";
constexpr const char* A_UseTool = "UseTool";
constexpr const char* A_EatFood = "EatFood";
constexpr const char* A_OpenQuest = "Quest";
constexpr const char* A_OpenMap = "Map";
constexpr const char* A_AdvanceDialog = "AdvanceDialog";
constexpr const char* A_GiftNpc = "Gift";
constexpr const char* A_Sleep = "Sleep";
constexpr const char* A_DebugToggle = "Debug";
constexpr const char* A_CloudToggle = "Cloud";
constexpr const char* A_QuickSave = "QuickSave";
constexpr const char* A_QuickLoad = "QuickLoad";
constexpr const char* A_Hotbar1 = "Hotbar1";
constexpr const char* A_Hotbar2 = "Hotbar2";
constexpr const char* A_Hotbar3 = "Hotbar3";
constexpr const char* A_Hotbar4 = "Hotbar4";
constexpr const char* A_Hotbar5 = "Hotbar5";
constexpr const char* A_Hotbar6 = "Hotbar6";
constexpr const char* A_Hotbar7 = "Hotbar7";
constexpr const char* A_Hotbar8 = "Hotbar8";
constexpr const char* A_Hotbar9 = "Hotbar9";
constexpr const char* A_Hotbar10 = "Hotbar10";
constexpr const char* A_Hotbar11 = "Hotbar11";
constexpr const char* A_Hotbar12 = "Hotbar12";
constexpr const char* A_Unknown = "Unknown";

const std::unordered_map<std::string, sf::Keyboard::Key>& KEY_MAP() {
    static const std::unordered_map<std::string, sf::Keyboard::Key> map = {
        {K_A, sf::Keyboard::Key::A}, {K_B, sf::Keyboard::Key::B}, {K_C, sf::Keyboard::Key::C},
        {K_D, sf::Keyboard::Key::D}, {K_E, sf::Keyboard::Key::E}, {K_F, sf::Keyboard::Key::F},
        {K_G, sf::Keyboard::Key::G}, {K_H, sf::Keyboard::Key::H}, {K_I, sf::Keyboard::Key::I},
        {K_J, sf::Keyboard::Key::J}, {K_K, sf::Keyboard::Key::K}, {K_L, sf::Keyboard::Key::L},
        {K_M, sf::Keyboard::Key::M}, {K_N, sf::Keyboard::Key::N}, {K_O, sf::Keyboard::Key::O},
        {K_P, sf::Keyboard::Key::P}, {K_Q, sf::Keyboard::Key::Q}, {K_R, sf::Keyboard::Key::R},
        {K_S, sf::Keyboard::Key::S}, {K_T, sf::Keyboard::Key::T}, {K_U, sf::Keyboard::Key::U},
        {K_V, sf::Keyboard::Key::V}, {K_W, sf::Keyboard::Key::W}, {K_X, sf::Keyboard::Key::X},
        {K_Y, sf::Keyboard::Key::Y}, {K_Z, sf::Keyboard::Key::Z},
        {K_Space, sf::Keyboard::Key::Space}, {K_Enter, sf::Keyboard::Key::Enter},
        {K_Escape, sf::Keyboard::Key::Escape}, {K_Tab, sf::Keyboard::Key::Tab},
        {K_Backspace, sf::Keyboard::Key::Backspace}, {K_Delete, sf::Keyboard::Key::Delete},
        {K_Up, sf::Keyboard::Key::Up}, {K_Down, sf::Keyboard::Key::Down},
        {K_Left, sf::Keyboard::Key::Left}, {K_Right, sf::Keyboard::Key::Right},
        {K_LShift, sf::Keyboard::Key::LShift}, {K_RShift, sf::Keyboard::Key::RShift},
        {K_LControl, sf::Keyboard::Key::LControl}, {K_RControl, sf::Keyboard::Key::RControl},
        {K_LAlt, sf::Keyboard::Key::LAlt}, {K_RAlt, sf::Keyboard::Key::RAlt},
        {K_Num0, sf::Keyboard::Key::Num0}, {K_Num1, sf::Keyboard::Key::Num1},
        {K_Num2, sf::Keyboard::Key::Num2}, {K_Num3, sf::Keyboard::Key::Num3},
        {K_Num4, sf::Keyboard::Key::Num4}, {K_Num5, sf::Keyboard::Key::Num5},
        {K_Num6, sf::Keyboard::Key::Num6}, {K_Num7, sf::Keyboard::Key::Num7},
        {K_Num8, sf::Keyboard::Key::Num8}, {K_Num9, sf::Keyboard::Key::Num9},
    };
    return map;
}

std::string TrimCopy(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }

    return value.substr(begin, end - begin);
}

std::string ToUpperAsciiCopy(const std::string& value) {
    std::string upper = value;
    std::transform(upper.begin(), upper.end(), upper.begin(), [](const unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    return upper;
}

const std::unordered_map<std::string, Action>& ACTION_MAP() {
    static const std::unordered_map<std::string, Action> map = {
        {"MOVEUP", Action::MoveUp},
        {"MOVEDOWN", Action::MoveDown},
        {"MOVELEFT", Action::MoveLeft},
        {"MOVERIGHT", Action::MoveRight},
        {"INTERACT", Action::Interact},
        {"INVENTORY", Action::OpenInventory},
        {"OPENINVENTORY", Action::OpenInventory},
        {"QUEST", Action::OpenQuestMenu},
        {"OPENQUESTMENU", Action::OpenQuestMenu},
        {"MAP", Action::OpenMap},
        {"OPENMAP", Action::OpenMap},
        {"CANCEL", Action::Cancel},
        {"MENU", Action::OpenMenu},
        {"OPENMENU", Action::OpenMenu},
        {"CONFIRM", Action::Confirm},
        {"SPRINT", Action::Sprint},
        {"USETOOL", Action::UseTool},
        {"EATFOOD", Action::EatFood},
        {"EAT", Action::EatFood},
        {"ADVANCEDIALOG", Action::AdvanceDialog},
        {"GIFT", Action::GiftNpc},
        {"GIFTNPC", Action::GiftNpc},
        {"SLEEP", Action::Sleep},
        {"DEBUG", Action::DebugToggle},
        {"DEBUGTOGGLE", Action::DebugToggle},
        {"CLOUD", Action::CloudToggle},
        {"CLOUDTOGGLE", Action::CloudToggle},
        {"QUICKSAVE", Action::QuickSave},
        {"QUICKLOAD", Action::QuickLoad},
        {"HOTBAR1", Action::HotbarSlot1},
        {"HOTBAR2", Action::HotbarSlot2},
        {"HOTBAR3", Action::HotbarSlot3},
        {"HOTBAR4", Action::HotbarSlot4},
        {"HOTBAR5", Action::HotbarSlot5},
        {"HOTBAR6", Action::HotbarSlot6},
        {"HOTBAR7", Action::HotbarSlot7},
        {"HOTBAR8", Action::HotbarSlot8},
        {"HOTBAR9", Action::HotbarSlot9},
        {"HOTBAR10", Action::HotbarSlot10},
        {"HOTBAR11", Action::HotbarSlot11},
        {"HOTBAR12", Action::HotbarSlot12},
    };
    return map;
}

bool TryParseActionName(const std::string& raw_name, Action& out_action) {
    const std::string normalized = ToUpperAsciiCopy(TrimCopy(raw_name));
    const auto found = ACTION_MAP().find(normalized);
    if (found == ACTION_MAP().end()) {
        return false;
    }
    out_action = found->second;
    return true;
}

bool TryParseKeyName(const std::string& raw_name, sf::Keyboard::Key& out_key) {
    const std::string normalized = ToUpperAsciiCopy(TrimCopy(raw_name));
    for (const auto& [name, key] : KEY_MAP()) {
        if (ToUpperAsciiCopy(name) == normalized) {
            out_key = key;
            return true;
        }
    }
    return false;
}

}  // namespace

InputManager::InputManager() {
    ResetToDefaults();
    RebuildCurrentState_();
    previous_ = current_;
}

void InputManager::BeginNewFrame() {
    previous_ = current_;
    RebuildCurrentState_();
}

void InputManager::HandleEvent(const sf::Event& event) {
    CSM_ZONE_SCOPED;
    // 当前实现以实时按键状态为准，事件流仅保留接口兼容性。
    (void)event;
    RebuildCurrentState_();
}

bool InputManager::IsActionPressed(Action action) const {
    return current_.IsActionPressed(action);
}

bool InputManager::IsActionJustPressed(Action action) const {
    return current_.IsActionPressed(action)
        && !previous_.IsActionPressed(action);
}

sf::Vector2f InputManager::GetMovementVector() const {
    float x = 0.0f;
    float y = 0.0f;
    if (IsActionPressed(Action::MoveUp))    y -= 1.0f;
    if (IsActionPressed(Action::MoveDown))  y += 1.0f;
    if (IsActionPressed(Action::MoveLeft))  x -= 1.0f;
    if (IsActionPressed(Action::MoveRight)) x += 1.0f;
    const float len = std::sqrt(x * x + y * y);
    if (len > 0.0f) {
        return sf::Vector2f(x / len, y / len);
    }
    return sf::Vector2f(0.0f, 0.0f);
}

void InputManager::BindKey(sf::Keyboard::Key key, Action action) {
    auto& keys = action_bindings_[static_cast<std::size_t>(action)];
    const auto exists = std::find(keys.begin(), keys.end(), key) != keys.end();
    if (!exists) {
        keys.push_back(key);
    }
}

bool InputManager::LoadFromFile(const std::string& path) {
    std::ifstream stream(path);
    if (!stream.is_open()) {
        infrastructure::Logger::Warning("InputManager: cannot open key binding config: " + path
                                        + ", fallback to default bindings.");
        return false;
    }

    bool loaded_any_binding = false;
    std::string line;
    int line_number = 0;
    while (std::getline(stream, line)) {
        ++line_number;
        const std::string trimmed = TrimCopy(line);
        if (trimmed.empty() || trimmed[0] == '#') {
            continue;
        }

        const std::size_t equals_pos = trimmed.find('=');
        if (equals_pos == std::string::npos) {
            infrastructure::Logger::Warning("InputManager: invalid key binding line "
                                            + std::to_string(line_number) + " (missing '=')");
            continue;
        }

        Action action = Action::Count;
        const std::string action_name = trimmed.substr(0, equals_pos);
        if (!TryParseActionName(action_name, action)) {
            infrastructure::Logger::Warning("InputManager: unknown action '" + TrimCopy(action_name)
                                            + "' at line " + std::to_string(line_number));
            continue;
        }

        std::istringstream key_stream(trimmed.substr(equals_pos + 1));
        std::string token;
        std::vector<sf::Keyboard::Key> parsed_keys;
        while (std::getline(key_stream, token, ',')) {
            const std::string key_name = TrimCopy(token);
            if (key_name.empty()) {
                continue;
            }

            sf::Keyboard::Key key = sf::Keyboard::Key::Unknown;
            if (!TryParseKeyName(key_name, key)) {
                infrastructure::Logger::Warning("InputManager: unknown key '" + key_name
                                                + "' for action '" + TrimCopy(action_name) + "'");
                continue;
            }
            const auto key_exists = std::find(parsed_keys.begin(), parsed_keys.end(), key) != parsed_keys.end();
            if (!key_exists) {
                parsed_keys.push_back(key);
            }
        }

        if (!parsed_keys.empty()) {
            action_bindings_[static_cast<std::size_t>(action)] = std::move(parsed_keys);
            loaded_any_binding = true;
        }
    }

    if (!loaded_any_binding) {
        infrastructure::Logger::Warning("InputManager: no valid bindings loaded from " + path
                                        + ", fallback to defaults.");
        ResetToDefaults();
        return false;
    }

    RebuildCurrentState_();
    previous_ = current_;
    return true;
}

void InputManager::ResetToDefaults() {
    for (auto& keys : action_bindings_) {
        keys.clear();
    }

    BindKey(sf::Keyboard::Key::W, Action::MoveUp);
    BindKey(sf::Keyboard::Key::Up, Action::MoveUp);
    BindKey(sf::Keyboard::Key::S, Action::MoveDown);
    BindKey(sf::Keyboard::Key::Down, Action::MoveDown);
    BindKey(sf::Keyboard::Key::A, Action::MoveLeft);
    BindKey(sf::Keyboard::Key::Left, Action::MoveLeft);
    BindKey(sf::Keyboard::Key::D, Action::MoveRight);
    BindKey(sf::Keyboard::Key::Right, Action::MoveRight);
    BindKey(sf::Keyboard::Key::E, Action::Interact);
    BindKey(sf::Keyboard::Key::E, Action::AdvanceDialog);
    BindKey(sf::Keyboard::Key::I, Action::OpenInventory);
    BindKey(sf::Keyboard::Key::F, Action::OpenQuestMenu);
    BindKey(sf::Keyboard::Key::M, Action::OpenMap);
    BindKey(sf::Keyboard::Key::Escape, Action::Cancel);
    BindKey(sf::Keyboard::Key::Escape, Action::OpenMenu);
    BindKey(sf::Keyboard::Key::Enter, Action::Confirm);
    BindKey(sf::Keyboard::Key::LShift, Action::Sprint);
    BindKey(sf::Keyboard::Key::Q, Action::UseTool);
    BindKey(sf::Keyboard::Key::H, Action::EatFood);
    BindKey(sf::Keyboard::Key::G, Action::GiftNpc);
    BindKey(sf::Keyboard::Key::T, Action::Sleep);
    BindKey(sf::Keyboard::Key::F3, Action::DebugToggle);
    BindKey(sf::Keyboard::Key::F5, Action::CloudToggle);
    BindKey(sf::Keyboard::Key::F6, Action::QuickSave);
    BindKey(sf::Keyboard::Key::F9, Action::QuickLoad);
    BindKey(sf::Keyboard::Key::Num1, Action::HotbarSlot1);
    BindKey(sf::Keyboard::Key::Num2, Action::HotbarSlot2);
    BindKey(sf::Keyboard::Key::Num3, Action::HotbarSlot3);
    BindKey(sf::Keyboard::Key::Num4, Action::HotbarSlot4);
    BindKey(sf::Keyboard::Key::Num5, Action::HotbarSlot5);
    BindKey(sf::Keyboard::Key::Num6, Action::HotbarSlot6);
    BindKey(sf::Keyboard::Key::Num7, Action::HotbarSlot7);
    BindKey(sf::Keyboard::Key::Num8, Action::HotbarSlot8);
    BindKey(sf::Keyboard::Key::Num9, Action::HotbarSlot9);
    BindKey(sf::Keyboard::Key::Num0, Action::HotbarSlot10);
    BindKey(sf::Keyboard::Key::Hyphen, Action::HotbarSlot11);
    BindKey(sf::Keyboard::Key::Equal, Action::HotbarSlot12);

    RebuildCurrentState_();
    previous_ = current_;
}

sf::Keyboard::Key InputManager::GetDefaultKey(Action action) const {
    switch (action) {
    case Action::MoveUp:       return sf::Keyboard::Key::W;
    case Action::MoveDown:     return sf::Keyboard::Key::S;
    case Action::MoveLeft:     return sf::Keyboard::Key::A;
    case Action::MoveRight:    return sf::Keyboard::Key::D;
    case Action::Interact:     return sf::Keyboard::Key::E;
    case Action::OpenInventory: return sf::Keyboard::Key::I;
    case Action::OpenQuestMenu: return sf::Keyboard::Key::F;
    case Action::OpenMap:      return sf::Keyboard::Key::M;
    case Action::Cancel:        return sf::Keyboard::Key::Escape;
    case Action::OpenMenu:     return sf::Keyboard::Key::Escape;
    case Action::Confirm:       return sf::Keyboard::Key::Enter;
    case Action::Sprint:        return sf::Keyboard::Key::LShift;
    case Action::UseTool:      return sf::Keyboard::Key::Q;
    case Action::EatFood:      return sf::Keyboard::Key::H;
    case Action::AdvanceDialog: return sf::Keyboard::Key::E;
    case Action::GiftNpc:      return sf::Keyboard::Key::G;
    case Action::Sleep:         return sf::Keyboard::Key::T;
    case Action::DebugToggle:   return sf::Keyboard::Key::F3;
    case Action::CloudToggle:   return sf::Keyboard::Key::F5;
    case Action::QuickSave:     return sf::Keyboard::Key::F6;
    case Action::QuickLoad:     return sf::Keyboard::Key::F9;
    case Action::HotbarSlot1:  return sf::Keyboard::Key::Num1;
    case Action::HotbarSlot2:  return sf::Keyboard::Key::Num2;
    case Action::HotbarSlot3:  return sf::Keyboard::Key::Num3;
    case Action::HotbarSlot4:  return sf::Keyboard::Key::Num4;
    case Action::HotbarSlot5:  return sf::Keyboard::Key::Num5;
    case Action::HotbarSlot6:  return sf::Keyboard::Key::Num6;
    case Action::HotbarSlot7:  return sf::Keyboard::Key::Num7;
    case Action::HotbarSlot8:  return sf::Keyboard::Key::Num8;
    case Action::HotbarSlot9:  return sf::Keyboard::Key::Num9;
    case Action::HotbarSlot10: return sf::Keyboard::Key::Num0;
    case Action::HotbarSlot11: return sf::Keyboard::Key::Hyphen;
    case Action::HotbarSlot12: return sf::Keyboard::Key::Equal;
    case Action::Count:        return sf::Keyboard::Key::Unknown;
    }
    return sf::Keyboard::Key::Unknown;
}

sf::Keyboard::Key InputManager::GetPrimaryKey(Action action) const {
    const auto& keys = action_bindings_[static_cast<std::size_t>(action)];
    if (!keys.empty()) {
        return keys.front();
    }
    return GetDefaultKey(action);
}

std::string InputManager::GetPrimaryKeyName(Action action) const {
    return KeyName(GetPrimaryKey(action));
}

sf::Vector2f InputManager::GetGamepadVector() const {
    return sf::Vector2f(0.0f, 0.0f);
}

bool InputManager::IsGamepadConnected() const {
    return false;
}

void InputManager::RebuildCurrentState_() {
    current_.pressed.fill(false);
    for (std::size_t i = 0; i < action_bindings_.size(); ++i) {
        const auto& keys = action_bindings_[i];
        const bool pressed = std::any_of(keys.begin(), keys.end(), [](const sf::Keyboard::Key key) {
            return sf::Keyboard::isKeyPressed(key);
        });
        current_.pressed[i] = pressed;
    }
}

const char* ActionDisplayName(Action action) {
    switch (action) {
    case Action::MoveUp:       return A_MoveUp;
    case Action::MoveDown:     return A_MoveDown;
    case Action::MoveLeft:    return A_MoveLeft;
    case Action::MoveRight:    return A_MoveRight;
    case Action::Interact:     return A_Interact;
    case Action::OpenInventory: return A_OpenInventory;
    case Action::OpenQuestMenu: return A_OpenQuest;
    case Action::OpenMap:      return A_OpenMap;
    case Action::Cancel:        return A_Cancel;
    case Action::OpenMenu:     return A_OpenMenu;
    case Action::Confirm:       return A_Confirm;
    case Action::Sprint:        return A_Sprint;
    case Action::UseTool:      return A_UseTool;
    case Action::EatFood:      return A_EatFood;
    case Action::AdvanceDialog: return A_AdvanceDialog;
    case Action::GiftNpc:      return A_GiftNpc;
    case Action::Sleep:         return A_Sleep;
    case Action::DebugToggle:   return A_DebugToggle;
    case Action::CloudToggle:   return A_CloudToggle;
    case Action::QuickSave:     return A_QuickSave;
    case Action::QuickLoad:     return A_QuickLoad;
    case Action::HotbarSlot1:  return A_Hotbar1;
    case Action::HotbarSlot2:  return A_Hotbar2;
    case Action::HotbarSlot3:  return A_Hotbar3;
    case Action::HotbarSlot4:  return A_Hotbar4;
    case Action::HotbarSlot5:  return A_Hotbar5;
    case Action::HotbarSlot6:  return A_Hotbar6;
    case Action::HotbarSlot7:  return A_Hotbar7;
    case Action::HotbarSlot8:  return A_Hotbar8;
    case Action::HotbarSlot9:  return A_Hotbar9;
    case Action::HotbarSlot10: return A_Hotbar10;
    case Action::HotbarSlot11: return A_Hotbar11;
    case Action::HotbarSlot12: return A_Hotbar12;
    case Action::Count:        return A_Unknown;
    }
    return A_Unknown;
}

std::string KeyName(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Key::Unknown) return K_None;
    for (const auto& [name, k] : KEY_MAP()) {
        if (k == key) return name;
    }
    return K_Unknown;
}

}  // namespace CloudSeamanor::engine
