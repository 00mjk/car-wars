#include "NavigationMesh.h"

#include "ContentManager.h"

#include <iostream>
#include "../../Entities/EntityManager.h"
#include "../../Components/RigidbodyComponents/RigidbodyComponent.h"

NavigationMesh::NavigationMesh(nlohmann::json data) {
	columnCount = ContentManager::GetFromJson<size_t>(data["ColumnCount"], 100);
	rowCount = ContentManager::GetFromJson<size_t>(data["RowCount"], 100);
	spacing = ContentManager::GetFromJson<float>(data["Spacing"], 2.f);

	Initialize();
}

size_t NavigationMesh::GetVertexCount() const {
    return columnCount * rowCount;
}

float NavigationMesh::GetSpacing() const {
    return spacing;
}

void NavigationMesh::Initialize() {
    vertices = new NavigationVertex[GetVertexCount()];

	for (size_t row = 0; row < rowCount; ++row) {
        for (size_t col = 0; col < columnCount; ++col) {
            const size_t index = row*columnCount + col;
            // TODO: Cylinder positions
            vertices[index].position = glm::vec3(rowCount * -0.5f*spacing + row*spacing, 1.f, columnCount * -0.5f*spacing + col*spacing);
		}
	}

    InitializeRenderBuffers();
    UpdateMesh();
}

void NavigationMesh::UpdateMesh() {
    UpdateMesh(EntityManager::GetComponents(ComponentType_RigidDynamic));
    UpdateMesh(EntityManager::GetComponents(ComponentType_RigidStatic));
    UpdateMesh(EntityManager::GetComponents(ComponentType_Vehicle));
}

void NavigationMesh::UpdateMesh(std::vector<Component*> rigidbodies) {
    for (Component* component : rigidbodies) {
        RigidbodyComponent *rigidbody = static_cast<RigidbodyComponent*>(component);
        UpdateMesh(rigidbody);
    }

    UpdateRenderBuffers();
}

void NavigationMesh::UpdateMesh(RigidbodyComponent* rigidbody) {
    physx::PxBounds3 bounds = rigidbody->pxRigid->getWorldBounds(1.f);
    std::vector<size_t> contained = FindAllContainedBy(bounds);
    for (size_t index : contained) {
        vertices[index].score = 0.f;
    }
}

size_t NavigationMesh::FindClosestVertex(glm::vec3 worldPosition) const {
    // Binary search over rows
    size_t left = 0;
    size_t right = rowCount - 1;
    size_t row = 0;
    glm::vec3 position;
    while (left < right) {
        row = (left + right) / 2;
        position = GetPosition(row, 0);
        if (position.x < worldPosition.x) {
            left = row + 1;
        } else if (position.x > worldPosition.x) {
            right = row - 1;
        } else {
            break;
        }
    }

    // Find shortest distance from best guess row
    float shortestDist = abs(position.x - worldPosition.x);
    for (size_t r = row - 2; r < row + 2; ++r) {
        const float dist = abs(GetPosition(r, 0).x - worldPosition.x);
        if (dist < shortestDist) {
            shortestDist = dist;
            row = r;
        }
    }

    // Binary search over columns
    left = 0;
    right = columnCount - 1;
    size_t column = 0;
    while (left < right) {
        column = (left + right) / 2;
        const glm::vec3 position = GetPosition(row, column);
        if (position.z < worldPosition.z) {
            left = column + 1;
        } else if (position.z > worldPosition.z) {
            right = column - 1;
        } else {
            break;
        }
    }

    // Find shortest distance from best guess column
    shortestDist = abs(position.z - worldPosition.z);
    for (size_t c = column - 2; c < column + 2; ++c) {
        const float dist = abs(GetPosition(row, c).z - worldPosition.z);
        if (dist < shortestDist) {
            shortestDist = dist;
            column = c;
        }
    }

    return row * columnCount + column;
}

glm::vec3 NavigationMesh::GetPosition(size_t index) const {
    return GetVertex(index).position;
}

float NavigationMesh::GetScore(size_t index) const {
    return GetVertex(index).score;
}

NavigationVertex NavigationMesh::GetVertex(size_t row, size_t col) const {
    return GetVertex(row * columnCount + col);
}

glm::vec3 NavigationMesh::GetPosition(size_t row, size_t col) const {
    return GetVertex(row, col).position;
}

float NavigationMesh::GetScore(size_t row, size_t col) const {
    return GetVertex(row, col).score;
}

std::vector<size_t> NavigationMesh::GetNeighbours(size_t index) {
    std::vector<size_t> neighbours;
    
    const int left = GetLeft(index);
    if (left != -1) neighbours.push_back(left);

    const int forwardLeft = GetForward(left);
    if (forwardLeft != -1) neighbours.push_back(forwardLeft);

    const int backwardLeft = GetBackward(left);
    if (backwardLeft != -1) neighbours.push_back(backwardLeft);

    const int right = GetRight(index);
    if (right != -1) neighbours.push_back(right);

    const int forwardRight = GetForward(right);
    if (forwardRight != -1) neighbours.push_back(forwardRight);

    const int backwardRight = GetBackward(right);
    if (backwardRight != -1) neighbours.push_back(backwardRight);

    const int forward = GetForward(index);
    if (forward != -1) neighbours.push_back(forward);

    const int backward = GetBackward(index);
    if (backward != -1) neighbours.push_back(backward);

    return neighbours;
}

NavigationVertex NavigationMesh::GetVertex(size_t index) const {
    return vertices[index];
}

// TODO: Make less ugly (split into sub-functions)
std::vector<size_t> NavigationMesh::FindAllContainedBy(physx::PxBounds3 bounds) {
    std::vector<size_t> contained;

    const size_t closest = FindClosestVertex(Transform::FromPx(bounds.getCenter()));
    const physx::PxVec3 position = Transform::ToPx(vertices[closest].position);
    if (!bounds.contains(position)) return contained;
    contained.push_back(closest);
    
    int left = closest;
    while ((left = GetLeft(left)) != -1) {
        const physx::PxVec3 position = Transform::ToPx(vertices[left].position);
        if (!bounds.contains(position)) break;
        contained.push_back(left);

        int forward = left;
        while ((forward = GetForward(forward)) != -1) {
            const physx::PxVec3 position = Transform::ToPx(vertices[forward].position);
            if (!bounds.contains(position)) break;
            contained.push_back(forward);
        }

        int backward = left;
        while ((backward = GetBackward(backward)) != -1) {
            const physx::PxVec3 position = Transform::ToPx(vertices[backward].position);
            if (!bounds.contains(position)) break;
            contained.push_back(backward);
        }
    }

    int right = closest;
    while ((right = GetRight(right)) != -1) {
        const physx::PxVec3 position = Transform::ToPx(vertices[right].position);
        if (!bounds.contains(position)) break;
        contained.push_back(right);

        int forward = right;
        while ((forward = GetForward(forward)) != -1) {
            const physx::PxVec3 position = Transform::ToPx(vertices[forward].position);
            if (!bounds.contains(position)) break;
            contained.push_back(forward);
        }

        int backward = right;
        while ((backward = GetBackward(backward)) != -1) {
            const physx::PxVec3 position = Transform::ToPx(vertices[backward].position);
            if (!bounds.contains(position)) break;
            contained.push_back(backward);
        }
    }

    int forward = closest;
    while ((forward = GetForward(forward)) != -1) {
        const physx::PxVec3 position = Transform::ToPx(vertices[forward].position);
        if (!bounds.contains(position)) break;
        contained.push_back(forward);
    }

    int backward = closest;
    while ((backward = GetBackward(backward)) != -1) {
        const physx::PxVec3 position = Transform::ToPx(vertices[backward].position);
        if (!bounds.contains(position)) break;
        contained.push_back(backward);
    }

    return contained;
}

int NavigationMesh::GetForward(size_t index) const {
    const int forward = index + columnCount;
    return forward < GetVertexCount() ? forward : -1;
}

int NavigationMesh::GetBackward(size_t index) const {
    const int backward = index - columnCount;
    return backward >= 0 ? backward : -1;
}

int NavigationMesh::GetLeft(size_t index) const {
    const int left = index - 1;
    return left % columnCount != columnCount - 1 ? left : -1;
}

int NavigationMesh::GetRight(size_t index) const {
    const int right = index + 1;
    return right % columnCount != 0 ? right : -1;
}

void NavigationMesh::InitializeRenderBuffers() {
    glGenBuffers(1, &vbo);
    UpdateRenderBuffers();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(NavigationVertex), reinterpret_cast<void*>(0));        // score

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(NavigationVertex), reinterpret_cast<void*>(4));        // position
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void NavigationMesh::UpdateRenderBuffers() const {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * rowCount * columnCount, vertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}