// https://github.com/mrdoob/three.js/blob/r129/src/core/Object3D.js

#ifndef THREEPP_OBJECT3D_HPP
#define THREEPP_OBJECT3D_HPP

#include "threepp/math/Euler.hpp"
#include "threepp/math/MathUtils.hpp"
#include "threepp/math/Matrix3.hpp"
#include "threepp/math/Matrix4.hpp"
#include "threepp/math/Quaternion.hpp"
#include "threepp/math/Vector3.hpp"

#include "threepp/core/EventDispatcher.hpp"


#include <functional>
#include <memory>
#include <optional>

namespace threepp {

    class BufferGeometry;

    class Object3D : public std::enable_shared_from_this<Object3D>, private EventDispatcher {

    public:
        const unsigned int id = _object3Did++;

        std::string uuid = generateUUID();

        std::string name;

        std::shared_ptr<Object3D> parent;
        std::vector<std::shared_ptr<Object3D>> children;

        const Vector3 up = Vector3(0, 1, 0);

        Vector3 position;
        Euler rotation;
        Quaternion quaternion;
        Vector3 scale = Vector3(1, 1, 1);

        Matrix4 modelViewMatrix;
        Matrix3 normalMatrix;

        Matrix4 matrix;
        Matrix4 matrixWorld;

        bool matrixAutoUpdate = true;
        bool matrixWorldNeedsUpdate = false;

        bool visible = true;

        bool castShadows = true;
        bool receiveShadow = true;

        bool frustumCulled = true;
        unsigned int renderOrder = 0;

        Object3D(const Object3D &) = delete;

        virtual std::string type() const {
            return "Object3D";
        }

        virtual std::shared_ptr<BufferGeometry> geometry() {
            return nullptr;
        }

        void applyMatrix4(const Matrix4 &matrix) {

            if (this->matrixAutoUpdate) this->updateMatrix();

            this->matrix.premultiply(matrix);

            this->matrix.decompose(this->position, this->quaternion, this->scale);
        }

        Object3D &applyQuaternion(const Quaternion &q) {

            this->quaternion.premultiply(q);

            return *this;
        }

        void setRotationFromAxisAngle(const Vector3 &axis, float angle) {

            // assumes axis is normalized

            this->quaternion.setFromAxisAngle(axis, angle);
        }

        void setRotationFromEuler(const Euler &euler) {

            this->quaternion.setFromEuler(euler, true);
        }

        void setRotationFromMatrix(const Matrix4 &m) {

            // assumes the upper 3x3 of m is a pure rotation matrix (i.e, unscaled)

            this->quaternion.setFromRotationMatrix(m);
        }

        void setRotationFromQuaternion(const Quaternion &q) {

            // assumes q is normalized

            this->quaternion = q;
        }

        Object3D &rotateOnAxis(const Vector3 &axis, float angle) {

            // rotate object on axis in object space
            // axis is assumed to be normalized

            _q1.setFromAxisAngle(axis, angle);

            this->quaternion.multiply(_q1);

            return *this;
        }

        Object3D &rotateOnWorldAxis(const Vector3 &axis, float angle) {

            // rotate object on axis in world space
            // axis is assumed to be normalized
            // method assumes no rotated parent

            _q1.setFromAxisAngle(axis, angle);

            this->quaternion.premultiply(_q1);

            return *this;
        }

        Object3D &rotateX(float angle) {

            return this->rotateOnAxis(Vector3::X, angle);
        }

        Object3D &rotateY(float angle) {

            return this->rotateOnAxis(Vector3::Y, angle);
        }

        Object3D &rotateZ(float angle) {

            return this->rotateOnAxis(Vector3::Z, angle);
        }

        Object3D &translateOnAxis(const Vector3 &axis, float distance) {

            // translate object by distance along axis in object space
            // axis is assumed to be normalized

            _v1.copy(axis).applyQuaternion(this->quaternion);

            this->position.add(_v1.multiply(distance));

            return *this;
        }

        Object3D &translateX(float distance) {

            return this->translateOnAxis(Vector3::X, distance);
        }

        Object3D &translateY(float distance) {

            return this->translateOnAxis(Vector3::Y, distance);
        }

        Object3D &translateZ(float distance) {

            return this->translateOnAxis(Vector3::Z, distance);
        }

        void localToWorld(Vector3 &vector) const {

            vector.applyMatrix4(this->matrixWorld);
        }

        void worldToLocal(Vector3 &vector) const {

            vector.applyMatrix4(_m1.copy(this->matrixWorld).invert());
        }

        void lookAt(const Vector3 &vector) {
            lookAt(vector.x, vector.y, vector.z);
        }

        void lookAt(float x, float y, float z) {

            // TODO

            //            // This method does not support objects having non-uniformly-scaled parent(s)
            //
            //            _target.set(x, y, z);
            //
            //            this->updateWorldMatrix(true, false);
            //
            //            _position.setFromMatrixPosition(this->matrixWorld);
            //
            //            if (this->isCamera || this->isLight) {
            //
            //                _m1.lookAt(_position, _target, this->up);
            //
            //            } else {
            //
            //                _m1.lookAt(_target, _position, this->up);
            //            }
            //
            //            this->quaternion.setFromRotationMatrix(_m1);
            //
            //            if (parent) {
            //
            //                _m1.extractRotation(parent.matrixWorld);
            //                _q1.setFromRotationMatrix(_m1);
            //                this->quaternion.premultiply(_q1.invert());
            //            }
        }

        Object3D &add(const std::shared_ptr<Object3D> &object) {

            if (object->parent) {

                object->parent->remove( object );
            }

            object->parent = shared_from_this();
            this->children.emplace_back(object);

            object->dispatchEvent("added");

            return *this;
        }

        Object3D &remove(const std::shared_ptr<Object3D> &object) {

            auto find = std::find(children.begin(), children.end(), object);
            if (find != children.end()) {
                children.erase(find);
                object->parent = nullptr;
                object->dispatchEvent("remove");
            }

            return *this;
        }

        Object3D &removeFromParent() {

            if (parent) {

                parent->remove(shared_from_this());
            }

            return *this;
        }

        Object3D &clear() {

            for (auto &object : this->children) {

                object->parent = nullptr;

                object->dispatchEvent("remove");
            }

            this->children.clear();

            return *this;
        }

        std::shared_ptr<Object3D> getObjectByName(const std::string &name) {

            if (this->name == name) return shared_from_this();

            for (auto &child : this->children) {

                auto object = child->getObjectByName(name);

                if (object) {

                    return object;
                }
            }

            return nullptr;
        }

        void getWorldPosition(Vector3 &target) {

            this->updateWorldMatrix(true, false);

            target.setFromMatrixPosition(this->matrixWorld);
        }

        void getWorldQuaternion(Quaternion &target) {

            this->updateWorldMatrix(true, false);

            this->matrixWorld.decompose(_position, target, _scale);
        }

        void getWorldScale(Vector3 &target) {

            this->updateWorldMatrix(true, false);

            this->matrixWorld.decompose(_position, _quaternion, target);
        }

        void getWorldDirection(Vector3 &target) {

            this->updateWorldMatrix(true, false);

            auto e = this->matrixWorld.elements();

            target.set(e[8], e[9], e[10]).normalize();
        }

        void traverse(const std::function<void(std::shared_ptr<Object3D>)> &callback) {

            callback(shared_from_this());

            for (auto &i : children) {

                i->traverse(callback);
            }
        }

        void traverseVisible(const std::function<void(std::shared_ptr<Object3D>)> &callback) {

            if (!this->visible) return;

            callback(shared_from_this());

            for (auto &i : children) {

                i->traverseVisible(callback);
            }
        }

        void traverseAncestors(const std::function<void(std::shared_ptr<Object3D>)> &callback) const {

            if (parent) {

                callback(parent);

                parent->traverseAncestors(callback);
            }
        }

        void updateMatrix() {

            this->matrix.compose(this->position, this->quaternion, this->scale);

            this->matrixWorldNeedsUpdate = true;
        }

        void updateMatrixWorld(bool force = false) {

            if (this->matrixAutoUpdate) this->updateMatrix();

            if (this->matrixWorldNeedsUpdate || force) {

                if (!this->parent) {

                    this->matrixWorld = (this->matrix);

                } else {

                    this->matrixWorld.multiplyMatrices(this->parent->matrixWorld, this->matrix);
                }

                this->matrixWorldNeedsUpdate = false;

                force = true;
            }

            // update children

            for (auto &child : this->children) {

                child->updateMatrixWorld(force);
            }
        }

        void updateWorldMatrix(bool updateParents, bool updateChildren) {

            if (updateParents && parent) {

                parent->updateWorldMatrix(true, false);
            }

            if (this->matrixAutoUpdate) this->updateMatrix();

            if (!this->parent) {

                this->matrixWorld = (this->matrix);

            } else {

                this->matrixWorld.multiplyMatrices(this->parent->matrixWorld, this->matrix);
            }

            // update children

            if (updateChildren) {

                for (auto &child : children) {

                    child->updateWorldMatrix(false, true);
                }
            }
        }

        static std::shared_ptr<Object3D> create() {
            return std::shared_ptr<Object3D>(new Object3D());
        }

        ~Object3D() = default;

    protected:
        Object3D() {
            rotation._onChange(onRotationChange);
            quaternion._onChange(onQuaternionChange);
        };

    private:
        std::function<void()> onRotationChange = [&] {
            quaternion.setFromEuler(rotation, false);
        };

        std::function<void()> onQuaternionChange = [&] {
            rotation.setFromQuaternion(quaternion, std::nullopt, false);
        };

        static Vector3 _v1;
        static Quaternion _q1;
        static Matrix4 _m1;
        static Vector3 _target;

        static Vector3 _scale;
        static Vector3 _position;
        static Quaternion _quaternion;

        static unsigned int _object3Did;
    };

}// namespace threepp

#endif// THREEPP_OBJECT3D_HPP
