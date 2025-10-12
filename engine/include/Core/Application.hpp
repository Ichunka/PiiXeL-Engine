#ifndef PIIXELENGINE_APPLICATION_HPP
#define PIIXELENGINE_APPLICATION_HPP

#include <string>
#include <memory>

namespace PiiXeL {

class Engine;
#ifdef BUILD_WITH_EDITOR
class EditorLayer;
#endif

struct ApplicationConfig {
    std::string title{"PiiXeL Engine"};
    int windowWidth{1920};
    int windowHeight{1080};
    int targetFPS{60};
    bool vsync{true};
    bool resizable{true};
    bool fullscreen{false};
    std::string iconPath{};
};

class Application {
public:
    explicit Application(const ApplicationConfig& config);
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void Run();
    void Shutdown();

    [[nodiscard]] bool IsRunning() const { return m_Running; }
    [[nodiscard]] Engine* GetEngine() const { return m_Engine.get(); }

private:
    void Initialize();
    void Update(float deltaTime);
    void Render();
    void Cleanup();

private:
    ApplicationConfig m_Config;
    std::unique_ptr<Engine> m_Engine;
#ifdef BUILD_WITH_EDITOR
    std::unique_ptr<EditorLayer> m_EditorLayer;
#endif
    bool m_Running{false};
    bool m_Initialized{false};
};

} // namespace PiiXeL

#endif // PIIXELENGINE_APPLICATION_HPP
