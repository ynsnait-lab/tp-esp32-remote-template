#pragma once
#include <cstddef>
// =============================================================================
//  Buffer circulaire generique (taille fixe), PUR -> testable sur PC.
//  Garde les N derniers echantillons ; ecrase le plus ancien quand plein.
//  Sert a memoriser un historique de mesures (log) sans allocation dynamique.
// =============================================================================
template <typename T, size_t CAP>
class RingBuffer {
public:
  void push(T value) {
    buf_[head_] = value;
    head_ = (head_ + 1U) % CAP;
    if (count_ < CAP) { count_++; }
  }

  size_t size() const     { return count_; }
  size_t capacity() const { return CAP; }
  bool   full() const     { return count_ == CAP; }

  // Element i, du plus ancien (0) au plus recent (size()-1).
  T at(size_t i) const {
    size_t start = (head_ + CAP - count_) % CAP;
    return buf_[(start + i) % CAP];
  }

  // Dernier element insere (comportement indefini si vide).
  T last() const { return buf_[(head_ + CAP - 1U) % CAP]; }

private:
  T      buf_[CAP] = {};
  size_t head_  = 0U;
  size_t count_ = 0U;
};
