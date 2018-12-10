#include <rendering/xmb_column.h>
#include <system/device.h>
#include <rendering/xmb.h>
#include <rendering/xmb_icon.h>
#include <rendering/xmb_sub_icon.h>

#include <math.h>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include <common/debugLog.h>
#define printf debugLog

namespace v {
    XmbCol::XmbCol (u8 idx, GxmTexture* icon, f32 iconScale, const vec2& iconOffset,
        const string& text, GxmShader* shader, DeviceGpu* gpu, theme_data* theme,
        Xmb* xmb) :
        m_idx(idx), m_shader(shader), m_text(text), active(false), m_rowIdx(0),
        m_gpu(gpu), m_theme(theme), m_offsetX(0.0f), hide(false), m_xmb(xmb),
        expandedChild(NULL),
        subIconOpacity(1.0f, 0.0f, interpolate::easeOutCubic),
        opacity(1.0f, 0.0f, interpolate::easeOutCubic)
    {
        m_icon = new XmbIcon(icon, iconScale, iconOffset, gpu, shader, theme);
    }
    XmbCol::~XmbCol () {
        delete m_icon;
        for(auto i = items.begin();i != items.end();i++) delete (*i);
    }
    void XmbCol::offsetX(f32 offset) {
        m_offsetX = offset;
    }
    void XmbCol::update (f32 dt) {
        opacity.duration(m_theme->slide_animation_duration);
        subIconOpacity.duration(m_theme->slide_animation_duration);
        m_icon->position = vec2(
            m_theme->icon_offset.x + (m_theme->icon_spacing.x * m_idx) + m_offsetX,
            m_theme->icon_offset.y
        );
        m_icon->opacity = opacity;
        m_icon->update(dt);
        
        if(active) {
            for(u8 c = 0;c < items.size();c++) {
                items[c]->update(m_icon->position.x, dt);
            }
        }
    }
    void XmbCol::render () {
        if(hide) return;
        m_icon->render();
        if(m_theme->font) {
            vec3 c = hsl(m_theme->font_color);
            vec2 textpos = m_icon->position + m_theme->text_horizontal_icon_offset;
            if(m_theme->show_text_alignment_point) m_gpu->draw_point(textpos, 5, vec4(1,1,1,1));
            m_theme->font->print(textpos, m_text.c_str(), vec4(c.x, c.y, c.z, opacity), TEXT_ALIGN_X_CENTER_Y_CENTER);
        }
        if(active) for(u8 c = 0;c < items.size();c++) items[c]->render();
    }
    void XmbCol::shift (i8 direction) {
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
                    c->offsetY(-m_theme->icon_spacing.y * m_rowIdx);
                }
            }
        }
    }
    void XmbCol::childExpanded () {
        if(items[m_rowIdx]->items.size() == 0) return;
        
        // store which child was expanded
        expandedChild = items[m_rowIdx];
        
        // move this column's children to the left and decrease opacity
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = -m_theme->icon_spacing.x;
            item->opacity = c == m_rowIdx ? 0.5f : 0.1f;
            item->textOpacityMultiplier = c == m_rowIdx ? 1.0f : 0.0f;
        }
    }
    void XmbCol::childContracted () {
        if(items[m_rowIdx]->items.size() == 0) return;
        
        // clear the stored record of the expanded child
        expandedChild = NULL;
        
        // move all items from this row to the center and increase opacity
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->positionX = 0.0f;
            item->opacity = c == m_rowIdx ? 1.0f : 0.1f;
            item->textOpacityMultiplier = 1.0f;
        }
    }
    void XmbCol::hideIcons (bool evenTheActiveOne) {
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            (item->opacity = 0.0f).then([item, evenTheActiveOne]() mutable {
                item->hide = !item->active || evenTheActiveOne;
            });
        }
    }
    void XmbCol::showIcons () {
        for(u8 c = 0;c < items.size();c++) {
            XmbSubIcon* item = items[c];
            item->opacity = c == m_rowIdx ? 1.0f : 0.1f;
            item->hide = false;
        }
    }
    void XmbCol::onButtonDown(SceCtrlButtons btn) {
        if(!active) return;
        if(expandedChild) expandedChild->onButtonDown(btn);
        else if(btn == SCE_CTRL_CROSS) items[m_rowIdx]->expand();
    }
};
