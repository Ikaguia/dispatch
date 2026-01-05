#include <algorithm>
#include <memory>
#include <iostream>

#include <TextureManager.hpp>

TextureManager::TextureManager() {}
TextureManager& TextureManager::inst() {
	static TextureManager singleton;
	return singleton;
}

void TextureManager::load(const std::string& filePath, const std::string& key) {
	try {
		raylib::Texture t{filePath};
		textures.emplace(key.empty() ? filePath : key, std::move(t));
	} catch (std::exception& e) {
		std::cerr << "Key: " << key << ", filePath: " << filePath << std::endl;
		throw e;
	}
}
void TextureManager::unload(const std::string& key) { textures.erase(key); }
void TextureManager::clear() { textures.clear(); }

bool TextureManager::has(const std::string& key) const { return textures.contains(key); }

const raylib::Texture& TextureManager::operator[](const std::string& key) const { return textures.at(key); }
raylib::Texture& TextureManager::operator[](const std::string& key) { return textures.at(key); }
