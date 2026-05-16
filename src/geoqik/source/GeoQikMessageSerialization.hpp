#ifndef GEOQIKMESSAGESERIALIZATION_HPP
#define GEOQIKMESSAGESERIALIZATION_HPP

#include "GeoQikMessages.hpp"
#include <iosfwd>

namespace geoqik
{

class MessageWriter
{
public:
  explicit MessageWriter(std::ostream& stream);

  void write(const GeoQikLogEntry& message);

private:
  std::ostream& m_stream;
};

class MessageReader
{
public:
  explicit MessageReader(std::istream& stream);

  [[nodiscard]] GeoQikLogEntry read();

private:
  std::istream& m_stream;
};

} // namespace geoqik

#endif // GEOQIKMESSAGESERIALIZATION_HPP
