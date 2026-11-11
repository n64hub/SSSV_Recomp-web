// The launcher's 3D logo on a page: none. launcher_model3d.cpp draws it through plume's Vulkan, D3D12 and Metal
// backends with shaders built for them, and a page has WebGPU only, so with Emscripten this file is built in its place
// (CMakeLists.txt) and configure() says there is no model: the launcher shows its menu over its background alone.
#include "cs_sdk/launcher_model3d.h"

namespace csdk::launcher3d {
    void install_render_hook_chain() {}
    void prime_render_backend(void*, void*) {}
    bool configure(const Config&) { return false; }
    void set_enabled(bool) {}
    void on_launcher_menu_update(recompui::Element*) {}
    void reset_intro() {}
    void shutdown() {}
    Config get_current_tuning_snapshot() { return {}; }
    std::string make_cpp_initializer_snippet() { return {}; }
}
