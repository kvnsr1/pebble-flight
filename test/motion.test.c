#include <assert.h>
#include <stdio.h>
#include "../src/c/prototype/motion.h"

int main(void) {
  // Exact final pixels, bounded overshoot, and monotonic direct selection.
  for (int distance=10; distance<=228; ++distance) {
    assert(motion_travel(distance,3,0)==0);
    assert(motion_travel(distance,3,1000)==distance);
    assert(motion_travel(distance,3,780)==distance+3);
    for (int p=0; p<=1000; ++p) {
      int n=motion_travel(distance,3,p);
      assert(n>=0 && n<=distance+3);
    }
  }
  int previous=0;
  for (int p=0; p<=1000; ++p) {
    int n=motion_mix(0,51,motion_ease(p));
    assert(n>=previous && n<=51);
    previous=n;
  }
  assert(motion_mix(228,0,0)==228);
  assert(motion_mix(228,0,1000)==0);
  assert(motion_mix(20,80,-1)==20);
  assert(motion_mix(20,80,1001)==80);
  assert(motion_ease(0)==0 && motion_ease(1000)==1000);
  puts("Motion geometry: exact endpoints, bounded settle, selection, clamping passed");
}
