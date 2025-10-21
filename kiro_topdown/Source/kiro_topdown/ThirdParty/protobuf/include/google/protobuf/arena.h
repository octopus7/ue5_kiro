// Simplified protobuf arena header
#pragma once

PROTOBUF_NAMESPACE_OPEN

class Arena {
public:
    template<typename T>
    static T* CreateMaybeMessage(Arena* arena) {
        return new T();
    }
    
    template<typename T>
    class InternalHelper {
    public:
        static T* New(Arena* arena) {
            return new T();
        }
    };
};

PROTOBUF_NAMESPACE_CLOSE