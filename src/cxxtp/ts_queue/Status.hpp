#pragma once

#include <optional>

namespace cxxtp::ts_queue {

enum TransStatus { TS_DONE, TS_RACE, TS_FULL, TS_EMPTY, TS_UNKNOWN };

template <class Elem>
struct TransRes : public std::optional<Elem> {
  TransRes() : std::optional<Elem>(), status(TS_UNKNOWN) {}

  TransRes(Elem&& e, TransStatus s = TS_UNKNOWN)
      : std::optional<Elem>(std::move(e)), status(s) {}

  TransRes(std::nullopt_t, TransStatus s = TS_UNKNOWN)
      : std::optional<Elem>(std::nullopt), status(s) {}

  TransRes(TransStatus s)
      : std::optional<Elem>(std::nullopt), status(s) {}

  TransStatus status;
};

}  // namespace cxxtp::ts_queue
