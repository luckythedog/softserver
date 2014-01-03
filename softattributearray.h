#include <cstring>
#include <string>

template <class T> class softattributearray
{
  public:
    std::vector<T>value;
    std::string key;
    softattributearray(std::string _key, std::vector<T> _value) { key = _key; value = _value;};
};
