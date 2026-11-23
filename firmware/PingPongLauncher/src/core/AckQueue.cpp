#include "AckQueue.h"

#include <ArduinoJson.h>

void AckQueue::push(const char* id, CommandResult result, uint32_t rev) {
  if (id == nullptr || id[0] == '\0') {
    return;
  }
  if (count_ == CAPACITY) {
    pop();
  }
  Entry& entry = entries_[(head_ + count_) % CAPACITY];
  strncpy(entry.id, id, ID_LEN);
  entry.id[ID_LEN] = '\0';
  entry.result = result;
  entry.rev = rev;
  count_++;
}

bool AckQueue::peek(uint32_t bootId, char* out, size_t capacity) const {
  if (count_ == 0) {
    return false;
  }
  const Entry& entry = entries_[head_];
  JsonDocument doc;
  doc["id"] = entry.id;
  doc["rev"] = entry.rev;
  doc["boot"] = bootId;
  doc["result"] = resultName(entry.result);
  serializeJson(doc, out, capacity);
  return true;
}

void AckQueue::pop() {
  if (count_ == 0) {
    return;
  }
  head_ = (head_ + 1) % CAPACITY;
  count_--;
}

const char* AckQueue::resultName(CommandResult result) {
  switch (result) {
    case CommandResult::Changed:
      return "changed";
    case CommandResult::Duplicate:
      return "duplicate";
    case CommandResult::StateRequested:
      return "state";
    case CommandResult::OtaRequested:
      return "ota";
    case CommandResult::Invalid:
      return "invalid";
    default:
      return "unchanged";
  }
}
