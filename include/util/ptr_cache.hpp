#pragma once
#include <functional>
#include <unordered_map>
#include <memory>

template<class Key, 
         class Val,
         class Hash = std::hash<Key>>
struct ptr_cache
    : std::unordered_map
        <Key, std::weak_ptr<Val>, Hash>
{
    using base = std::unordered_map
        <Key, std::weak_ptr<Val>, Hash>;

    using creator_fn = std::function<std::unique_ptr<Val>(Key)>;
    using deleter_fn = std::function<void(Val*)>;
    
    creator_fn creator;
    deleter_fn deleter;
    
    explicit
    ptr_cache(creator_fn pcreator, 
              deleter_fn pdeleter)
        : creator(pcreator)
        , deleter(pdeleter)
    {}
    
    ptr_cache(const ptr_cache& other) = delete;
    ptr_cache& operator=(const ptr_cache& other) = delete;
    ptr_cache(ptr_cache&& other) noexcept = delete;
    ptr_cache& operator=(ptr_cache&& other) noexcept = delete;

    auto operator[](Key key)
    {
        auto& weak = base::operator[](key);
        if (auto strong = weak.lock())
            return strong;
        
        std::shared_ptr<Val> strong( 
            creator(key).release(),
            [&](Val* ptr){ 
                deleter(ptr);
                this->erase(key);
            });

        weak = strong;
        return strong;
    }
};

template<class Key, class Val, class Hash = std::hash<Key>>
struct basic_cache : ptr_cache<Key, Val, Hash>
{
    using base = ptr_cache<Key, Val, Hash>;
    
    basic_cache()
        : base(
            [&](auto&&) 
            { return make_unique<Val>(); }, 
            [](Val* ptr)
            { delete ptr; })
    {}
};