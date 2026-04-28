#pragma once
// 【SceneManager】栈式场景管理器
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
namespace CloudSeamanor::engine {

enum class SceneType { None=0, MainMenu, GameWorld, DialogueOverlay, InventoryOverlay, SettingsOverlay, SaveLoadOverlay, Count };

class Scene {
public:
    virtual ~Scene() = default;
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnPause() {}
    virtual void OnResume() {}
    virtual void Update(float delta) { (void)delta; }
    virtual void Render(sf::RenderWindow& window) { (void)window; }
    [[nodiscard]] virtual bool BlocksUpdate() const { return true; }
    [[nodiscard]] virtual bool BlocksRender() const { return true; }
    [[nodiscard]] virtual std::string Name() const { return "Scene"; }
};

class SceneManager {
public:
    SceneManager() = default;
    template <typename SceneT, typename... Args> void SwitchTo(Args&&... args);
    template <typename SceneT, typename... Args> void Push(Args&&... args);
    void Pop();
    void Pop(std::size_t count);
    template <typename SceneT> void PopUntil();
    void Update(float delta);
    void Render(sf::RenderWindow& window);
    [[nodiscard]] Scene* Top() const;
    [[nodiscard]] Scene& TopRef() const;
    [[nodiscard]] bool Empty() const { return scenes_.empty(); }
    [[nodiscard]] std::size_t Size() const { return scenes_.size(); }
    template <typename SceneT> [[nodiscard]] SceneT* Find() const;
    void Clear();
private:
    // 栈式语义：back() 为 Top()；使用 vector 便于遍历（避免 unique_ptr 拷贝问题）
    std::vector<std::unique_ptr<Scene>> scenes_;
};

// ============================================================================
// 【SceneTransition】淡入淡出场景过渡（BE-029）
// ============================================================================
// 设计目的：
// - 为“切换地图/切换场景标记”等跨系统动作提供统一过场动画
// - 允许在过场中点执行一次切换回调（例如重建 TMX 场景）
// ============================================================================
class SceneTransition {
public:
    enum class State : std::uint8_t { Idle, FadeOut, FadeIn };

    /**
     * @brief 启动一次淡入淡出过场
     * @param fade_out_seconds 变黑耗时
     * @param fade_in_seconds  变亮耗时
     * @param on_midpoint      变黑完成时回调（用于切换地图/状态）
     * @param on_complete      过场完成时回调
     */
    void Start(
        float fade_out_seconds,
        float fade_in_seconds,
        std::function<void()> on_midpoint,
        std::function<void()> on_complete = nullptr
    );

    void Update(float delta_seconds);
    void Render(sf::RenderWindow& window) const;

    [[nodiscard]] bool IsActive() const { return state_ != State::Idle; }
    [[nodiscard]] float Alpha() const { return alpha_; }
    void Reset();

private:
    State state_ = State::Idle;
    float alpha_ = 0.0f;
    float fade_out_seconds_ = 0.18f;
    float fade_in_seconds_ = 0.18f;
    float timer_ = 0.0f;
    bool midpoint_fired_ = false;
    std::function<void()> on_midpoint_;
    std::function<void()> on_complete_;
};

template <typename SceneT, typename... Args>
void SceneManager::SwitchTo(Args&&... args) {
    Clear();
    Push<SceneT>(std::forward<Args>(args)...);
}

template <typename SceneT, typename... Args>
void SceneManager::Push(Args&&... args) {
    if (Top()) Top()->OnPause();
    auto scene = std::make_unique<SceneT>(std::forward<Args>(args)...);
    scene->OnEnter();
    scenes_.push_back(std::move(scene));
}

template <typename SceneT> void SceneManager::PopUntil() {
    while (Size() > 1) Pop();
}

template <typename SceneT>
SceneT* SceneManager::Find() const {
    for (auto it = scenes_.rbegin(); it != scenes_.rend(); ++it) {
        if (typeid(*((*it).get())) == typeid(SceneT)) {
            return static_cast<SceneT*>((*it).get());
        }
    }
    return nullptr;
}

}  // namespace CloudSeamanor::engine
