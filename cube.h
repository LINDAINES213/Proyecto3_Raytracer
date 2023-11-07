#pragma once

#include <glm/glm.hpp>
#include "material.h"
#include "intersect.h"
#include "object.h"

class Cube : public Object {
public:
    Cube(const glm::vec3& min, const glm::vec3& max, const Material& mat);

    Intersect rayIntersect(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const override;

private:
    glm::vec3 min;
    glm::vec3 max;
};