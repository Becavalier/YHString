#include <iostream>
#include <variant>

constexpr int topThreshold = 23;
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
    ~EagerCopyImpl() { if (start != nullptr) { std::free(start); } }
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
  };

  struct ShortStringOptImpl {
    const static size_t bufSize = 3 * sizeof(nullptr);
    char buf[bufSize];
    size_t size;
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
      else throw std::out_of_range(outOfRangeMsg);
    }
  };

  InnerImpls v;
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
    if (v.index() != rhs.v.index()) { v = InnerImpls(rhs.v); }  // re-copy-construct if they're different.
    else { v = rhs.v; }
    return *this;
  }
  char& operator[](size_t index) {
    char* t = nullptr;
    std::visit([&t ,index = index, this](auto&& arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (!std::is_same_v<T, decltype(std::monostate())>) {
        t = arg[index];
      }
    }, v);
    return *t;
  }
};

// global helper / overloading.
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

int main(int argc, char **argv) {
  std::cout << std::boolalpha;
  
  // SSO: basic constructing, copy-constructing, and assignment.
  YHString strA("Hello, world!"), strB(strA), strC;
  strC = strA;
  std::cout << strA << std::endl;
  std::cout << strB << std::endl;
  std::cout << strC << std::endl;

  // COW: sharing dynamic memory; 
  YHString strD(R"(
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY 
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
    YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY YHSPY
  )");
  YHString strE(strD);
  std::cout << isSharing(strD, strE) << std::endl;  // true.
  
  // COW: sharing assignment.
  strC = strD;
  std::cout << strC << std::endl;
  std::cout << isSharing(strC, strE) << std::endl;  // true.
  
  // COW: non-sharing assignment.
  YHString strF(R"(
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO 
    HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO HELLO
  )");

  strC = strF;
  std::cout << strC << std::endl;
  std::cout << isSharing(strC, strE) << std::endl;  // false.
  
  // COW: non-sharing operator[].
  strD[255] = 'c';
  std::cout << strD << std::endl;
  std::cout << isSharing(strD, strE) << std::endl;  // false.

  // eager-copy: constructing.
  YHString strG = "Hello, there are still many things to do.";
  std::cout << strG << std::endl;

  // eager-copy: out-of-bound exception thrown;
  strG[225] = 'c';
  return 0;
}
