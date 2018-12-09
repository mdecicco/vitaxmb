#pragma once
#include <psp2/ctrl.h>
#include <common/types.h>
#include <vector>
using namespace std;

namespace v {
    class DeviceInput;
    class InputReceiver {
        public:
            InputReceiver ();
            virtual ~InputReceiver ();
            
            void stealInput ();
            void releaseInput (bool enable);
            void disableInput ();
            void enableInput ();
            void setAnalogDeadzones(f32 left, f32 right);
            
            virtual void onButtonDown(SceCtrlButtons btn) { }
            virtual void onButtonHeld(SceCtrlButtons btn) { }
            virtual void onButtonUp(SceCtrlButtons btn) { }
            
            virtual void onLeftAnalog(const vec2& pos, const vec2& delta) { }
            virtual void onRightAnalog(const vec2& pos, const vec2& delta) { }
            
        protected:
            friend class DeviceInput;
            DeviceInput* m_input;
            f32 left_analog_deadzone;
            f32 right_analog_deadzone;
            InputReceiver* next;
            InputReceiver* last;
    };
    
    class DeviceInput {
        public:
            DeviceInput ();
            ~DeviceInput ();
            
            void bind (InputReceiver* rec, f32 leftDeadzone = 0.9f, f32 rightDeadzone = 0.9f);

            int scan ();
            bool button (unsigned int button) const;
            vec2 left_stick() const { return m_ls; }
            vec2 right_stick() const { return m_rs; }
        
        protected:
            friend class InputReceiver;
            vec2 m_analogDeadzone;
            vec2 m_ls;
            vec2 m_rs;
            SceCtrlData m_oldCtrl;
            SceCtrlData m_ctrl;
            InputReceiver* m_activeReceivers;
            InputReceiver* m_inputStolenBy;
    };
};
