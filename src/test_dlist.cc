
#include "task.hpp"
#include <cassert>
#include <print>


struct Data;

using namespace internal_ak;

struct Data {
    Link node;
    int  value;
};

int main() {
  // This is a placeholder for the main function.
  // You can add your test cases or logic here.
  Data d1;
  d1.value = 100;
  InitLink(&d1.node);
  assert(IsLinkDetached(&d1.node));

  Data d2;
  d2.value = 200;
  InitLink(&d2.node);
  assert(IsLinkDetached(&d2.node));

  Data d3;
  d3.value = 300;
  InitLink(&d3.node);
  assert(IsLinkDetached(&d3.node));

  EnqueueLink(&d1.node, &d2.node);
  assert(!IsLinkDetached(&d1.node));
  assert(!IsLinkDetached(&d2.node));
  assert(d1.node.next == &d2.node);
  assert(d1.node.prev == &d2.node);
  assert(d2.node.prev == &d1.node); 
  assert(d2.node.next == &d1.node); 

  EnqueueLink(&d2.node, &d3.node);  
  assert(d3.node.prev == &d2.node);    
  assert(d3.node.next == &d1.node);  
  assert(d2.node.next == &d3.node);    
  assert(d1.node.prev == &d3.node);     
  
  std::print("done\n");
  return 0;
}