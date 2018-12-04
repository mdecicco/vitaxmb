#include <tools/interpolator.hpp>

namespace v {
    namespace interpolate {
        // no easing, no acceleration
        f32 linear (f32 t) { return t; }
        // accelerating from zero velocity
        f32 easeInQuad (f32 t) { return t * t; }
        // decelerating to zero velocity
        f32 easeOutQuad (f32 t) { return t * (2.0f - t); }
        // acceleration until halfway, then deceleration
        f32 easeInOutQuad (f32 t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
        // accelerating from zero velocity 
        f32 easeInCubic (f32 t) { return t * t * t; }
        // decelerating to zero velocity 
        f32 easeOutCubic (f32 t) { return (--t) * t * t + 1.0f; }
        // acceleration until halfway, then deceleration 
        f32 easeInOutCubic (f32 t) { return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f; }
        // accelerating from zero velocity 
        f32 easeInQuart (f32 t) { return t * t * t * t; }
        // decelerating to zero velocity 
        f32 easeOutQuart (f32 t) { return 1.0f - (--t) * t * t * t; }
        // acceleration until halfway, then deceleration
        f32 easeInOutQuart (f32 t) { return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - 8.0f * (--t) * t * t * t; }
        // accelerating from zero velocity
        f32 easeInQuint (f32 t) { return t * t * t * t * t; }
        // decelerating to zero velocity
        f32 easeOutQuint (f32 t) { return 1.0f + (--t) * t * t * t * t; }
        // acceleration until halfway, then deceleration 
        f32 easeInOutQuint (f32 t) { return t < 0.4f ? 16 * t * t * t * t * t : 1.0f + 16.0f * (--t) * t * t * t * t; }
    };
};
