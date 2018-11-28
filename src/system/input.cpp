#include <system/input.h>
#include <common/debugLog.h>
#include <string.h>
#define printf debugLog

namespace v {
    static SceCtrlButtons button_codes[24] = {
        SCE_CTRL_SELECT      , //!< Select button.
        SCE_CTRL_L3          , //!< L3 button.
        SCE_CTRL_R3          , //!< R3 button.
        SCE_CTRL_START       , //!< Start button.
        SCE_CTRL_UP          , //!< Up D-Pad button.
        SCE_CTRL_RIGHT       , //!< Right D-Pad button.
        SCE_CTRL_DOWN        , //!< Down D-Pad button.
        SCE_CTRL_LEFT        , //!< Left D-Pad button.
        SCE_CTRL_LTRIGGER    , //!< Left trigger.
        SCE_CTRL_L2          , //!< L2 button.
        SCE_CTRL_RTRIGGER    , //!< Right trigger.
        SCE_CTRL_R2          , //!< R2 button.
        SCE_CTRL_L1          , //!< L1 button.
        SCE_CTRL_R1          , //!< R1 button.
        SCE_CTRL_TRIANGLE    , //!< Triangle button.
        SCE_CTRL_CIRCLE      , //!< Circle button.
        SCE_CTRL_CROSS       , //!< Cross button.
        SCE_CTRL_SQUARE      , //!< Square button.
        SCE_CTRL_INTERCEPTED , //!< Input not available because intercepted by another application
        SCE_CTRL_PSBUTTON    , //!< Playstation (Home) button.
        SCE_CTRL_HEADPHONE   , //!< Headphone plugged in.
        SCE_CTRL_VOLUP       , //!< Volume up button.
        SCE_CTRL_VOLDOWN     , //!< Volume down button.
        SCE_CTRL_POWER         //!< Power button.
    };
    DeviceInput::DeviceInput () {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
        scan();
        memset(&m_ctrl, 0, sizeof(SceCtrlData));
        m_analogDeadzone = vec2(0.05f, 0.05f);
        m_ls = m_rs = vec2(0, 0);
    }
    DeviceInput::~DeviceInput () {
    }
    
    int DeviceInput::scan() {
        memcpy(&m_oldCtrl, &m_ctrl, sizeof(SceCtrlData));
        i32 ret = sceCtrlPeekBufferPositive(0, &m_ctrl, 1);
        bool down[24];
        bool up[24];
        bool held[24];
        bool nothing = true;
        for(u8 i = 0;i < 24;i++) {
            bool downNow = m_ctrl.buttons & button_codes[i];
            bool wasDown = m_oldCtrl.buttons & button_codes[i];
            
            down[i] = downNow && !wasDown;
            up[i] = !downNow && wasDown;
            held[i] = downNow && wasDown;
            if(nothing) nothing = down[i] || up[i] || held[i];
        }
        
        if(!nothing) {
            for(u8 i = 0;i < 24;i++) {
                if(down[i] || up[i] || held[i]) {
                    for(auto receiver = m_receivers.begin();receiver != m_receivers.end();receiver++) {
                        InputReceiver* rec = *receiver;
                        if(down[i]) rec->onButtonDown(button_codes[i]);
                        if(up[i]) rec->onButtonUp(button_codes[i]);
                        if(held[i]) rec->onButtonHeld(button_codes[i]);
                    }
                }
            }
        }
        
        vec2 ls = vec2(m_ctrl.lx, m_ctrl.ly);
        vec2 rs = vec2(m_ctrl.rx, m_ctrl.ry);
        vec2 old_ls = vec2(m_ctrl.lx, m_ctrl.ly);
        vec2 old_rs = vec2(m_ctrl.rx, m_ctrl.ry);
        
        // get lengths
        f32 ls_m = glm::length(ls);
        f32 rs_m = glm::length(rs);
        f32 old_ls_m = glm::length(old_ls);
        f32 old_rs_m = glm::length(old_rs);
        
        // normalize
        ls *= (1.0f / ls_m);
        rs *= (1.0f / rs_m);
        old_ls *= (1.0f / old_ls_m);
        old_rs *= (1.0f / old_rs_m);
        
        if (button(SCE_CTRL_CIRCLE)) {
            printf("%f\n", ls_m);
        }
        
        // clamp
        //ls *= glm::min(0, glm::max())
        m_ls = ls;
        m_rs = rs;
        
        return ret;
    }
    
    bool DeviceInput::button (unsigned int button) const {
        return m_ctrl.buttons & button;
    }
};
