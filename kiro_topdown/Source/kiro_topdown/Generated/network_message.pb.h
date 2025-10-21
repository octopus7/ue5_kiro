// Simplified protobuf implementation for UE integration
// Generated from network_message.proto

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace simpletcp {

// Message type enum
enum MessageType : int {
    MOVE = 0,
    POSITION = 1,
    USER_ID_ASSIGNMENT = 2,
    ALL_USERS_INFO = 3
};

// Forward declaration
class NetworkMessage;

// UserInfo class - simplified protobuf-like implementation
class UserInfo {
public:
    UserInfo() = default;
    ~UserInfo() = default;
    
    // Copy constructor and assignment
    UserInfo(const UserInfo& other) = default;
    UserInfo& operator=(const UserInfo& other) = default;
    
    // Accessors for user_id
    uint32_t user_id() const { return user_id_; }
    void set_user_id(uint32_t value) { user_id_ = value; }
    void clear_user_id() { user_id_ = 0; }
    
    // Accessors for character_id
    const std::string& character_id() const { return character_id_; }
    void set_character_id(const std::string& value) { character_id_ = value; }
    std::string* mutable_character_id() { return &character_id_; }
    void clear_character_id() { character_id_.clear(); }
    
    // Accessors for current_x
    float current_x() const { return current_x_; }
    void set_current_x(float value) { current_x_ = value; }
    void clear_current_x() { current_x_ = 0.0f; }
    
    // Accessors for current_y
    float current_y() const { return current_y_; }
    void set_current_y(float value) { current_y_ = value; }
    void clear_current_y() { current_y_ = 0.0f; }
    
    // Accessors for target_x
    float target_x() const { return target_x_; }
    void set_target_x(float value) { target_x_ = value; }
    void clear_target_x() { target_x_ = 0.0f; }
    
    // Accessors for target_y
    float target_y() const { return target_y_; }
    void set_target_y(float value) { target_y_ = value; }
    void clear_target_y() { target_y_ = 0.0f; }
    
    // Accessors for speed
    float speed() const { return speed_; }
    void set_speed(float value) { speed_ = value; }
    void clear_speed() { speed_ = 0.0f; }
    
    // Accessors for is_moving
    bool is_moving() const { return is_moving_; }
    void set_is_moving(bool value) { is_moving_ = value; }
    void clear_is_moving() { is_moving_ = false; }
    
    // Utility methods
    void Clear() {
        user_id_ = 0;
        character_id_.clear();
        current_x_ = 0.0f;
        current_y_ = 0.0f;
        target_x_ = 0.0f;
        target_y_ = 0.0f;
        speed_ = 0.0f;
        is_moving_ = false;
    }
    
    void CopyFrom(const UserInfo& other) {
        *this = other;
    }
    
    void MergeFrom(const UserInfo& other) {
        if (other.user_id_ != 0) user_id_ = other.user_id_;
        if (!other.character_id_.empty()) character_id_ = other.character_id_;
        if (other.current_x_ != 0.0f) current_x_ = other.current_x_;
        if (other.current_y_ != 0.0f) current_y_ = other.current_y_;
        if (other.target_x_ != 0.0f) target_x_ = other.target_x_;
        if (other.target_y_ != 0.0f) target_y_ = other.target_y_;
        if (other.speed_ != 0.0f) speed_ = other.speed_;
        if (other.is_moving_) is_moving_ = other.is_moving_;
    }
    
    bool IsInitialized() const { return true; }
    size_t ByteSizeLong() const { return 0; } // Simplified
    
private:
    uint32_t user_id_ = 0;
    std::string character_id_;
    float current_x_ = 0.0f;
    float current_y_ = 0.0f;
    float target_x_ = 0.0f;
    float target_y_ = 0.0f;
    float speed_ = 0.0f;
    bool is_moving_ = false;
};

// NetworkMessage class - simplified protobuf-like implementation
class NetworkMessage {
public:
    NetworkMessage() = default;
    ~NetworkMessage() = default;
    
    // Copy constructor and assignment
    NetworkMessage(const NetworkMessage& other) = default;
    NetworkMessage& operator=(const NetworkMessage& other) = default;
    
    // Accessors for type
    MessageType type() const { return type_; }
    void set_type(MessageType value) { type_ = value; }
    void clear_type() { type_ = MOVE; }
    
    // Accessors for character_id
    const std::string& character_id() const { return character_id_; }
    void set_character_id(const std::string& value) { character_id_ = value; }
    std::string* mutable_character_id() { return &character_id_; }
    void clear_character_id() { character_id_.clear(); }
    
    // Accessors for user_id
    uint32_t user_id() const { return user_id_; }
    void set_user_id(uint32_t value) { user_id_ = value; }
    void clear_user_id() { user_id_ = 0; }
    
    // Accessors for start_x
    float start_x() const { return start_x_; }
    void set_start_x(float value) { start_x_ = value; }
    void clear_start_x() { start_x_ = 0.0f; }
    
    // Accessors for start_y
    float start_y() const { return start_y_; }
    void set_start_y(float value) { start_y_ = value; }
    void clear_start_y() { start_y_ = 0.0f; }
    
    // Accessors for target_x
    float target_x() const { return target_x_; }
    void set_target_x(float value) { target_x_ = value; }
    void clear_target_x() { target_x_ = 0.0f; }
    
    // Accessors for target_y
    float target_y() const { return target_y_; }
    void set_target_y(float value) { target_y_ = value; }
    void clear_target_y() { target_y_ = 0.0f; }
    
    // Accessors for current_x
    float current_x() const { return current_x_; }
    void set_current_x(float value) { current_x_ = value; }
    void clear_current_x() { current_x_ = 0.0f; }
    
    // Accessors for current_y
    float current_y() const { return current_y_; }
    void set_current_y(float value) { current_y_ = value; }
    void clear_current_y() { current_y_ = 0.0f; }
    
    // Accessors for speed
    float speed() const { return speed_; }
    void set_speed(float value) { speed_ = value; }
    void clear_speed() { speed_ = 0.0f; }
    
    // Accessors for is_moving
    bool is_moving() const { return is_moving_; }
    void set_is_moving(bool value) { is_moving_ = value; }
    void clear_is_moving() { is_moving_ = false; }
    
    // Accessors for all_users (repeated field)
    int all_users_size() const { return static_cast<int>(all_users_.size()); }
    const UserInfo& all_users(int index) const { return all_users_[index]; }
    UserInfo* mutable_all_users(int index) { return &all_users_[index]; }
    UserInfo* add_all_users() { 
        all_users_.emplace_back();
        return &all_users_.back();
    }
    void clear_all_users() { all_users_.clear(); }
    const std::vector<UserInfo>& all_users() const { return all_users_; }
    std::vector<UserInfo>* mutable_all_users() { return &all_users_; }
    
    // Utility methods
    void Clear() {
        type_ = MOVE;
        character_id_.clear();
        user_id_ = 0;
        start_x_ = 0.0f;
        start_y_ = 0.0f;
        target_x_ = 0.0f;
        target_y_ = 0.0f;
        current_x_ = 0.0f;
        current_y_ = 0.0f;
        speed_ = 0.0f;
        is_moving_ = false;
        all_users_.clear();
    }
    
    void CopyFrom(const NetworkMessage& other) {
        *this = other;
    }
    
    void MergeFrom(const NetworkMessage& other) {
        if (other.type_ != MOVE) type_ = other.type_;
        if (!other.character_id_.empty()) character_id_ = other.character_id_;
        if (other.user_id_ != 0) user_id_ = other.user_id_;
        if (other.start_x_ != 0.0f) start_x_ = other.start_x_;
        if (other.start_y_ != 0.0f) start_y_ = other.start_y_;
        if (other.target_x_ != 0.0f) target_x_ = other.target_x_;
        if (other.target_y_ != 0.0f) target_y_ = other.target_y_;
        if (other.current_x_ != 0.0f) current_x_ = other.current_x_;
        if (other.current_y_ != 0.0f) current_y_ = other.current_y_;
        if (other.speed_ != 0.0f) speed_ = other.speed_;
        if (other.is_moving_) is_moving_ = other.is_moving_;
        if (!other.all_users_.empty()) {
            all_users_.insert(all_users_.end(), other.all_users_.begin(), other.all_users_.end());
        }
    }
    
    bool IsInitialized() const { return true; }
    size_t ByteSizeLong() const { return 0; } // Simplified
    
private:
    MessageType type_ = MOVE;
    std::string character_id_;
    uint32_t user_id_ = 0;
    float start_x_ = 0.0f;
    float start_y_ = 0.0f;
    float target_x_ = 0.0f;
    float target_y_ = 0.0f;
    float current_x_ = 0.0f;
    float current_y_ = 0.0f;
    float speed_ = 0.0f;
    bool is_moving_ = false;
    std::vector<UserInfo> all_users_;
};

} // namespace simpletcp