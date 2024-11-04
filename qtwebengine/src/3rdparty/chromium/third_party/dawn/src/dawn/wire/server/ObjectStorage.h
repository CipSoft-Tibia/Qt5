// Copyright 2019 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_DAWN_WIRE_SERVER_OBJECTSTORAGE_H_
#define SRC_DAWN_WIRE_SERVER_OBJECTSTORAGE_H_

#include <algorithm>
#include <map>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

#include "dawn/wire/WireCmd_autogen.h"
#include "dawn/wire/WireServer.h"

namespace dawn::wire::server {

// Whether this object has been allocated, or reserved for async object creation.
// Used by the KnownObjects queries
enum class AllocationState : uint32_t {
    Free,
    Reserved,
    Allocated,
};

template <typename T>
struct ObjectDataBase {
    // The backend-provided handle and generation to this object.
    T handle;
    ObjectGeneration generation = 0;

    AllocationState state;
};

// Stores what the backend knows about the type.
template <typename T>
struct ObjectData : public ObjectDataBase<T> {};

enum class BufferMapWriteState { Unmapped, Mapped, MapError };

template <>
struct ObjectData<WGPUBuffer> : public ObjectDataBase<WGPUBuffer> {
    // TODO(enga): Use a tagged pointer to save space.
    std::unique_ptr<MemoryTransferService::ReadHandle> readHandle;
    std::unique_ptr<MemoryTransferService::WriteHandle> writeHandle;
    BufferMapWriteState mapWriteState = BufferMapWriteState::Unmapped;
    WGPUBufferUsageFlags usage = WGPUBufferUsage_None;
    // Indicate if writeHandle needs to be destroyed on unmap
    bool mappedAtCreation = false;
};

struct DeviceInfo {
    Server* server;
    ObjectHandle self;
};

template <>
struct ObjectData<WGPUDevice> : public ObjectDataBase<WGPUDevice> {
    // Store |info| as a separate allocation so that its address does not move.
    // The pointer to |info| is used as the userdata to device callback.
    std::unique_ptr<DeviceInfo> info = std::make_unique<DeviceInfo>();
};

// Information of both an ID and an object data for use as a shorthand in doers.
template <typename T>
struct Known {
    ObjectId id;
    ObjectData<T>* data;

    const ObjectData<T>* operator->() const {
        ASSERT(data != nullptr);
        return data;
    }
    ObjectData<T>* operator->() {
        ASSERT(data != nullptr);
        return data;
    }

    ObjectHandle AsHandle() const {
        ASSERT(data != nullptr);
        return {id, data->generation};
    }
};

// Keeps track of the mapping between client IDs and backend objects.
template <typename T>
class KnownObjectsBase {
  public:
    using Data = ObjectData<T>;

    KnownObjectsBase() {
        // Reserve ID 0 so that it can be used to represent nullptr for optional object values
        // in the wire format. However don't tag it as allocated so that it is an error to ask
        // KnownObjects for ID 0.
        Data reservation;
        reservation.handle = nullptr;
        reservation.state = AllocationState::Free;
        mKnown.push_back(std::move(reservation));
    }

    // Get a backend objects for a given client ID.
    // Returns an error if the object wasn't previously allocated.
    WireResult GetNativeHandle(ObjectId id, T* handle) const {
        if (id >= mKnown.size()) {
            return WireResult::FatalError;
        }

        const Data* data = &mKnown[id];
        if (data->state != AllocationState::Allocated) {
            return WireResult::FatalError;
        }
        *handle = data->handle;

        return WireResult::Success;
    }

    WireResult Get(ObjectId id, Known<T>* result) {
        if (id >= mKnown.size()) {
            return WireResult::FatalError;
        }

        Data* data = &mKnown[id];
        if (data->state != AllocationState::Allocated) {
            return WireResult::FatalError;
        }

        *result = Known<T>{id, data};
        return WireResult::Success;
    }

    Known<T> FillReservation(ObjectId id, T handle) {
        ASSERT(id < mKnown.size());
        Data* data = &mKnown[id];
        ASSERT(data->state == AllocationState::Reserved);
        data->handle = handle;
        data->state = AllocationState::Allocated;
        return {id, data};
    }

    // Allocates the data for a given ID and returns it in result.
    // Returns false if the ID is already allocated, or too far ahead, or if ID is 0 (ID 0 is
    // reserved for nullptr). Invalidates all the Data*
    WireResult Allocate(Known<T>* result,
                        ObjectHandle handle,
                        AllocationState state = AllocationState::Allocated) {
        if (handle.id == 0 || handle.id > mKnown.size()) {
            return WireResult::FatalError;
        }

        Data data;
        data.state = state;
        data.handle = nullptr;

        if (handle.id >= mKnown.size()) {
            mKnown.push_back(std::move(data));
            *result = {handle.id, &mKnown.back()};
            return WireResult::Success;
        }

        if (mKnown[handle.id].state != AllocationState::Free) {
            return WireResult::FatalError;
        }

        // The generation should be strictly increasing.
        if (handle.generation <= mKnown[handle.id].generation) {
            return WireResult::FatalError;
        }
        // update the generation in the slot
        data.generation = handle.generation;

        mKnown[handle.id] = std::move(data);

        *result = {handle.id, &mKnown[handle.id]};
        return WireResult::Success;
    }

    // Marks an ID as deallocated
    void Free(ObjectId id) {
        ASSERT(id < mKnown.size());
        mKnown[id].state = AllocationState::Free;
    }

    std::vector<T> AcquireAllHandles() {
        std::vector<T> objects;
        for (Data& data : mKnown) {
            if (data.state == AllocationState::Allocated && data.handle != nullptr) {
                objects.push_back(data.handle);
                data.state = AllocationState::Free;
                data.handle = nullptr;
            }
        }

        return objects;
    }

    std::vector<T> GetAllHandles() const {
        std::vector<T> objects;
        for (const Data& data : mKnown) {
            if (data.state == AllocationState::Allocated && data.handle != nullptr) {
                objects.push_back(data.handle);
            }
        }

        return objects;
    }

  protected:
    std::vector<Data> mKnown;
};

template <typename T>
class KnownObjects : public KnownObjectsBase<T> {
  public:
    KnownObjects() = default;
};

template <>
class KnownObjects<WGPUDevice> : public KnownObjectsBase<WGPUDevice> {
  public:
    KnownObjects() = default;

    WireResult Allocate(Known<WGPUDevice>* result,
                        ObjectHandle handle,
                        AllocationState state = AllocationState::Allocated) {
        WIRE_TRY(KnownObjectsBase<WGPUDevice>::Allocate(result, handle, state));
        AddToKnownSet(*result);
        return WireResult::Success;
    }

    Known<WGPUDevice> FillReservation(ObjectId id, WGPUDevice handle) {
        Known<WGPUDevice> result = KnownObjectsBase<WGPUDevice>::FillReservation(id, handle);
        AddToKnownSet(result);
        return result;
    }

    void Free(ObjectId id) {
        mKnownSet.erase(mKnown[id].handle);
        KnownObjectsBase<WGPUDevice>::Free(id);
    }

    bool IsKnown(WGPUDevice device) const { return mKnownSet.count(device) != 0; }

  private:
    void AddToKnownSet(Known<WGPUDevice> device) {
        if (device->state == AllocationState::Allocated && device->handle != nullptr) {
            mKnownSet.insert(device->handle);
        }
    }
    std::unordered_set<WGPUDevice> mKnownSet;
};

// ObjectIds are lost in deserialization. Store the ids of deserialized
// objects here so they can be used in command handlers. This is useful
// for creating ReturnWireCmds which contain client ids
template <typename T>
class ObjectIdLookupTable {
  public:
    void Store(T key, ObjectId id) { mTable[key] = id; }

    // Return the cached ObjectId, or 0 (null handle)
    ObjectId Get(T key) const {
        const auto it = mTable.find(key);
        if (it != mTable.end()) {
            return it->second;
        }
        return 0;
    }

    void Remove(T key) {
        auto it = mTable.find(key);
        if (it != mTable.end()) {
            mTable.erase(it);
        }
    }

  private:
    std::map<T, ObjectId> mTable;
};

}  // namespace dawn::wire::server

#endif  // SRC_DAWN_WIRE_SERVER_OBJECTSTORAGE_H_
