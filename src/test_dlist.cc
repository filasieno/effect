
#include "dlist.hpp"
#include <cassert>
#include <print>

struct Data;

struct Data {
    DList node;
    int  value;
};

int main() {
  // This is a placeholder for the main function.
  // You can add your test cases or logic here.
  Data d1;
  d1.value = 100;
  d1.node.init();
  assert(d1.node.detached());

  Data d2;
  d2.value = 200;
  d2.node.init();
  assert(d2.node.detached());

  Data d3;
  d3.value = 300;
  d3.node.init();
  assert(d3.node.detached());

  d1.node.push_back(&d2.node);
  assert(!d1.node.detached());
  assert(!d2.node.detached());
  assert(d1.node.next == &d2.node);
  assert(d1.node.prev == &d2.node);
  assert(d2.node.prev == &d1.node); 
  assert(d2.node.next == &d1.node); 

  d2.node.push_back(&d3.node);
  assert(!d3.node.detached());
  assert(d3.node.prev == &d2.node);    
  assert(d3.node.next == &d1.node);  
  assert(d2.node.next == &d3.node);    
  assert(d1.node.prev == &d3.node);     
  
  std::printf("done\n");
  return 0;
}