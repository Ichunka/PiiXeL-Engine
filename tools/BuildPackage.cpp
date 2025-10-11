#include "Build/GamePackageBuilder.hpp"
#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {
    std::string projectPath = ".";
    std::string outputPath = "datas/game.package";

    if (argc > 1) {
        projectPath = argv[1];
    }

    if (argc > 2) {
        outputPath = argv[2];
    } else {
        std::filesystem::path workingDir = std::filesystem::current_path();
        outputPath = (workingDir / "datas" / "game.package").string();
    }

    std::filesystem::path outputDir = std::filesystem::path(outputPath).parent_path();
    if (!std::filesystem::exists(outputDir)) {
        std::filesystem::create_directories(outputDir);
        std::cout << "Created directory: " << outputDir.string() << std::endl;
    }

    std::cout << "Building game package..." << std::endl;
    std::cout << "Project path: " << projectPath << std::endl;
    std::cout << "Output path: " << outputPath << std::endl;

    PiiXeL::GamePackageBuilder builder{};

    if (builder.BuildFromProject(projectPath, outputPath)) {
        std::cout << "Game package built successfully!" << std::endl;
        return 0;
    } else {
        std::cerr << "Failed to build game package" << std::endl;
        return 1;
    }
}
