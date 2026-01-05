#pragma once

#include <unordered_map>
#include <raylib-cpp.hpp>

class TextureManager {
private:
	TextureManager();
public:
	std::unordered_map<std::string, raylib::Texture> textures;
    static TextureManager& inst();

    void load(const std::string& filePath, const std::string& key="");
    void unload(const std::string& key);
    void clear();

    bool has(const std::string& key) const;

    const raylib::Texture& operator[](const std::string& key) const;
	raylib::Texture& operator[](const std::string& key);
};
