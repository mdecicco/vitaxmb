#pragma once
#include <common/types.h>
#include <psp2/rtc.h>
#include <functional>

namespace v {
    typedef f32 (*InterpolationFactorCallback)(f32);
    
    namespace interpolate {
        // no easing, no acceleration
        f32 linear (f32 normalizedInput);
        // accelerating from zero velocity
        f32 easeInQuad (f32 normalizedInput);
        // decelerating to zero velocity
        f32 easeOutQuad (f32 normalizedInput);
        // acceleration until halfway, then deceleration
        f32 easeInOutQuad (f32 normalizedInput);
        // accelerating from zero velocity 
        f32 easeInCubic (f32 normalizedInput);
        // decelerating to zero velocity 
        f32 easeOutCubic (f32 normalizedInput);
        // acceleration until halfway, then deceleration 
        f32 easeInOutCubic (f32 normalizedInput);
        // accelerating from zero velocity 
        f32 easeInQuart (f32 normalizedInput);
        // decelerating to zero velocity 
        f32 easeOutQuart (f32 normalizedInput);
        // acceleration until halfway, then deceleration
        f32 easeInOutQuart (f32 normalizedInput);
        // accelerating from zero velocity
        f32 easeInQuint (f32 normalizedInput);
        // decelerating to zero velocity
        f32 easeOutQuint (f32 normalizedInput);
        // acceleration until halfway, then deceleration 
        f32 easeInOutQuint (f32 normalizedInput);
    };
    
    template <typename t>
    class Interpolator {
        public:
            Interpolator (const t& initial, f32 duration, InterpolationFactorCallback method) :
                m_initial(initial), m_final(initial), m_duration(duration), m_useFinishedCb(false),
                m_transitionMethod(method)
                {
                    m_startTick.tick = 0;
                }
            ~Interpolator () { }
            
            f32 duration () const { return m_duration; }
            void duration (f32 duration) { m_duration = duration; }
            void set_immediate (const t& value) {
                m_initial = value;
                m_final = value;
                m_useFinishedCb = false;
                m_startTick.tick = 0;
            }
            void then (std::function<void()> cb) {
                m_useFinishedCb = true;
                m_finished = cb;
            }
            
            Interpolator& operator = (const t& value) {
                sceRtcGetCurrentTick(&m_startTick);
                m_final = value;
                m_useFinishedCb = false;
                return *this;
            }
            operator t() {
                if(m_startTick.tick == 0) return m_final;
                SceRtcTick now;
                sceRtcGetCurrentTick(&now);
                f32 elapsed = f32(now.tick - m_startTick.tick) / f32(sceRtcGetTickResolution());
                if(elapsed >= m_duration) {
                    m_startTick.tick = 0;
                    m_initial = m_final;
                    if(m_useFinishedCb) {
                        m_useFinishedCb = false;
                        m_finished();
                    }
                    return m_initial;
                }
                f32 factor = m_transitionMethod(elapsed / m_duration);
                return m_initial + ((m_final - m_initial) * factor);
            }
        
        protected:
            InterpolationFactorCallback m_transitionMethod;
            bool m_useFinishedCb;
            std::function<void()> m_finished;
            t m_final;
            t m_initial;
            SceRtcTick m_startTick;
            f32 m_duration;
    };
};
