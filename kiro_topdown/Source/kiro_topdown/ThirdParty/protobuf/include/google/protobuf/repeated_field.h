// Simplified protobuf repeated field header
#pragma once

#include <vector>

PROTOBUF_NAMESPACE_OPEN

template<typename Element>
class RepeatedPtrField {
private:
    std::vector<Element*> elements_;

public:
    RepeatedPtrField() = default;
    explicit RepeatedPtrField(void* arena) {}
    
    ~RepeatedPtrField() {
        Clear();
    }
    
    int size() const { return static_cast<int>(elements_.size()); }
    
    Element* Add() {
        Element* element = new Element();
        elements_.push_back(element);
        return element;
    }
    
    const Element& Get(int index) const {
        return *elements_[index];
    }
    
    Element* Mutable(int index) {
        return elements_[index];
    }
    
    void Clear() {
        for (Element* element : elements_) {
            delete element;
        }
        elements_.clear();
    }
};

PROTOBUF_NAMESPACE_CLOSE