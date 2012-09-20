#include "mgc.hh"

#include <queue>

extern "C" {
#include <string.h>
#include <stdlib.h>
#include "magic.h"
}

namespace node{
  using namespace std;
  using namespace v8;
  
  class Magic: public ObjectWrap {
  protected:
    class Baton {
    private:
      uv_work_t req;
    protected:
      Magic *magic;
      Persistent<Function> cb;
    public:
      Baton(Magic *magic_,
            Handle<Function> cb_){
        magic = magic_;
        req.data = this;
        if(cb_->IsFunction()){
          cb = Persistent<Function>::New(cb_);
        }
      }
      virtual
      ~Baton(){
        cb.Dispose();
      }
      void
      exec(){
        uv_queue_work(uv_default_loop(), &req, Run, End);
      }
    protected:
      void
      ret(unsigned argc, Handle<Value> argv[]){
        if(cb->IsFunction()){
          TryCatch try_catch;
          cb->Call(Context::GetCurrent()->Global(), argc, argv);
          if(try_catch.HasCaught()){
            node::FatalException(try_catch);
          }
        }
      }
      virtual void
      run() = 0;
      virtual void
      end(){
        const unsigned argc = 1;
        Local<Value> argv[argc] = {
          Local<Value>::New(Null())
        };
        ret(argc, argv);
      }
    private:
      void
      fin(){
        HandleScope scope;
        const char* error = magic_error(magic->cookie);
        if(error){
          const unsigned argc = 1;
          Local<Value> argv[argc] = {
            Local<Value>::New(ERROR(Error, error))
          };
          ret(argc, argv);
        }else{
          end();
        }
      }
    public:
      static void
      Run(uv_work_t *req){
        Baton *baton = static_cast<Baton*>(req->data);
        baton->run();
      }
      static void
      End(uv_work_t *req){
        Baton *baton = static_cast<Baton*>(req->data);
        baton->fin();
        delete baton;
      }
    };
  private:
    magic_t cookie;
    bool run;
    queue<Baton*> que;
    void deque(){
      if(!run && que.size() > 0){
        que.front()->exec();
        que.pop();
      }
    }
    void toque(Baton *baton){
      que.push(baton);
      deque();
    }
  public:
    Magic(int flags){
      run = false;
      HandleScope scope;
      if((cookie = magic_open(flags)) == NULL){
        THROW(Error, uv_strerror(uv_last_error(uv_default_loop())));
      }
    }
    ~Magic() {
      que.empty();
      if(cookie != NULL){
        magic_close(cookie);
      }
    }
  protected:
    class LoadBaton: public Baton {
    private:
      Persistent<String> filename;
    public:
      LoadBaton(Magic *magic_,
                Handle<String> filename_,
                Handle<Function> cb_): Baton(magic_, cb_) {
        if(filename_->IsString()){
          filename = Persistent<String>::New(filename_);
        }
      }
      virtual
      ~LoadBaton(){
        filename.Dispose();
      }
    protected:
      void
      run(){
        String::Utf8Value str(filename);
        const char* filename_ = NULL;
        if(filename->IsString()){
          filename_ = *str;
        }
        magic_load(magic->cookie, filename_);
      }
    };
  public:
    static Handle<Value> LoadAsync(const Arguments& args){
      Magic* magic = ObjectWrap::Unwrap<Magic>(args.This());
      
      if(args.Length() == 2 && args[0]->IsString() && args[1]->IsFunction()){
        magic->toque(new LoadBaton(magic,
                                   args[0]->ToString(),
                                   Handle<Function>::Cast(args[1])));
        return Undefined();
      }else if(args.Length() == 1 && args[0]->IsString()){
        magic->toque(new LoadBaton(magic,
                                   args[0]->ToString(),
                                   Handle<Function>::Cast(Undefined())));
        return Undefined();
      }else if(args.Length() == 1 && args[0]->IsFunction()){
        magic->toque(new LoadBaton(magic,
                                   Handle<String>::Cast(Undefined()),
                                   Handle<Function>::Cast(args[0])));
        return Undefined();
      }
      return THROW(TypeError, "Bad arguments");
    }
    
  protected:
    class BufferBaton: public Baton {
    private:
      Persistent<Object> buffer;
      const char* result;
    public:
      BufferBaton(Magic *magic_,
                  Handle<Object> buffer_,
                  Handle<Function> cb_): Baton(magic_, cb_){
        result = NULL;
        buffer = Persistent<Object>::New(buffer_);
      }
      virtual
      ~BufferBaton(){
        buffer.Dispose();
      }
    protected:
      void
      run(){
        result = magic_buffer(magic->cookie,
                              Buffer::Data(buffer),
                              Buffer::Length(buffer));
      }
      void
      end(){
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
          Local<Value>::New(Null()),
          Local<Value>::New(String::New(result))
        };
        ret(argc, argv);
      }
    };
    
  public:
    static Handle<Value> BufferAsync(const Arguments& args) {
      HandleScope scope;
      Magic* magic = ObjectWrap::Unwrap<Magic>(args.This());
      
      if(args.Length() < 2){
        return THROW(TypeError, "Two arguments required");
      }
      if(!Buffer::HasInstance(args[0])){
        return THROW(TypeError, "Buffer as first argument required");
      }
      if(!args[1]->IsFunction()){
        return THROW(TypeError, "Callback function as second argument required");
      }
      
      magic->toque(new BufferBaton(magic,
                                   Handle<Object>::Cast(args[0]),
                                   Handle<Function>::Cast(args[1])));
      
      return Undefined();
    }
    
  public:
    static Handle<Value> New(const Arguments& args){
      HandleScope scope;

      if(!args.IsConstructCall()){
        return args.Callee()->NewInstance();
      }
      
      int flags = MAGIC_NONE;
      
      if(args.Length() == 1 && args[1]->IsInt32()){
        flags = args[1]->Int32Value();
      }else{
        return THROW(TypeError, "Argument must be an integer");
      }
      
      Magic* obj = new Magic(flags);
      obj->Wrap(args.This());
      
      return args.This();
    }
    
    static const Persistent<FunctionTemplate> tpl;
    
    static void
    Initialize(const Handle<Object> target){
      HandleScope scope;
      Local<String> name = String::NewSymbol("MGC");
      
      tpl->SetClassName(name);
      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      
      DEFINE_CONST(tpl, MAGIC_, NONE);               /* No flags */
      DEFINE_CONST(tpl, MAGIC_, DEBUG);              /* Turn on debugging */
      //DEFINE_CONST(tpl, MAGIC_, SYMLINK);            /* Follow symlinks */
      //DEFINE_CONST(tpl, MAGIC_, COMPRESS);           /* Check inside compressed files */
      //DEFINE_CONST(tpl, MAGIC_, DEVICES);            /* Look at the contents of devices */
      DEFINE_CONST(tpl, MAGIC_, MIME_TYPE);          /* Return the MIME type */
      DEFINE_CONST(tpl, MAGIC_, CONTINUE);           /* Return all matches */
      //DEFINE_CONST(tpl, MAGIC_, CHECK);              /* Print warnings to stderr */
      //DEFINE_CONST(tpl, MAGIC_, PRESERVE_ATIME);     /* Restore access time on exit */
      DEFINE_CONST(tpl, MAGIC_, RAW);                /* Don't translate unprintable chars */
      DEFINE_CONST(tpl, MAGIC_, ERROR);              /* Handle ENOENT etc as real errors */
      DEFINE_CONST(tpl, MAGIC_, MIME_ENCODING);      /* Return the MIME encoding */
      DEFINE_CONST(tpl, MAGIC_, MIME);              
      //DEFINE_CONST(tpl, MAGIC_, APPLE);              /* Return the Apple creator and type */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_COMPRESS);  /* Don't check for compressed files */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_TAR);       /* Don't check for tar files */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_SOFT);      /* Don't check magic entries */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_APPTYPE);   /* Don't check application type */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_ELF);       /* Don't check for elf details */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_TEXT);      /* Don't check for text files */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_CDF);       /* Don't check for cdf files */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_TOKENS);    /* Don't check tokens */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_ENCODING);  /* Don't check text encodings */
      
      /* Defined for backwards compatibility (renamed) */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_ASCII);
      
      /* Defined for backwards compatibility; do nothing */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_FORTRAN);   /* Don't check ascii/fortran */
      DEFINE_CONST(tpl, MAGIC_, NO_CHECK_TROFF);     /* Don't check ascii/troff */
      
      NODE_SET_PROTOTYPE_METHOD(tpl, "load", LoadAsync);
      NODE_SET_PROTOTYPE_METHOD(tpl, "buffer", BufferAsync);
      
      target->Set(name, tpl->GetFunction());
    }
  };
  
  const Persistent<FunctionTemplate> Magic::tpl = Persistent<FunctionTemplate>::New(FunctionTemplate::New(New));
  
  extern "C" {
    void init(Handle<Object> target){
      HandleScope scope;
      Magic::Initialize(target);
    }
    
    NODE_MODULE(mgc, init);
  }
}
