#ifndef UI_CORE_WIDGET_HPP
#define UI_CORE_WIDGET_HPP

#include <string>
#include <imgui.h>

namespace c2l::ui::core {

class Widget 
{
public:
    explicit Widget(std::string name) 
    	: m_name(std::move(name)) {}
    virtual ~Widget() = default;

    virtual void render() = 0;
    virtual void update([[maybe_unused]] double dt) {}
    
    [[nodiscard]] const std::string& name() const noexcept { return m_name; }
    [[nodiscard]] bool is_visible() const noexcept { return m_visible; }
    void set_visible(bool visible) noexcept { m_visible = visible; }
    
    void set_position(const ImVec2& pos) { m_position = pos; }
    void set_size(const ImVec2& size) { m_size = size; }

protected:
    std::string m_name;
    bool m_visible		{true};
    ImVec2 m_position	{0, 0};
    ImVec2 m_size		{0, 0};
};

} // namespace c2l::ui::core

#endif // UI_CORE_WIDGET_HPP