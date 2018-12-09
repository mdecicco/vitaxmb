#pragma once
#include <common/types.h>
#include <system/input.h>

namespace v {
    class Device;
    class GxmBuffer;
    class GxmShader;
    class ColorPicker : public InputReceiver {
        public:
            // 0...1 (0 == 0, 1 == scale)
            void ring_thickness (f32 thickness) { m_ringThickness = thickness; m_doRebuild = true; }
            void square_spacing (f32 spacing) { m_squareSpacing = spacing; m_doRebuild = true; }
            void set_color (const vec3& hsl) { m_hsl = hsl; m_doRecolor = true; }
            vec3 color () const { return m_hsl; }
            bool changed () const;
            
            void update (f32 dt);
            void render ();
            
            virtual void onLeftAnalog (const vec2& pos, const vec2& delta);
            virtual void onRightAnalog (const vec2& pos, const vec2& delta);
            
            vec2 position;
            bool hide;
            f32 opacity;
            f32 scale; // in pixels
            
        protected:
            friend class Xmb;
            ColorPicker (Device* device);
            ~ColorPicker ();
            
            u64 m_changedFrameId;
            f32 m_ringThickness;
            f32 m_squareSpacing;
            bool m_doRebuild;
            bool m_doRecolor;
            vec3 m_hsl;
            
            Device* m_device;
            GxmBuffer* m_vertices;
            GxmBuffer* m_indices;
            GxmShader* m_shader;
    };
};
