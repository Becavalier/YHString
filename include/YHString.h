#ifndef YHSTRING
#define YHSTRING

#include <variant>
#include <iostream>
#include <algorithm>

constexpr int topThreshold = 16;
constexpr int bottomThreshold = 255;
constexpr const char* outOfRangeMsg = "The accessing index is out of range.";

class YHString {
  // forward-declaration.
  struct EagerCopyImpl;
  struct CopyOnWriteImpl;
  struct ShortStringOptImpl;
  using InnerImpls = std::variant<std::monostate, EagerCopyImpl, CopyOnWriteImpl, ShortStringOptImpl>;

  // friends.
  friend std::ostream& operator<<(std::ostream&, YHString&);
  friend bool isSharing(const YHString&, const YHString&);

  // implementations.
  struct EagerCopyImpl {
    size_t size;
    size_t capacity;
    char* start;
    EagerCopyImpl() = default;
    EagerCopyImpl(const char* str) : 
      size(std::strlen(str)), 
      capacity(size),
      start(static_cast<char*>(std::malloc(size))) {
        std::memcpy(start, str, size);
    }
    ~EagerCopyImpl() {
      if (start != nullptr) { std::free(start); }
    }
    EagerCopyImpl(const EagerCopyImpl& rhs) :
      size(rhs.size),
      capacity(rhs.capacity),
      start(static_cast<char*>(std::malloc(rhs.capacity))) {
        std::memcpy(start, rhs.start, capacity);
    }
    EagerCopyImpl& operator=(const EagerCopyImpl& rhs) {
      if (this != &rhs) {
        size = rhs.size;
        capacity = rhs.capacity;
        start = static_cast<char*>(std::malloc(rhs.capacity));
        std::memcpy(start, rhs.start, capacity);
      }
      return *this;
    }
    char* operator[](size_t index) {
      if (index < size) { return start + index; }
      else { throw std::out_of_range(outOfRangeMsg); }
    }
    auto length() const { return size; }
    const auto* data() const { return start; }
  };

  struct CopyOnWriteImpl {
    struct Resource {
      size_t size;
      size_t capacity;
      char* start;
      Resource(const char* str) : 
        size(std::strlen(str)),
        capacity(size),
        start(static_cast<char*>(std::malloc(size))) {
          std::memcpy(start, str, size);
      }
      ~Resource() { if (start != nullptr) { std::free(start); } }
    };
    std::shared_ptr<Resource> res;
    CopyOnWriteImpl() = default;
    CopyOnWriteImpl(const char* str) : res(std::make_shared<Resource>(str)) {}
    CopyOnWriteImpl(const CopyOnWriteImpl& rhs) : res(rhs.res) {}
    CopyOnWriteImpl& operator=(const CopyOnWriteImpl& rhs) {
      if (this != &rhs) { res = rhs.res; }
      return *this;
    }
    char* operator[](size_t index) {
      if (index < res->size) { 
        // copy.
        res = std::make_shared<Resource>(res->start);
        return res->start + index; 
      } else { throw std::out_of_range(outOfRangeMsg); }
    }
    auto length() const { return res->size; }
    const auto* data() const { return res->start; }
  };

  struct ShortStringOptImpl {
    size_t size;
    char buf[topThreshold];
  public:
    ShortStringOptImpl() = default;
    ShortStringOptImpl(const char* str) : 
      size(std::strlen(str)) {
        std::memcpy(buf, str, size);
    }
    ShortStringOptImpl(const ShortStringOptImpl& rhs) :
      size(rhs.size) {
        std::memcpy(buf, rhs.buf, size);
    }
    ShortStringOptImpl& operator=(const ShortStringOptImpl& rhs) {
      if (this != &rhs) {
        size = rhs.size;
        std::memcpy(buf, rhs.buf, size);
      }
      return *this;
    }
    char* operator[](size_t index) {
      if (index < size) { return buf + index; }
      else { throw std::out_of_range(outOfRangeMsg); }
    }
    auto length() const { return size; }
    const auto* data() const { return buf; }
  };

  InnerImpls v;
  // a little bit ugly here, anyway :(
  #define VISIT_COMMON_RET(retType, expr, ...) \
    [=]() { \
      retType _t; \
      std::visit([&_t, this, ##__VA_ARGS__](auto&& arg) { \
        using T = std::decay_t<decltype(arg)>; \
        if constexpr (!std::is_same_v<T, decltype(std::monostate())>) { \
          _t = expr; \
        } \
      }, v); \
      return _t; \
    }()
    
  void echo(std::ostream& os) {
    std::visit([&os](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, EagerCopyImpl>) {
        os << arg.start; 
      } else if constexpr (std::is_same_v<T, CopyOnWriteImpl>) {
        os << arg.res->start; 
      } else if constexpr (std::is_same_v<T, ShortStringOptImpl>) { 
        os << arg.buf; 
      }
    }, v);
  }
 public:
  YHString() = default;
  YHString(const char* str) {
    const auto len = std::strlen(str);
    // SSO should have dynamic capacity.
    if (len <= topThreshold) {
      v = InnerImpls(std::in_place_type<ShortStringOptImpl>, str); 
    } else if (len <= bottomThreshold) {
      v = InnerImpls(std::in_place_type<EagerCopyImpl>, str); 
    } else { 
      v = InnerImpls(std::in_place_type<CopyOnWriteImpl>, str); 
    }
  }
  YHString(const YHString& rhs) : v(rhs.v) {}
  YHString& operator=(const YHString& rhs) {
    if (v.index() != rhs.v.index()) {
      v = InnerImpls(rhs.v);  // re-copy-construct if they're different.
    } else {
      v = rhs.v; 
    }
    return *this;
  }
  const auto* data() const { 
    return VISIT_COMMON_RET(const char*, arg.data());
  }
  char& operator[](size_t index) {
    return *VISIT_COMMON_RET(char*, arg[index], index);
  }
  auto length() const { 
    return VISIT_COMMON_RET(size_t, arg.length());
  }
};

// global helpers / overloadings.
bool operator==(const YHString& lhs, const YHString& rhs) {
  bool result = true;
  if (&lhs != &rhs) {
    if (const auto llen = lhs.length(); llen == rhs.length()) {
      auto sx = lhs.data();
      auto sy = rhs.data();
      for (auto i = 0; i < llen; ++i) {
        if (sx[i] != sy[i]) { 
          result = false;
          break;
        }
      }
    } else {
      result = false;
    }
  }
  return result;
}
std::ostream& operator<<(std::ostream& os, YHString& rhs) { rhs.echo(os); return os; }
bool isSharing(const YHString& lhs, const YHString& rhs) {
  bool result = false;
  if (lhs.v.index() == rhs.v.index()) {
    std::visit([&result](auto&& argX, auto&& argY) {
      using T = std::decay_t<decltype(argX)>;
      using V = std::decay_t<decltype(argY)>;
      if constexpr (
        std::is_same_v<T, YHString::CopyOnWriteImpl> && 
        std::is_same_v<V, YHString::CopyOnWriteImpl>) {
          result = argX.res == argY.res;
      }
    }, lhs.v, rhs.v);
  }
  return result;
}

#endif
