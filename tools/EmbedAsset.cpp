#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

struct Asset {
    std::string filePath;
    std::string assetName;
    std::string identifier;
    std::vector<unsigned char> data;
};

std::string MakeCIdentifier(const std::string& name) {
    std::string result;
    for (char c : name) {
        if (std::isalnum(c)) {
            result += c;
        } else {
            result += '_';
        }
    }
    return result;
}

bool ReadAssetFile(Asset& asset) {
    std::ifstream file(asset.filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Error: Cannot open file: " << asset.filePath << std::endl;
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    asset.data.resize(size);
    if (!file.read(reinterpret_cast<char*>(asset.data.data()), size)) {
        std::cerr << "Error: Cannot read file: " << asset.filePath << std::endl;
        return false;
    }

    return true;
}

void GenerateAssetHeader(const Asset& asset, const std::string& outputDir) {
    std::string outputPath = outputDir + "/" + asset.identifier + ".hpp";
    std::ofstream out(outputPath);

    if (!out) {
        std::cerr << "Error: Cannot create output file: " << outputPath << std::endl;
        return;
    }

    out << "#pragma once\n";
    out << "#include <cstddef>\n";
    out << "#include <array>\n";
    out << "#include <string_view>\n\n";
    out << "namespace PiiXeL::EmbeddedAssets {\n\n";
    out << "constexpr std::string_view k_" << asset.identifier << "_Name = \"" << asset.assetName << "\";\n";
    out << "constexpr std::size_t k_" << asset.identifier << "_Size = " << asset.data.size() << ";\n\n";
    out << "inline constexpr std::array<std::byte, " << asset.data.size() << "> k_" << asset.identifier << "_Data = {\n    ";

    for (size_t i = 0; i < asset.data.size(); ++i) {
        if (i > 0) {
            out << ", ";
            if (i % 16 == 0) {
                out << "\n    ";
            }
        }
        out << "std::byte{0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(asset.data[i]) << "}";
    }

    out << "\n};\n\n";
    out << "}\n";

    std::cout << "Generated: " << outputPath << " (" << asset.data.size() << " bytes)" << std::endl;
}

void GenerateRegistry(const std::vector<Asset>& assets, const std::string& outputDir) {
    std::string registryPath = outputDir + "/EmbeddedAssetRegistry.generated.hpp";
    std::ofstream out(registryPath);

    if (!out) {
        std::cerr << "Error: Cannot create registry file: " << registryPath << std::endl;
        return;
    }

    out << "#pragma once\n";
    for (const Asset& asset : assets) {
        out << "#include \"" << asset.identifier << ".hpp\"\n";
    }

    out << "#include <unordered_map>\n";
    out << "#include <string_view>\n";
    out << "#include <span>\n";
    out << "#include <cstddef>\n\n";
    out << "namespace PiiXeL::EmbeddedAssets {\n\n";
    out << "inline const std::unordered_map<std::string_view, std::span<const std::byte>> Registry = {\n";

    for (size_t i = 0; i < assets.size(); ++i) {
        const Asset& asset = assets[i];
        if (i > 0) {
            out << ",\n";
        }
        out << "    { k_" << asset.identifier << "_Name, { k_" << asset.identifier << "_Data.data(), k_" << asset.identifier << "_Size } }";
    }

    out << "\n};\n\n";
    out << "}\n";

    std::cout << "Generated registry: " << registryPath << " (" << assets.size() << " assets)" << std::endl;
}

std::vector<std::string> ReadAssetListFile(const std::string& filepath) {
    std::vector<std::string> assets;
    std::ifstream file(filepath);

    if (!file) {
        std::cerr << "Error: Cannot open asset list file: " << filepath << std::endl;
        return assets;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line[0] != '#') {
            assets.push_back(line);
        }
    }

    return assets;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <output_dir> <asset_file:asset_name> [<asset_file:asset_name> ...]" << std::endl;
        std::cerr << "       " << argv[0] << " <output_dir> @<asset_list_file>" << std::endl;
        std::cerr << "Example: " << argv[0] << " build/embedded_assets splash.png:engine/ui/splash logo.png:engine/ui/logo" << std::endl;
        return 1;
    }

    std::string outputDir = argv[1];

    if (!fs::exists(outputDir)) {
        fs::create_directories(outputDir);
    }

    std::vector<std::string> assetArgs;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg[0] == '@') {
            std::string listFile = arg.substr(1);
            std::vector<std::string> assetsFromFile = ReadAssetListFile(listFile);
            assetArgs.insert(assetArgs.end(), assetsFromFile.begin(), assetsFromFile.end());
        } else {
            assetArgs.push_back(arg);
        }
    }

    std::vector<Asset> assets;

    for (const std::string& arg : assetArgs) {
        size_t colonPos = arg.rfind(':');

        if (colonPos == std::string::npos) {
            std::cerr << "Error: Invalid argument format (expected file:name): " << arg << std::endl;
            continue;
        }

        Asset asset;
        asset.filePath = arg.substr(0, colonPos);
        asset.assetName = arg.substr(colonPos + 1);
        asset.identifier = MakeCIdentifier(asset.assetName);

        if (!fs::exists(asset.filePath)) {
            std::cerr << "Warning: Asset file not found: " << asset.filePath << " (skipping)" << std::endl;
            continue;
        }

        if (ReadAssetFile(asset)) {
            GenerateAssetHeader(asset, outputDir);
            assets.push_back(asset);
        }
    }

    GenerateRegistry(assets, outputDir);

    if (assets.empty()) {
        std::cout << "No assets embedded (registry is empty)" << std::endl;
    }

    return 0;
}
