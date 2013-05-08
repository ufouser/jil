#ifndef JIL_EDITOR_BUFFER_LISTENER_H_
#define JIL_EDITOR_BUFFER_LISTENER_H_
#pragma once

#include "boost/any.hpp"
#include "editor/text_range.h"

namespace jil {
namespace editor {

enum ChangeBit {
  kLineUpdatedBit = 0,
  kLineAddedBit,
  kLineDeleteBit,
};

// Text buffer change types.
enum ChangeType {
  kLineUpdated    = 1 << kLineUpdatedBit,
  kLineAdded      = 1 << kLineAddedBit,
  // Note: The line number in the change data will be invalid.
  kLineDeleted    = 1 << kLineDeleteBit,
};

typedef int ChangeTypes;

class ChangeData : public LineRange {
public:
  // Single line.
  ChangeData(Coord first_line, Coord last_line = 0)
      : LineRange(first_line, last_line) {
  }

  const boost::any& extra() const { return extra_; }

  ChangeData& set_extra(const boost::any& extra) {
    extra_ = extra;
    return *this;
  }

private:
  boost::any extra_;
};

// Implement this interface and attach to a text buffer to listen to the changes of it.
class BufferListener {
public:
  virtual ~BufferListener() {
  }

  virtual void OnBufferChange(ChangeType type, const ChangeData& data) = 0;

  // TODO: Add new change types.
  virtual void OnBufferEncodingChange() = 0;
  virtual void OnBufferFileNameChange() = 0;
  virtual void OnBufferSaved() = 0;
};

} } // namespace jil::editor

#endif // JIL_EDITOR_BUFFER_LISTENER_H_
