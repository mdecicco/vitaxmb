#include <rendering/xmb_sub_icon.h>

#include <system/device.h>
#include <system/texture.h>
#include <rendering/xmb.h>
#include <rendering/xmb_icon.h>
#include <rendering/xmb_column.h>
#include <rendering/xmb_option.h>
#include <rendering/xmb_options_pane.h>
#include <rendering/color_picker.h>

#include <math.h>
#include <unordered_map>
using namespace std;

#include <common/debugLog.h>
#define printf debugLog

namespace v {
    XmbSubIcon::XmbSubIcon (u8 level, u16 idx, const string& setting, GxmTexture* icon,
                f32 iconScale, const vec2& iconOffset, const std::string& text,
                const std::string& desc, GxmShader* shader, DeviceGpu* gpu,
                theme_data* theme, XmbSubIcon* parent, XmbCol* xmbCol, Xmb* xmb) :
        m_level(level), m_idx(idx), m_shader(shader), m_text(text), active(false),
        m_rowIdx(0), expanded(false), m_theme(theme), m_description(desc),
        expandedChild(NULL), m_parent(parent), m_xmbCol(xmbCol), m_gpu(gpu), hide(false),
        m_xmb(xmb), showing_options(false), m_optionsIdx(0), m_setting(setting),
        positionX(level * theme->icon_spacing.x, 0.0f, interpolate::easeOutQuad),
        m_offsetY(0.0f, 0.0f, interpolate::easeOutQuad),
        opacity(idx == 0 ? 1.0f : 0.1f, 0.0f, interpolate::easeOutQuad),
        textOpacityMultiplier(1.0f, 0.0f, interpolate::easeOutQuad),
        subIconOpacity(1.0f, 0.0f, interpolate::easeOutQuad)
    {
        m_icon = new XmbIcon(icon, iconScale, iconOffset, gpu, shader, theme);
    }
    XmbSubIcon::~XmbSubIcon () {
        delete m_icon;
        for(auto i = items.begin();i != items.end();i++) delete (*i);
        for(auto i = options.begin();i != options.end();i++) delete (*i);
    }
    void XmbSubIcon::offsetY(f32 offset) {
        m_offsetY = offset;
    }
    void XmbSubIcon::update (f32 rootX, f32 dt) {
        positionX.duration(m_theme->slide_animation_duration);
        m_offsetY.duration(m_theme->slide_animation_duration);
        opacity.duration(m_theme->slide_animation_duration);
        textOpacityMultiplier.duration(m_theme->slide_animation_duration);
        subIconOpacity.duration(m_theme->slide_animation_duration);
        
        m_icon->position = vec2(
            rootX + positionX,
            (m_theme->icon_offset.y + m_theme->icon_spacing.y) + (m_idx * m_theme->icon_spacing.y) + m_offsetY
        );
        m_icon->opacity = opacity;
        m_icon->update(dt);
        
        if(expanded) for(u16 c = 0;c < items.size();c++) items[c]->update(rootX, dt);
        else if(showing_options) {
            for(u16 c = 0;c < options.size();c++) options[c]->update(dt);
        }
    }
    void XmbSubIcon::render () {
        if(hide) return;
        if(expanded) for(u16 c = 0;c < items.size();c++) items[c]->render();
        
        m_icon->render();
        if(m_theme->font && !expanded) {
            vec3 c = hsl(m_theme->font_color);
            vec2 pos = m_icon->position + m_theme->text_vertical_icon_offset;
            if(m_theme->show_text_alignment_point) m_gpu->draw_point(pos, 5, vec4(1,1,1,1));
            m_theme->font->print(
                pos,
                m_text.c_str(),
                vec4(c.x, c.y, c.z, opacity * (f32)textOpacityMultiplier),
                TEXT_ALIGN_X_LEFT_Y_CENTER
            );
        }
        
        if(m_theme->font && options.size() > 0 && items.size() == 0) {
            XmbOption* option = options[m_optionsIdx];
            vec3 c = hsl(m_theme->font_color);
            vec2 pos = m_icon->position + m_theme->text_option_icon_offset;
            if(m_theme->show_text_alignment_point) m_gpu->draw_point(pos, 5, vec4(1,1,1,1));
            m_theme->font->print(
                pos,
                option->value_str().c_str(),
                vec4(c.x, c.y, c.z, opacity * (f32)textOpacityMultiplier * 0.75f),
                TEXT_ALIGN_X_LEFT_Y_CENTER
            );
        }
    }
    void XmbSubIcon::shift (i8 direction) {
        if(showing_options) {
            u8 lastOption = m_optionsIdx;
            m_optionsIdx += direction;
            if(m_optionsIdx < 0) m_optionsIdx = 0;
            if(m_optionsIdx >= (i8)options.size()) m_optionsIdx = options.size() - 1;
            if(!(options.size() == 1 && options[0]->type() == OPTION_TYPE_COLOR)) {
                if(lastOption != m_optionsIdx) {
                    for(u8 i = 0;i < options.size();i++) {
                        XmbOption* o = options[i];
                        if(i == m_optionsIdx) o->opacity = 1.0f;
                        else o->opacity = 0.5f;
                        o->offsetY = -(m_optionsIdx * 30.0f);
                    }
                    
                    m_xmb->setting_changed(m_setting, options[m_optionsIdx]->value(), options[lastOption]->value());
                }
            }
        }
        else if(expandedChild) expandedChild->shift(direction);
        else {
            u16 lastRow = m_rowIdx;
            m_rowIdx += direction;
            if (m_rowIdx >= (u16)items.size()) m_rowIdx = items.size() - 1;
            else if (m_rowIdx < 0) m_rowIdx = 0;
            
            if(lastRow != m_rowIdx) {
                for(u16 i = 0;i < items.size();i++) {
                    XmbSubIcon* c = items[i];
                    if(i == m_rowIdx) {
                        c->active = true;
                        c->opacity = 1.0f;
                    } else {
                        (c->opacity = 0.1f).then([c]() mutable { c->active = false; });
                    }
                    c->offsetY(-m_theme->icon_spacing.y * m_rowIdx);
                }
            }
        }
    }
    void XmbSubIcon::expand () {
        if(items.size() == 0) return;
        if(m_parent) m_parent->childExpanded();
        else m_xmbCol->childExpanded();
        
        // mark this as expanded so that the children are rendered immediately
        // (so they can fade in from 0)
        expanded = true;
        
        // move this item's children to the center and increase their opacity
        for(u16 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = 0.0f;
            item->opacity = c == m_rowIdx ? 1.0f : 0.1f;
            item->active = c == m_rowIdx;
        }
    }
    void XmbSubIcon::contract () {
        if(items.size() == 0) return;
        if(m_parent) m_parent->childContracted();
        else m_xmbCol->childContracted();
        
        // move this item's children to the right and decrease their opacity
        // also make this as not expanded once the animation is done
        // (so that they keep rendering until 0 opacity)
        for(u16 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = m_theme->icon_spacing.x;
            item->opacity = 0.0f;
            if(c == items.size() - 1) {
                item->opacity.then([item, this]() mutable {
                    this->expanded = false;
                });
            }
        }
    }
    void XmbSubIcon::childExpanded () {
        if(items.size() == 0) return;
        // store which child was expanded
        expandedChild = items[m_rowIdx];
        
        // move all items from this row to the left and decrease opacity
        for(u16 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = -m_theme->icon_spacing.x;
            item->opacity = c == m_rowIdx ? 0.5f : 0.1f;
            item->textOpacityMultiplier = c == m_rowIdx ? 1.0f : 0.0f;
        }
        
        // The parent's icons must be hidden to avoid overlapping
        if(m_parent) m_parent->hideIcons();
        else m_xmbCol->hideIcons();
    }
    void XmbSubIcon::childContracted () {
        // clear the stored record of the expanded child
        expandedChild = NULL;
        
        // move all items from this row to the center and increase opacity
        for(u16 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = 0.0f;
            item->opacity = c == m_rowIdx ? 1.0f : 0.1f;
            item->textOpacityMultiplier = 1.0f;
        }
        
        // The parent's icons must be shown again to fill the gap
        if(m_parent) m_parent->showIcons();
        else m_xmbCol->showIcons();
    }
    void XmbSubIcon::showOptions () {
        if(this->m_parent) this->m_parent->expandedChild = this;
        else if(this->m_xmbCol) this->m_xmbCol->expandedChild = this;
        XmbOptionsPane* pane = m_xmb->options_pane();
        pane->offsetX = -300.0f;
        pane->opacity = 1.0f;
        pane->renderCallback = [this, pane]() {
            f32 offsetX = pane->offsetX;
            for(u8 i = 0;i < this->options.size();i++) this->options[i]->render(offsetX);
        };
        pane->hide = false;
        showing_options = true;
        for(u8 c = 0;c < options.size();c++) {
            options[c]->became_visible();
            options[c]->opacity = m_optionsIdx == c ? 1.0f : 0.5f;
        }
    }
    void XmbSubIcon::hideOptions () {
        XmbOptionsPane* pane = m_xmb->options_pane();
        pane->offsetX = 0.0f;
        (pane->opacity = 0.0f).then([this, pane]() mutable {
            this->showing_options = false;
            
            if(this->m_parent) this->m_parent->expandedChild = NULL;
            else if(this->m_xmbCol) this->m_xmbCol->expandedChild = NULL;
            
            pane->renderCallback = NULL;
            pane->hide = true;
            
            ColorPicker* input = this->m_xmb->color_input();
            input->hide = true;
            
            for(u8 c = 0;c < this->options.size();c++) this->options[c]->became_hidden();
        });
        for(u8 c = 0;c < options.size();c++) {
            options[c]->opacity = 0.0f;
        }
    }
    void XmbSubIcon::hideIcons () {
        for(u16 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            (item->opacity = 0.0f).then([item]() mutable {
                item->hide = !item->active;
            });
        }
    }
    void XmbSubIcon::showIcons () {
        for(u16 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->opacity = c == m_rowIdx ? 0.5f : 0.1f;
            item->hide = false;
        }
    }
    void XmbSubIcon::onButtonDown(SceCtrlButtons btn) {
        if(!active) return;
        if(items.size() > 0) {
            //If the selected child item has options
            XmbSubIcon* selected = items[m_rowIdx];
            if(selected->options.size() > 0) {
                SceCtrlButtons trigger = selected->items.size() == 0 ? SCE_CTRL_CROSS : SCE_CTRL_TRIANGLE;
                if(!selected->showing_options && btn == trigger) {
                    selected->showOptions();
                    return;
                }
            }
            
            if(selected->showing_options && selected->options.size() > 0) return;
            
            if(expandedChild) expandedChild->onButtonDown(btn);
            else if(btn == SCE_CTRL_CIRCLE) contract();
            else if(btn == SCE_CTRL_CROSS) selected->expand();
        } else if(btn == SCE_CTRL_CIRCLE && m_parent) contract();
    }
};
