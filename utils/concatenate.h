#pragma once

#include <sstream>
#include <string>
#include <type_traits>

// Helper function to convert a single value to a string
template<typename T>
std::string ToString(T&& value) {
    std::stringstream ss;
    ss << std::boolalpha << std::forward<T>(value);
    return ss.str();
}

// Base case for the variadic function template
inline std::string Concatenate() {
    return "";
}

// Variadic function template to handle multiple arguments
template<typename T, typename... Args>
std::string Concatenate(T&& first, Args&&... args) {
    return ToString(std::forward<T>(first)) + Concatenate(std::forward<Args>(args)...);
}
