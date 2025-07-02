from conan import ConanFile
from conan.tools.cmake import CMake, cmake_layout

class Conan(ConanFile):
    name = "vkeng"
    version = "1.0"
    license = "MIT"
    author = "Allan Schöttler"
    url = "https://github.com/allanschottler/VkEng"
    description = "Vulkan Engine"
    topics = ("Vulkan", "C++")
    
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = [
        "CMakeLists.txt",
        "include/*",
        "demo/*",
        "demo/Triangle2D/*",
        "demo/Triangle2D/shaders/*"
    ]

    requires = [
        "boost/1.85.0",
        "glm/1.0.1",
        "range-v3/0.12.0",
        "glfw/3.4",
        "vulkan-headers/1.4.309.0",
        "vulkan-loader/1.4.309.0",
        "libpng/1.6.40"
    ]
    
    def configure(self):
        self.options["glfw"].shared = False
        self.options["vulkan-loader"].shared = False
    
    def layout(self):
        cmake_layout(self)
    
    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
    
