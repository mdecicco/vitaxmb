#include <rendering/xmb_sub_icon.h>

#include <system/device.h>
#include <system/texture.h>
#include <rendering/xmb.h>
#include <rendering/xmb_icon.h>
#include <rendering/xmb_column.h>
#include <rendering/xmb_option.h>

#include <math.h>
#include <unordered_map>
using namespace std;

#include <common/debugLog.h>
#define printf debugLog

namespace v {
    XmbSubIcon::XmbSubIcon (u8 level, u8 idx, GxmTexture* icon, f32 iconScale, const vec2& iconOffset,
                const string& text, const string& desc, GxmShader* shader, DeviceGpu* gpu,
                theme_data* theme, XmbSubIcon* parent, XmbCol* xmbCol) :
        m_idx(idx), m_shader(shader), m_text(text), active(false), m_rowIdx(0),
        m_level(level), expanded(false), m_theme(theme), m_description(desc),
        expandedChild(NULL), m_parent(parent), m_xmbCol(xmbCol), m_gpu(gpu),
        hide(false),
        positionX(level * theme->icon_spacing, 0.0f, interpolate::easeOutQuad),
        m_offsetY(0.0f, 0.0f, interpolate::easeOutQuad),
        opacity(idx == 0 ? 1.0f : 0.1f, 0.0f, interpolate::easeOutQuad),
        textOpacityMultiplier(1.0f, 0.0f, interpolate::easeOutQuad),
        subIconOpacity(1.0f, 0.0f, interpolate::easeOutQuad)
    {
        m_icon = new XmbIcon(icon, iconScale, iconOffset, gpu, shader, theme);
    }
    XmbSubIcon::~XmbSubIcon () {
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
            (m_theme->icon_offset.y + m_theme->icon_spacing) + (m_idx * m_theme->icon_spacing) + m_offsetY
        );
        m_icon->opacity = opacity;
        m_icon->update(dt);
        
        if(expanded) {
            for(u8 c = 0;c < items.size();c++) {
                items[c]->update(rootX, dt);
            }
        }
    }
    void XmbSubIcon::render () {
        if(hide) return;
        if(expanded) for(u8 c = 0;c < items.size();c++) items[c]->render();
        m_icon->render();
        if(m_theme->font && !expanded) {
            vec3 c = hsl(m_theme->font_color);
            vec2 pos = m_icon->position + m_theme->font_column_icon_offset;
            if(m_theme->show_text_alignment_point) m_gpu->draw_point(pos, 5, vec4(1,1,1,1));
            m_theme->font->print(
                pos,
                m_text.c_str(),
                vec4(c.x, c.y, c.z, opacity * (f32)textOpacityMultiplier),
                TEXT_ALIGN_X_LEFT_Y_CENTER
            );
        }
    }
    void XmbSubIcon::shift (i8 direction) {
        if(expandedChild) expandedChild->shift(direction);
        else {
            u8 lastRow = m_rowIdx;
            m_rowIdx += direction;
            if (m_rowIdx >= (i8)items.size()) m_rowIdx = items.size() - 1;
            else if (m_rowIdx < 0) m_rowIdx = 0;
            
            if(lastRow != m_rowIdx) {
                for(u8 i = 0;i < items.size();i++) {
                    XmbSubIcon* c = items[i];
                    if(i == m_rowIdx) {
                        c->active = true;
                        c->opacity = 1.0f;
                    } else {
                        (c->opacity = 0.1f).then([c]() mutable { c->active = false; });
                    }
                    c->offsetY(-m_theme->icon_spacing * m_rowIdx);
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
        this->expanded = true;
        
        
        // move this item's children to the center and increase their opacity
        for(u8 c = 0;c < items.size();c++) {
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
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = m_theme->icon_spacing;
            (item->opacity = 0.0f).then([item, this]() mutable {
                this->expanded = false;
            });
        }
    }
    void XmbSubIcon::childExpanded () {
        if(items.size() == 0) return;
        // store which child was expanded
        expandedChild = items[m_rowIdx];
        
        // move all items from this row to the left and decrease opacity
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = -m_theme->icon_spacing;
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
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = 0.0f;
            item->opacity = c == m_rowIdx ? 1.0f : 0.1f;
            item->textOpacityMultiplier = 1.0f;
        }
        
        // The parent's icons must be shown again to fill the gap
        if(m_parent) m_parent->showIcons();
        else m_xmbCol->showIcons();
    }
    void XmbSubIcon::hideIcons () {
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            (item->opacity = 0.0f).then([item]() mutable {
                item->hide = !item->active;
            });
        }
    }
    void XmbSubIcon::showIcons () {
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->opacity = c == m_rowIdx ? 0.5f : 0.1f;
            item->hide = false;
        }
    }
    void XmbSubIcon::onButtonDown(SceCtrlButtons btn) {
        if(!active) return;
        if(items.size() > 0) {
            if(expandedChild) expandedChild->onButtonDown(btn);
            else if(btn == SCE_CTRL_CIRCLE) contract();
            else if(btn == SCE_CTRL_CROSS) items[m_rowIdx]->expand();
        }
        else if(btn == SCE_CTRL_CIRCLE && m_parent) contract();
    }
};
