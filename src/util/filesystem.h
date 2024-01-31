#pragma once
// This code obtained from : https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/filesystem.h

#include <string>
#include <cstdlib>
#include "config.h"

class FileSystem
{
private:
    typedef std::string(*Builder)(const std::string& path);
public:
    static std::string getPath(const std::string& path){
        static std::string(*pathBuilder)(const std::string&) = getPathBuilder();
        return (*pathBuilder)(path);
    }
private:
    static std::string const &getRoot(){
        static char const * envRoot = getenv("LOGL_ROOT_PATH");
        static char const * givenRoot = (envRoot != nullptr ? envRoot : logl_root);
        static std::string root (givenRoot != nullptr ? givenRoot : "");
        return root;
    }
    static Builder getPathBuilder(){
        if (getRoot() != "")
            return &FileSystem::getPathRelativeRoot;
        else
            return &FileSystem::getPathRelativeBinary;
    }
    static std::string getPathRelativeRoot(const std::string& path){
        return getRoot() + std::string("/") + path;
    }
    static std::string getPathRelativeBinary(const std::string& path){
        return "../../../" + path;
    }
};