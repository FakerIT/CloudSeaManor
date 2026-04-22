#include <SFML/Graphics.hpp>

// 这是最早的 C++ 原型入口文件。
// 目的非常单纯：只验证三件事——窗口能打开、主循环能跑、角色能移动。
// 这样做的好处是可以在正式工程搭建前，先确认开发环境、依赖和基础输入逻辑没有问题。
int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Yunhai MVP - First Code");
    window.setFramerateLimit(60);

    // 先用矩形代替玩家精灵。
    // 这样做的目的是把“角色移动”这个核心问题和“美术资源接入”解耦，
    // 方便先验证控制逻辑本身是否正常。
    sf::RectangleShape player({36.0f, 36.0f});
    player.setFillColor(sf::Color(90, 200, 255));
    player.setPosition(640.0f, 360.0f);

    // 使用“像素/秒”作为移动速度单位。
    // 这样搭配 delta time 后，可以让不同机器上的移动表现更一致。
    const float moveSpeed = 240.0f;
    sf::Clock deltaClock;

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            // 处理关闭事件，保证窗口可以正常退出。
            if (event.type == sf::Event::Closed) {
                window.close();
            }
        }

        // 计算这一帧经过的时间。
        // 目的在于让移动速度与帧率解耦，避免高帧率机器移动过快。
        const float dt = deltaClock.restart().asSeconds();
        sf::Vector2f movement(0.0f, 0.0f);

        // 读取 WASD 输入，并把速度换算为本帧实际位移。
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) movement.y -= moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) movement.y += moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) movement.x -= moveSpeed * dt;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) movement.x += moveSpeed * dt;

        // 把输入得到的位移应用到玩家对象上。
        player.move(movement);

        // 每帧清屏、绘制、显示。
        // 这是最基础的渲染循环结构，后续所有游戏系统都会建立在这个节奏之上。
        window.clear(sf::Color(24, 28, 36));
        window.draw(player);
        window.display();
    }

    return 0;
}
