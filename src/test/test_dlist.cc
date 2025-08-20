#define AK_IMPLEMENTATION
#include "ak.hpp" // IWYU pragma: keep
#include <cassert>
#include <print>

using namespace ak;
using namespace utl;

struct Data;

struct Data {
    utl::DLink node;
    int  value;
};

int main() {
  using namespace utl;
  
  Data d1;
  d1.value = 100;
  init_dlink(&d1.node);
  assert(is_dlink_detached(&d1.node));

  Data d2;
  d2.value = 200;
  init_dlink(&d2.node);
  assert(is_dlink_detached(&d2.node));

  Data d3;
  d3.value = 300;
  init_dlink(&d3.node);
  assert(is_dlink_detached(&d3.node));

  enqueue_dlink(&d1.node, &d2.node);
  assert(!is_dlink_detached(&d1.node));
  assert(!is_dlink_detached(&d2.node));
  assert(d1.node.next == &d2.node);
  assert(d1.node.prev == &d2.node);
  assert(d2.node.prev == &d1.node); 
  assert(d2.node.next == &d1.node); 

  enqueue_dlink(&d2.node, &d3.node);  
  assert(d3.node.prev == &d2.node);    
  assert(d3.node.next == &d1.node);  
  assert(d2.node.next == &d3.node);    
  assert(d1.node.prev == &d3.node);     
  
  std::print("done\n");
  return 0;
}