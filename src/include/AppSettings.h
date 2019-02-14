#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <string>

namespace Engine {
class Settings {
public:
    Settings(std::string settingsFile)
        : filePath(settingsFile)
    {
    }
    bool load();
    bool store();

    std::pair<int, int> getResolution() const;
    float getFov() const;
    float getRenderDistance() const;
    bool getFullscreen() const;
    int getRefreshRate() const;
    float getBrightness() const;
    std::string getLevelPath() const;
    bool withValidationLayer() const;

    void updateResolution(int w, int h);
    void updateFullscreen(bool fs);
    void updateRefreshRate(int rate);
    void updateBrightness(int b);

private:
    int width = 1366;
    int height = 768;
    float fov = 70.0f;
    float renderDistance = 1000.0f;
    int refreshRate = 60;
    float brightness = 1.0f;
    bool isFullscreen = false;
    bool validation = false;
    std::string levelPath;

    std::string filePath;
};
}

#endif // APP_SETTINGS_H
