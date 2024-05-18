#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../model.h"

#include <vector>
#include <memory>
#include <tuple>

class Transform
{
public:
    void computeModelMatrix(){
        _modelMatrix = getLocalModelMatrix();
        _isDirty = false;
    }
    void computeModelMatrix(const glm::mat4& parentGlobalModelMatrix){
        _modelMatrix = parentGlobalModelMatrix * getLocalModelMatrix();
        _isDirty = false;
    }
    
    void setLocalScale(const glm::vec3& scale){
        _scale = scale;
        _isDirty = true;
    }
    
    void setLocalRotation(const glm::vec3& rot){
        _eulerRot = rot;
        _isDirty = true;
    }
    void setLocalPosition(const glm::vec3& pos){
        _pos = pos;
        _isDirty = true;
    }

    const glm::vec3 getGlobalPosition() const{
		return _modelMatrix[3];
	}

	const glm::vec3& getLocalPosition() const{
		return _pos;
	}

	const glm::vec3& getLocalRotation() const{
		return _eulerRot;
	}

	const glm::vec3& getLocalScale() const{
		return _scale;
	}

	const glm::mat4& getModelMatrix() const{
		return _modelMatrix;
	}

    glm::vec3 getRight() const{
		return _modelMatrix[0];
	}

	glm::vec3 getUp() const{
		return _modelMatrix[1];
	}

	glm::vec3 getBackward() const{
		return _modelMatrix[2];
	}

	glm::vec3 getForward() const{
		return -_modelMatrix[2];
	}

	glm::vec3 getGlobalScale() const{
		return { glm::length(getRight()), glm::length(getUp()), glm::length(getBackward()) };
	}

    bool isDirty(){
        return _isDirty;
    }

private:
    bool _isDirty = true;
    // local space information
    glm::vec3 _pos       = {0.0f ,0.0f, 0.0f};
    glm::vec3 _eulerRot  = {0.0f ,0.0f, 0.0f};
    glm::vec3 _scale     = {0.0f ,0.0f, 0.0f};

    // global space information concatenate in matrix
    glm::mat4 _modelMatrix = glm::mat4(1.0f);

    glm::mat4 getLocalModelMatrix(){
        const glm::mat4 transformX = glm::rotate(glm::mat4(1.0), glm::radians(_eulerRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
        const glm::mat4 transformY = glm::rotate(glm::mat4(1.0), glm::radians(_eulerRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 transformZ = glm::rotate(glm::mat4(1.0), glm::radians(_eulerRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // y * x * z
        const glm::mat4 rotationMatrix = transformY * transformX * transformZ;

        // translation * rotation * scale (TRS matrix)
        return glm::translate(glm::mat4(1.0f), _pos) * rotationMatrix * glm::scale(glm::mat4(1.0f), _scale);
    }
};

class Entity : public Model
{
public:
    Transform transform;
    Entity(const std::string &path) : Model(path){
    }
    ~Entity() = default;
    
    // scene graph
    std::vector<std::unique_ptr<Entity>> children;
    Entity *parent = nullptr;

    template<typename... TArgs>
    void addChild(TArgs&&... args){
        children.emplace_back(std::make_unique<Entity>(std::forward<TArgs>(args)...));
        children.back()->parent = this;
    }

    void updateSelfAndChild(){
        if (transform.isDirty()){
            forceUpdateSelfAndChild();
            return;
        }
        for (auto &&child : children){
            child->updateSelfAndChild();
        }
    }
    void forceUpdateSelfAndChild(){
        if (parent)
            transform.computeModelMatrix(parent->transform.getModelMatrix());
        else
            transform.computeModelMatrix();
        for (auto &&child : children){
            child->forceUpdateSelfAndChild();
        }
    }
};

// class Scene{
// public:
//     void draw(){}
//     void update(){}
// private:
//     std::vector<std::pair<Entity,Shader>> objList;
// };