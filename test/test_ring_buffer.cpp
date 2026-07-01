// =============================================================================
//  Test unitaire du buffer circulaire (historique de mesures).
//     g++ -std=c++11 test/test_ring_buffer.cpp -o /tmp/test_ring && /tmp/test_ring
// =============================================================================
#include <cstdio>
#include "../src/ring_buffer.h"

static int fails = 0;
static void check(bool cond, const char* msg) {
  printf("[%s] %s\n", cond ? "OK  " : "FAIL", msg);
  if (!cond) { fails++; }
}

int main() {
  RingBuffer<float, 4> rb;
  check(rb.size() == 0U, "vide au depart");
  check(rb.capacity() == 4U, "capacite = 4");

  rb.push(1.0f); rb.push(2.0f); rb.push(3.0f);
  check(rb.size() == 3U, "taille = 3 apres 3 push");
  check(rb.last() == 3.0f, "dernier = 3");
  check(rb.at(0) == 1.0f, "plus ancien = 1");

  rb.push(4.0f); rb.push(5.0f);            // depassement de capacite (garde 2,3,4,5)
  check(rb.size() == 4U, "taille plafonnee a la capacite");
  check(rb.full(), "plein");
  check(rb.last() == 5.0f, "dernier = 5 apres overflow");
  check(rb.at(0) == 2.0f, "plus ancien = 2 apres overflow (1 ecrase)");

  printf("\n%s (%d echec(s))\n",
         fails == 0 ? "== TOUS LES TESTS PASSENT ==" : "== DES TESTS ECHOUENT ==", fails);
  return fails == 0 ? 0 : 1;
}
