/**
 * @file StringFace.hpp
 * @author askryabin
 *
 */

#ifndef ZMQMESSAGE_EXAMPLES_STRINGFACE_HPP_
#define ZMQMESSAGE_EXAMPLES_STRINGFACE_HPP_

//this string-like class just wraps memory, not owns it.
class StringFace
{
public:
  const char*
  data() const
  {
    return p_;
  }

  size_t
  length() const
  {
    return len_;
  }

  StringFace()
      : p_(0), len_(0)
  {
  }

  StringFace(const char* p, size_t len)
      : p_(p), len_(len)
  {
  }

  bool
  operator==(const std::string& s)
  {
    return s.length() == len_ && !memcmp(p_, s.data(), len_);
  }

  bool
  operator==(const char* s)
  {
    return strlen(s) == len_ && !memcmp(p_, s, len_);
  }

private:
  const char* p_;
  size_t len_;
};

//used for alphanumeric sorting StringFace objects
int
compare(const StringFace& s1, const StringFace& s2)
{
  const size_t LEN = std::min(s1.length(), s2.length());
  if (const int RESULT = ::memcmp(s1.data(), s2.data(), LEN))
  {
    return RESULT;
  }
  return s1.length() == s2.length() ? 0 : (s1.length() < s2.length() ? -1 : 1);
}

bool
operator <(const StringFace& s1, const StringFace& s2)
{
  return compare(s1, s2) < 0;
}

#endif /* ZMQMESSAGE_EXAMPLES_STRINGFACE_HPP_ */
