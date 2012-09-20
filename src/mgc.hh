#include <node.h>
#include <node_buffer.h>

#define ERROR(type, msg) Exception::type(String::New(msg))
#define THROW(type, msg) ThrowException(ERROR(type, msg))
#define DEFINE_CONST(target, prefix, constant) (target)->Set(String::NewSymbol(#constant), Integer::New(prefix##constant))

namespace node{
  using namespace v8;
  
}
