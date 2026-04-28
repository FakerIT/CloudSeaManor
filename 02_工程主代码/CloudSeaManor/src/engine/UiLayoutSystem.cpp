// ============================================================================
// 【UiLayoutSystem】UI 页面布局系统实现
// ============================================================================

#include "CloudSeamanor/UiLayoutSystem.hpp"

#include "CloudSeamanor/Logger.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace CloudSeamanor::engine {

// ============================================================================
// UiPage
// ============================================================================

UiPage::UiPage() = default;

UiPage::UiPage(const UiPageConfig& config) {
    LoadFromConfig(config);
}

void UiPage::LoadFromConfig(const UiPageConfig& config) {
    config_ = config;
    open_ = false;
    geometry_dirty_ = true;
}

void UiPage::AddElement(const UiElement& element) {
    elements_[element.id] = element;
    geometry_dirty_ = true;
}

void UiPage::RemoveElement(const std::string& element_id) {
    elements_.erase(element_id);
    geometry_dirty_ = true;
}

UiElement* UiPage::GetElement(const std::string& id) {
    auto it = elements_.find(id);
    return it != elements_.end() ? &it->second : nullptr;
}

const UiElement* UiPage::GetElement(const std::string& id) const {
    auto it = elements_.find(id);
    return it != elements_.end() ? &it->second : nullptr;
}

void UiPage::SetAssetManager(infrastructure::SpriteAssetManager* mgr) {
    asset_manager_ = mgr;
}

void UiPage::SetFontRenderer(PixelFontRenderer* renderer) {
    font_renderer_ = renderer;
}

void UiPage::SetCenter(const sf::Vector2f& center) {
    page_rect_.position = {center.x - config_.width * 0.5f,
                              center.y - config_.height * 0.5f};
    page_rect_.size = {static_cast<float>(config_.width),
                         static_cast<float>(config_.height)};
    geometry_dirty_ = true;
}

void UiPage::Open() {
    if (open_) return;
    open_ = true;
    anim_state_ = AnimationState::FadeIn;
    anim_elapsed_ = 0.0f;
    alpha_ = 0.0f;
}

void UiPage::Close() {
    if (!open_) return;
    anim_state_ = AnimationState::FadeOut;
    anim_elapsed_ = 0.0f;
}

void UiPage::RefreshBindings() {
    geometry_dirty_ = true;
}

void UiPage::ClearBindings() {
    data_bindings_.clear();
}

void UiPage::HandleMouseMove(float mx, float my) {
    // 转换到页面局部坐标
    float lx = mx - page_rect_.position.x;
    float ly = my - page_rect_.position.y;

    for (auto& [id, elem] : elements_) {
        bool was_hovered = elem.hovered;
        elem.hovered = elem.visible && elem.enabled &&
                        lx >= elem.x && lx <= elem.x + elem.width &&
                        ly >= elem.y && ly <= elem.y + elem.height;

        if (elem.hovered && !was_hovered) {
            HandleElementHover_(id);
        }
    }
}

void UiPage::HandleMouseClick(float mx, float my) {
    HandleMouseMove(mx, my);

    for (auto& [id, elem] : elements_) {
        if (elem.hovered && elem.interactive && elem.enabled) {
            HandleElementClick_(id);
            break;
        }
    }
}

bool UiPage::HandleKeyPressed(const sf::Event::KeyPressed& key) {
    if (key.code == sf::Keyboard::Escape && open_) {
        Close();
        return true;
    }

    // Tab 切换
    if (key.code == sf::Keyboard::Tab) {
        NavigateFocus("right");
        return true;
    }

    // 方向键焦点导航
    if (key.code == sf::Keyboard::Up) { NavigateFocus("up"); return true; }
    if (key.code == sf::Keyboard::Down) { NavigateFocus("down"); return true; }
    if (key.code == sf::Keyboard::Left) { NavigateFocus("left"); return true; }
    if (key.code == sf::Keyboard::Right) { NavigateFocus("right"); return true; }

    // Enter 确认
    if (key.code == sf::Keyboard::Enter) {
        if (!focused_element_id_.empty()) {
            HandleElementClick_(focused_element_id_);
            return true;
        }
    }

    return false;
}

void UiPage::SetFocusNavigation(const std::string& from_id,
                                const std::string& to_id,
                                const std::string& direction) {
    focus_navigation_[from_id][direction] = to_id;
}

void UiPage::NavigateFocus(const std::string& direction) {
    if (focused_element_id_.empty()) {
        if (!elements_.empty()) {
            focused_element_id_ = elements_.begin()->first;
        }
        return;
    }

    auto it = focus_navigation_.find(focused_element_id_);
    if (it != focus_navigation_.end()) {
        auto dir_it = it->second.find(direction);
        if (dir_it != it->second.end()) {
            focused_element_id_ = dir_it->second;
            return;
        }
    }

    // 默认行为：找最近的下一个可交互元素
    auto elem_it = elements_.find(focused_element_id_);
    if (elem_it != elements_.end()) {
        // 简单循环
        auto next_it = std::next(elem_it);
        if (next_it == elements_.end()) next_it = elements_.begin();
        for (auto it2 = next_it; it2 != elements_.end(); ++it2) {
            if (it2->second.interactive && it2->second.visible) {
                focused_element_id_ = it2->first;
                return;
            }
        }
    }
}

void UiPage::Update(float delta_seconds) {
    UpdateAnimation_(delta_seconds);

    if (geometry_dirty_) {
        ComputeLayout_();
        RebuildGeometry_();
        geometry_dirty_ = false;
    }
}

void UiPage::Render(sf::RenderWindow& window) {
    if (!open_ && alpha_ <= 0.0f) return;
    window.draw(*this);
}

sf::FloatRect UiPage::GetPageRect() const {
    return page_rect_;
}

void UiPage::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!open_ && alpha_ <= 0.0f) return;

    // 构建渲染状态
    sf::RenderStates rs = states;
    rs.transform.translate(page_rect_.position);

    // 应用透明度
    if (alpha_ < 1.0f) {
        rs = sf::RenderStates::Default;
    }

    // 绘制背景 + 边框
    if (!bg_vertices_.isEmpty()) {
        // 应用 alpha
        for (std::size_t i = 0; i < bg_vertices_.getVertexCount(); ++i) {
            sf::Color c = bg_vertices_[i].color;
            c.a = static_cast<std::uint8_t>(c.a * alpha_);
            bg_vertices_[i].color = c;
        }
        target.draw(bg_vertices_, rs);
    }

    if (!border_vertices_.isEmpty()) {
        for (std::size_t i = 0; i < border_vertices_.getVertexCount(); ++i) {
            sf::Color c = border_vertices_[i].color;
            c.a = static_cast<std::uint8_t>(c.a * alpha_);
            border_vertices_[i].color = c;
        }
        target.draw(border_vertices_, rs);
    }

    // 绘制子元素
    for (const auto& [id, elem] : elements_) {
        if (!elem.visible) continue;

        sf::RenderStates elem_rs = rs;
        elem_rs.transform.translate(elem.x, elem.y);

        // 绘制元素（根据类型）
        switch (elem.type) {
            case UiElementType::Image:
            case UiElementType::Image9Patch: {
                // 通过 asset_manager_ 加载精灵并绘制
                if (asset_manager_ && !elem.style.sprite_id.empty()) {
                    auto sprite_opt = asset_manager_->GetSprite(elem.style.sprite_id);
                    if (sprite_opt) {
                        sf::Sprite sp;
                        sp.setTexture(*asset_manager_->GetAtlasTexture(sprite_opt->atlas_id));
                        sp.setTextureRect(sprite_opt->region);
                        // 应用 alpha
                        auto c = sp.getColor();
                        c.a = static_cast<std::uint8_t>(elem.style.alpha * alpha_ * 255);
                        sp.setColor(c);
                        target.draw(sp, elem_rs);
                    }
                }
                break;
            }
            case UiElementType::Text: {
                if (font_renderer_ && !elem.text_content.empty()) {
                    TextStyle style = elem.style.text_style;
                    style.fill_color.a = static_cast<std::uint8_t>(elem.style.alpha * 255);
                    font_renderer_->DrawText(target, elem.text_content,
                                             sf::Vector2f(elem.padding_left, elem.padding_top),
                                             style);
                }
                break;
            }
            case UiElementType::ProgressBar: {
                // 进度条背景
                sf::RectangleShape bg({elem.width, elem.height});
                bg.setFillColor(sf::Color(200, 192, 168));
                bg.setOutlineThickness(1);
                bg.setOutlineColor(sf::Color(92, 58, 30));
                target.draw(bg, elem_rs);

                // 进度条填充
                float ratio = (elem.progress_max > 0)
                    ? elem.progress_value / elem.progress_max : 0.0f;
                float fill_w = (elem.width - 4) * ratio;
                if (fill_w > 0) {
                    sf::RectangleShape fill({fill_w, elem.height - 4});
                    fill.setPosition(2, 2);
                    fill.setFillColor(sf::Color(212, 168, 75)); // LightBrown
                    auto c = fill.getFillColor();
                    c.a = static_cast<std::uint8_t>(elem.style.alpha * 255);
                    fill.setFillColor(c);
                    target.draw(fill, elem_rs);
                }
                break;
            }
            case UiElementType::Button: {
                // 按钮背景（复用现有逻辑）
                sf::RectangleShape btn({elem.width, elem.height});
                btn.setFillColor(elem.hovered
                    ? sf::Color(255, 250, 205)  // HighlightYellow
                    : sf::Color(247, 239, 210)); // LightCream
                btn.setOutlineThickness(elem.hovered ? 2 : 1);
                btn.setOutlineColor(sf::Color(92, 58, 30));
                auto c = btn.getFillColor();
                c.a = static_cast<std::uint8_t>(elem.style.alpha * 255);
                btn.setFillColor(c);
                target.draw(btn, elem_rs);

                // 按钮文字
                if (font_renderer_ && !elem.text_content.empty()) {
                    TextStyle style = elem.style.text_style;
                    style.fill_color.a = static_cast<std::uint8_t>(elem.style.alpha * 255);
                    font_renderer_->DrawCenteredText(target, elem.text_content,
                                                     sf::Vector2f(elem.width * 0.5f,
                                                                    elem.height * 0.5f),
                                                     style);
                }
                break;
            }
            default:
                break;
        }

        // 焦点指示器（手柄）
        if (elem.id == focused_element_id_) {
            sf::RectangleShape focus_ring({elem.width + 4, elem.height + 4});
            focus_ring.setPosition(-2, -2);
            focus_ring.setFillColor(sf::Color::Transparent);
            focus_ring.setOutlineThickness(2);
            focus_ring.setOutlineColor(sf::Color(144, 238, 144, 200)); // ActiveGreen
            target.draw(focus_ring, elem_rs);
        }
    }
}

void UiPage::ComputeLayout_() {
    const sf::FloatRect page_local{
        {0.0f, 0.0f},
        {page_rect_.size.x, page_rect_.size.y}
    };

    float flow_cursor_x = 0.0f;
    float flow_cursor_y = 0.0f;
    float flow_line_max_h = 0.0f;
    int grid_index = 0;

    for (auto& [id, elem] : elements_) {
        switch (elem.layout) {
        case LayoutRule::Absolute:
            break;
        case LayoutRule::Anchor: {
            const float usable_w = std::max(0.0f, page_local.size.x - elem.width);
            const float usable_h = std::max(0.0f, page_local.size.y - elem.height);
            float x = 0.0f;
            float y = 0.0f;
            switch (elem.anchor) {
            case AnchorPoint::TopLeft: x = 0.0f; y = 0.0f; break;
            case AnchorPoint::TopCenter: x = usable_w * 0.5f; y = 0.0f; break;
            case AnchorPoint::TopRight: x = usable_w; y = 0.0f; break;
            case AnchorPoint::MiddleLeft: x = 0.0f; y = usable_h * 0.5f; break;
            case AnchorPoint::MiddleCenter: x = usable_w * 0.5f; y = usable_h * 0.5f; break;
            case AnchorPoint::MiddleRight: x = usable_w; y = usable_h * 0.5f; break;
            case AnchorPoint::BottomLeft: x = 0.0f; y = usable_h; break;
            case AnchorPoint::BottomCenter: x = usable_w * 0.5f; y = usable_h; break;
            case AnchorPoint::BottomRight: x = usable_w; y = usable_h; break;
            }
            elem.x = x + elem.anchor_offset_x + elem.margin_left - elem.margin_right;
            elem.y = y + elem.anchor_offset_y + elem.margin_top - elem.margin_bottom;
            break;
        }
        case LayoutRule::Flow: {
            const float next_w = elem.width + elem.margin_left + elem.margin_right;
            const float next_h = elem.height + elem.margin_top + elem.margin_bottom;
            const float next_x = flow_cursor_x + next_w;
            if (next_x > page_local.size.x && flow_cursor_x > 0.0f) {
                flow_cursor_x = 0.0f;
                flow_cursor_y += flow_line_max_h + std::max(0.0f, elem.spacing_y);
                flow_line_max_h = 0.0f;
            }

            elem.x = flow_cursor_x + elem.margin_left;
            elem.y = flow_cursor_y + elem.margin_top;
            flow_cursor_x += next_w + std::max(0.0f, elem.spacing_x);
            flow_line_max_h = std::max(flow_line_max_h, next_h);
            break;
        }
        case LayoutRule::Grid: {
            const int columns = std::max(1, elem.grid_columns);
            const int row = grid_index / columns;
            const int col = grid_index % columns;
            elem.x = col * (elem.width + elem.spacing_x) + elem.margin_left;
            elem.y = row * (elem.height + elem.spacing_y) + elem.margin_top;
            ++grid_index;
            break;
        }
        case LayoutRule::Stack:
            elem.x = elem.margin_left;
            elem.y = elem.margin_top;
            break;
        }
    }
}

void UiPage::UpdateAnimation_(float delta_seconds) {
    anim_elapsed_ += delta_seconds;

    switch (anim_state_) {
        case AnimationState::FadeIn: {
            float t = anim_elapsed_ / anim_duration_;
            alpha_ = std::min(1.0f, t);
            if (alpha_ >= 1.0f) {
                anim_state_ = AnimationState::Idle;
                open_ = true;
            }
            break;
        }
        case AnimationState::FadeOut: {
            float t = anim_elapsed_ / anim_duration_;
            alpha_ = 1.0f - std::min(1.0f, t);
            if (alpha_ <= 0.0f) {
                anim_state_ = AnimationState::Idle;
                open_ = false;
            }
            break;
        }
        case AnimationState::Idle:
            break;
    }
}

void UiPage::RebuildGeometry_() {
    // 背景
    bg_vertices_.clear();
    bg_vertices_.setPrimitiveType(sf::PrimitiveType::Quads);
    bg_vertices_.resize(4);

    bg_vertices_[0].position = {0.0f, 0.0f};
    bg_vertices_[1].position = {page_rect_.size.x, 0.0f};
    bg_vertices_[2].position = {page_rect_.size.x, page_rect_.size.y};
    bg_vertices_[3].position = {0.0f, page_rect_.size.y};
    for (int i = 0; i < 4; ++i) {
        bg_vertices_[i].color = sf::Color(251, 244, 224); // Cream
    }

    // 边框
    border_vertices_.clear();
    border_vertices_.setPrimitiveType(sf::PrimitiveType::Lines);

    // 8 个角块
    float cs = 8.0f; // corner block size
    auto addCorner = [&](float x, float y, int qx, int qy) {
        // 角块: 2 个三角形 = 6 顶点
        border_vertices_.append(sf::Vertex({x, y},
            sf::Color(92, 58, 30))); // BrownOutline
        border_vertices_.append(sf::Vertex({x + cs * qx, y},
            sf::Color(92, 58, 30)));
        border_vertices_.append(sf::Vertex({x, y + cs * qy},
            sf::Color(92, 58, 30)));
        border_vertices_.append(sf::Vertex({x + cs * qx, y},
            sf::Color(92, 58, 30)));
        border_vertices_.append(sf::Vertex({x + cs * qx, y + cs * qy},
            sf::Color(92, 58, 30)));
        border_vertices_.append(sf::Vertex({x, y + cs * qy},
            sf::Color(92, 58, 30)));
    };

    addCorner(0, 0, 1, 1);                           // 左上
    addCorner(page_rect_.size.x - cs, 0, -1, 1);    // 右上
    addCorner(0, page_rect_.size.y - cs, 1, -1);    // 左下
    addCorner(page_rect_.size.x - cs, page_rect_.size.y - cs, -1, -1); // 右下
}

void UiPage::HandleElementHover_(const std::string& element_id) {
    auto* elem = GetElement(element_id);
    if (elem && elem->on_hover) {
        elem->on_hover(element_id);
    }
}

void UiPage::HandleElementClick_(const std::string& element_id) {
    auto* elem = GetElement(element_id);
    if (elem && elem->on_click) {
        elem->on_click(element_id);
    }
}

// ============================================================================
// UiPageManager
// ============================================================================

UiPageManager::UiPageManager() = default;

void UiPageManager::SetAssetManager(infrastructure::SpriteAssetManager* mgr) {
    asset_manager_ = mgr;
    for (auto& [id, page] : pages_) {
        page->SetAssetManager(mgr);
    }
}

void UiPageManager::SetFontRenderer(PixelFontRenderer* renderer) {
    font_renderer_ = renderer;
    for (auto& [id, page] : pages_) {
        page->SetFontRenderer(renderer);
    }
}

UiPage* UiPageManager::RegisterPage(const UiPageConfig& page_config) {
    auto page = std::make_unique<UiPage>(page_config);
    page->SetAssetManager(asset_manager_);
    page->SetFontRenderer(font_renderer_);
    UiPage* ptr = page.get();
    pages_[page_config.page_id] = std::move(page);
    return ptr;
}

UiPage* UiPageManager::RegisterPage(const std::string& page_id, std::unique_ptr<UiPage> page) {
    page->SetAssetManager(asset_manager_);
    page->SetFontRenderer(font_renderer_);
    UiPage* ptr = page.get();
    pages_[page_id] = std::move(page);
    return ptr;
}

int UiPageManager::LoadPagesFromDirectory(const std::string& pages_dir) {
    int count = 0;
    auto configs = UiLayoutSerializer::ScanPagesFromDirectory(pages_dir);
    for (auto& cfg : configs) {
        RegisterPage(cfg);
        count++;
    }
    return count;
}

void UiPageManager::OpenPage(const std::string& page_id) {
    auto it = pages_.find(page_id);
    if (it == pages_.end()) return;

    if (it->second->IsOpen()) return;

    // 关闭其他模态页
    if (it->second->config_.modal) {
        CloseNonModalPages();
    }

    it->second->Open();
}

void UiPageManager::ClosePage(const std::string& page_id) {
    auto it = pages_.find(page_id);
    if (it == pages_.end()) return;
    it->second->Close();
}

void UiPageManager::TogglePage(const std::string& page_id) {
    auto it = pages_.find(page_id);
    if (it == pages_.end()) return;
    if (it->second->IsOpen()) {
        it->second->Close();
    } else {
        it->second->Open();
    }
}

void UiPageManager::CloseAllPages() {
    for (auto& [id, page] : pages_) {
        if (page->IsOpen()) {
            page->Close();
        }
    }
}

void UiPageManager::CloseNonModalPages() {
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() && !page->config_.modal) {
            page->Close();
        }
    }
}

UiPage* UiPageManager::GetPage(const std::string& page_id) {
    auto it = pages_.find(page_id);
    return it != pages_.end() ? it->second.get() : nullptr;
}

const UiPage* UiPageManager::GetPage(const std::string& page_id) const {
    auto it = pages_.find(page_id);
    return it != pages_.end() ? it->second.get() : nullptr;
}

bool UiPageManager::IsPageOpen(const std::string& page_id) const {
    auto it = pages_.find(page_id);
    return it != pages_.end() && it->second->IsOpen();
}

std::vector<std::string> UiPageManager::GetOpenPageIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, page] : pages_) {
        if (page->IsOpen()) ids.push_back(id);
    }
    return ids;
}

bool UiPageManager::HasOpenPage() const {
    for (const auto& [id, page] : pages_) {
        if (page->IsOpen()) return true;
    }
    return false;
}

void UiPageManager::Update(float delta_seconds) {
    // 处理待关闭队列
    for (const auto& id : pending_close_) {
        auto it = pages_.find(id);
        if (it != pages_.end() && !it->second->IsOpen()) {
            // 已完全关闭
        }
    }
    pending_close_.clear();

    // 更新所有页面动画
    for (auto& [id, page] : pages_) {
        page->Update(delta_seconds);
    }
}

void UiPageManager::Render(sf::RenderWindow& window) {
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() || page->IsAnimating()) {
            page->Render(window);
        }
    }
}

void UiPageManager::HandleMouseMove(float mx, float my) {
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() && !page->config_.modal) {
            page->HandleMouseMove(mx, my);
        }
    }
    // 模态页最后处理
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() && page->config_.modal) {
            page->HandleMouseMove(mx, my);
        }
    }
}

void UiPageManager::HandleMouseClick(float mx, float my) {
    // 模态页优先
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() && page->config_.modal) {
            page->HandleMouseClick(mx, my);
        }
    }
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() && !page->config_.modal) {
            page->HandleMouseClick(mx, my);
        }
    }
}

bool UiPageManager::HandleKeyPressed(const sf::Event::KeyPressed& key) {
    if (key.code == sf::Keyboard::Escape) {
        // Esc: 优先关闭模态页，然后非模态
        for (auto& [id, page] : pages_) {
            if (page->IsOpen() && page->config_.modal) {
                page->Close();
                return true;
            }
        }
        for (auto& [id, page] : pages_) {
            if (page->IsOpen() && !page->config_.modal) {
                page->Close();
                return true;
            }
        }
    }

    // 其他按键：只处理打开的页面
    for (auto& [id, page] : pages_) {
        if (page->IsOpen() && page->HandleKeyPressed(key)) {
            return true;
        }
    }
    return false;
}

std::vector<UiPage*> UiPageManager::GetAllPages() {
    std::vector<UiPage*> result;
    for (auto& [id, page] : pages_) {
        result.push_back(page.get());
    }
    return result;
}

// ============================================================================
// UiLayoutSerializer
// ============================================================================

std::string UiLayoutSerializer::SerializePage(const UiPageConfig& config) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"page_id\": \"" << config.page_id << "\",\n";
    out << "  \"title\": \"" << config.title << "\",\n";
    out << "  \"width\": " << config.width << ",\n";
    out << "  \"height\": " << config.height << ",\n";
    out << "  \"modal\": " << (config.modal ? "true" : "false") << ",\n";
    out << "  \"pause_game\": " << (config.pause_game ? "true" : "false") << ",\n";
    out << "  \"elements\": [\n";
    for (std::size_t i = 0; i < config.root_element_ids.size(); ++i) {
        const bool is_last = (i + 1 == config.root_element_ids.size());
        out << "    {\n";
        out << "      \"id\": \"" << config.root_element_ids[i] << "\",\n";
        out << "      \"x\": 0,\n";
        out << "      \"y\": 0,\n";
        out << "      \"width\": 48,\n";
        out << "      \"height\": 48,\n";
        out << "      \"text\": \"\"\n";
        out << "    }" << (is_last ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return out.str();
}

bool UiLayoutSerializer::WritePageToFile(const std::string& path,
                                         const UiPageConfig& config) {
    const auto json = SerializePage(config);
    std::ofstream file(path, std::ios::trunc);
    if (!file.is_open()) {
        Logger::Warning("UiLayoutSerializer: 无法写入页面文件: " + path);
        return false;
    }
    file << json;
    return file.good();
}

std::optional<UiPageConfig> UiLayoutSerializer::ParsePage(const std::string& json) {
    auto jv = JsonValue::Parse(json);
    if (!jv || !jv->IsObject()) return std::nullopt;

    const auto& obj = jv->AsObject();
    UiPageConfig config;
    config.page_id = obj.GetString("page_id", "");
    config.title = obj.GetString("title", "");
    config.width = obj.GetInt("width", 640);
    config.height = obj.GetInt("height", 480);
    config.modal = obj.GetBool("modal", false);
    config.pause_game = obj.GetBool("pause_game", false);

    if (auto* elems = obj.GetArray("elements")) {
        for (std::size_t i = 0; i < elems->Size(); ++i) {
            const auto& elem_obj = (*elems)[i]->AsObject();
            UiElement elem;
            elem.id = elem_obj.GetString("id", "");
            elem.x = static_cast<float>(elem_obj.GetInt("x", 0));
            elem.y = static_cast<float>(elem_obj.GetInt("y", 0));
            elem.width = static_cast<float>(elem_obj.GetInt("width", 48));
            elem.height = static_cast<float>(elem_obj.GetInt("height", 48));
            elem.text_content = elem_obj.GetString("text", "");
            config.root_element_ids.push_back(elem.id);
        }
    }

    return config;
}

std::optional<UiPageConfig> UiLayoutSerializer::LoadPageFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    std::stringstream ss;
    ss << file.rdbuf();
    return ParsePage(ss.str());
}

std::vector<UiPageConfig> UiLayoutSerializer::ScanPagesFromDirectory(const std::string& dir) {
    std::vector<UiPageConfig> configs;
    namespace fs = std::filesystem;
    if (!fs::exists(dir)) return configs;

    for (const auto& entry : fs::directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        auto path = entry.path().string();
        if (path.ends_with(".ui.json")) {
            if (auto cfg = LoadPageFromFile(path)) {
                configs.push_back(*cfg);
            }
        }
    }
    return configs;
}

UiPageConfig UiLayoutSerializer::CreateEmptyPage(const std::string& page_id) {
    UiPageConfig cfg;
    cfg.page_id = page_id;
    cfg.title = "未命名";
    cfg.width = 640;
    cfg.height = 480;
    return cfg;
}

std::string UiLayoutSerializer::ExportPageAsJson(const UiPage* page) {
    if (page == nullptr) {
        return "{}";
    }
    UiPageConfig cfg = CreateEmptyPage("exported_page");
    const auto rect = page->GetPageRect();
    cfg.width = static_cast<int>(rect.size.x);
    cfg.height = static_cast<int>(rect.size.y);
    return SerializePage(cfg);
}

// ============================================================================
// 预置页面模板实现
// ============================================================================

#define MAKE_PAGE(category, w, h) \
    UiPageConfig cfg; \
    cfg.page_id = category; \
    cfg.title = ""; \
    cfg.width = w; \
    cfg.height = h; \
    cfg.modal = true; \
    cfg.pause_game = true; \
    return cfg;

UiPageConfig PresetPageFactory::MakeInventoryPage() { MAKE_PAGE("inventory", 640, 480); }
UiPageConfig PresetPageFactory::MakeWorkshopPage() { MAKE_PAGE("workshop", 640, 480); }
UiPageConfig PresetPageFactory::MakeContractPage() { MAKE_PAGE("contract", 720, 560); }
UiPageConfig PresetPageFactory::MakeNpcDetailPage() { MAKE_PAGE("npc_detail", 560, 480); }
UiPageConfig PresetPageFactory::MakeSpiritBeastPage() { MAKE_PAGE("spirit_beast", 640, 520); }
UiPageConfig PresetPageFactory::MakeSaveLoadPage() { MAKE_PAGE("save_load", 600, 480); }
UiPageConfig PresetPageFactory::MakeSettingsPage() { MAKE_PAGE("settings", 480, 400); }
UiPageConfig PresetPageFactory::MakeCloudForecastPage() { MAKE_PAGE("cloud_forecast", 560, 480); }
UiPageConfig PresetPageFactory::MakeMainMenuPage() { MAKE_PAGE("main_menu", 560, 360); }
UiPageConfig PresetPageFactory::MakePlayerStatusPage() { MAKE_PAGE("player_status", 480, 520); }
UiPageConfig PresetPageFactory::MakeTeaGardenPage() { MAKE_PAGE("tea_garden", 560, 420); }
UiPageConfig PresetPageFactory::MakeFestivalPage() { MAKE_PAGE("festival", 560, 480); }
UiPageConfig PresetPageFactory::MakeSpiritRealmPage() { MAKE_PAGE("spirit_realm", 600, 480); }
UiPageConfig PresetPageFactory::MakeBuildingPage() { MAKE_PAGE("building", 640, 520); }

}  // namespace CloudSeamanor::engine
