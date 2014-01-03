#include <cstring>
#include <string>
#include <vector>

template <class T> class softattribute
{
  public:
    T value;
    std::string key;
    softattribute(std::string _key, T _value) { key = _key; value = _value;};

};
