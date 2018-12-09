#include <stdio.h>
#include <dirent.h>
#include <math.h>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

#include <common/debugLog.h>
#include <system/device.h>
#include <rendering/xmb.h>
using namespace v;

#define printf debugLog
namespace v {
    class Application {
        public:
            Application (const vector<string>& Arguments) {
                for(u32 i = 0;i < Arguments.size();i++) {
                    printf("Arg %d: %s\n", i, Arguments[i].c_str());
                }
            }
            
            ~Application () {
                printf("Exiting...\n");
            }
            
            int run () {
                Xmb* xmb = new Xmb(&m_device.gpu());
                m_device.input().bind(xmb);
                
                f32 lastTime = m_device.time();
                f32 fps = 0.0f;
                f32 dt = 0.0f;
                printf("Starting loop\n");
                vec3 color = vec3();
                while(!m_device.input().button(SCE_CTRL_TRIANGLE)) {
                    f32 curTime = m_device.time();
                    dt = curTime - lastTime;
                    fps = 1.0f / dt;
                    lastTime = curTime;
                    
                    m_device.input().scan();
                    
                    m_device.gpu().begin_frame();
                    m_device.gpu().clear_screen();
                    xmb->update(dt);
                    xmb->render();
                    m_device.gpu().end_frame();
                    m_device.screen().vblank();
                }
                
                return 0;
            }
            
            Device m_device;
    };
};

int main(int argc, char *argv[]) {
    vector<string> args;
    for(unsigned char i = 0;i < argc;i++) args.push_back(argv[i]);
    Application app(args);
    return app.run();
}
