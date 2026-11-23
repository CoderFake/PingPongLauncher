#pragma once

#include <Arduino.h>

#include "StateCodec.h"

/**
 * Small FIFO of command acknowledgements. Acks are queued from the MQTT
 * callback and published from the main loop, so the client buffer is never
 * reused while a message is still being handled.
 */
class AckQueue {
 public:
  /**
   * Remembers that command `id` was processed with `result` at state `rev`.
   */
  void push(const char* id, CommandResult result, uint32_t rev);

  /**
   * Writes the oldest ack as JSON into `out`; returns false when empty.
   */
  bool peek(uint32_t bootId, char* out, size_t capacity) const;

  /**
   * Drops the oldest ack after it was published.
   */
  void pop();

 private:
  static constexpr uint8_t CAPACITY = 8;
  static constexpr uint8_t ID_LEN = 24;

  struct Entry {
    char id[ID_LEN + 1];
    CommandResult result;
    uint32_t rev;
  };

  static const char* resultName(CommandResult result);

  Entry entries_[CAPACITY] = {};
  uint8_t head_ = 0;
  uint8_t count_ = 0;
};
