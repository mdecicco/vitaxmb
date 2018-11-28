#pragma once
#include <psp2/ctrl.h>
#include <common/types.h>
#include <vector>
using namespace std;

namespace v {
    class InputReceiver {
        public:
            InputReceiver () { }
            virtual ~InputReceiver () { }
            
            virtual void onButtonDown(SceCtrlButtons btn) { }
            virtual void onButtonHeld(SceCtrlButtons btn) { }
            virtual void onButtonUp(SceCtrlButtons btn) { }
            
            virtual void onLeftAnalog(const vec2& pos, const vec2& delta) { }
            virtual void onRightAnalog(const vec2& pos, const vec2& delta) { }
    };
    
    class DeviceInput {
        public:
            DeviceInput ();
            ~DeviceInput ();
            
            void bind (InputReceiver* rec) { m_receivers.push_back(rec); }

            int scan ();
            bool button (unsigned int button) const;
            vec2 left_stick() const { return m_ls; }
            vec2 right_stick() const { return m_rs; }
        
        protected:
            vec2 m_analogDeadzone;
            vec2 m_ls;
            vec2 m_rs;
            SceCtrlData m_oldCtrl;
            SceCtrlData m_ctrl;
            vector<InputReceiver*> m_receivers;
    };
};
