#include "mgc.hh"

#include <string>
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
        magic->run = true;
        uv_queue_work(uv_default_loop(), &req, Run, End);
      }
    protected:
      void
      ret(unsigned argc, Handle<Value> argv[]){
        if(!cb.IsEmpty()){
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
        magic->run = false;
        magic->deque();
        
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
      String::Utf8Value *filename;
    public:
      LoadBaton(Magic *magic_,
                Handle<Value> filename_,
                Handle<Function> cb_): Baton(magic_, cb_) {
        if(filename_->IsString()){
          filename = new String::Utf8Value(filename_);
        }else{
          filename = NULL;
        }
      }
      virtual
      ~LoadBaton(){
        if(filename){
          delete filename;
        }
      }
    protected:
      void
      run(){
        magic_load(magic->cookie, filename ? **filename : NULL);
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
                                   Undefined(),
                                   Handle<Function>::Cast(args[0])));
        return Undefined();
      }
      return THROW(TypeError, "Bad arguments");
    }
    
  protected:
    class DataBaton: public Baton {
    private:
      Persistent<Object> data; /* buffer or string argument */
      String::Utf8Value *str; /* raw string data for string argument */
      const void* raw; /* raw data */
      unsigned len; /* data length in bytes */
      string result; /* result */
    public:
      DataBaton(Magic *magic_,
                Handle<Object> data_,
                Handle<Function> cb_): Baton(magic_, cb_) {
        data = Persistent<Object>::New(data_);
        if(data->IsString()){
          str = new String::Utf8Value(data_);
          raw = **str;
          len = str->length();
        }else{
          str = NULL;
          raw = Buffer::Data(data);
          len = Buffer::Length(data);
        }
      }
      virtual
      ~DataBaton(){
        if(str){
          delete str;
        }
        data.Dispose();
      }
    protected:
      void
      run(){
        result = magic_buffer(magic->cookie, raw, len);
      }
      void
      end(){
        const unsigned argc = 2;
        Local<Value> argv[argc] = {
          Local<Value>::New(Null()),
          Local<Value>::New(String::New(result.data()))
        };
        ret(argc, argv);
      }
    };
    
  public:
    static Handle<Value> DataAsync(const Arguments& args) {
      HandleScope scope;
      Magic* magic = ObjectWrap::Unwrap<Magic>(args.This());
      
      if(args.Length() < 2){
        return THROW(TypeError, "Two arguments required");
      }
      if(!args[0]->IsString() && !Buffer::HasInstance(args[0])){
        return THROW(TypeError, "Buffer or String as first argument required");
      }
      if(!args[1]->IsFunction()){
        return THROW(TypeError, "Callback function as second argument required");
      }
      
      magic->toque(new DataBaton(magic,
                                 Handle<Object>::Cast(args[0]),
                                 Handle<Function>::Cast(args[1])));
      
      return Undefined();
    }
    
  public:
    static Handle<Value> New(const Arguments& args){
      HandleScope scope;
      Local<Object> self = args.IsConstructCall() ? args.This() : args.Callee()->NewInstance();
      int flags = MAGIC_NONE;
      
      if(args.Length()){
        if(args.Length() == 1 && args[0]->IsInt32()){
          flags = args[0]->Int32Value();
        }else{
          return THROW(TypeError, "Argument must be an integer");
        }
      }
      
      Magic* obj = new Magic(flags);
      obj->Wrap(self);
      
      return self;
    }
    
    static const Persistent<FunctionTemplate> tpl;
    
    static void
    Initialize(const Handle<Object> target){
      HandleScope scope;
      Local<String> name = String::NewSymbol("MGC");
      
      tpl->SetClassName(name);
      tpl->InstanceTemplate()->SetInternalFieldCount(1);
      
      DEFINE_CONST(target, MAGIC_, NONE);               /* No flags */
      DEFINE_CONST(target, MAGIC_, DEBUG);              /* Turn on debugging */
      //DEFINE_CONST(target, MAGIC_, SYMLINK);            /* Follow symlinks */
      //DEFINE_CONST(target, MAGIC_, COMPRESS);           /* Check inside compressed files */
      //DEFINE_CONST(target, MAGIC_, DEVICES);            /* Look at the contents of devices */
      DEFINE_CONST(target, MAGIC_, MIME_TYPE);          /* Return the MIME type */
      DEFINE_CONST(target, MAGIC_, CONTINUE);           /* Return all matches */
      //DEFINE_CONST(target, MAGIC_, CHECK);              /* Print warnings to stderr */
      //DEFINE_CONST(target, MAGIC_, PRESERVE_ATIME);     /* Restore access time on exit */
      DEFINE_CONST(target, MAGIC_, RAW);                /* Don't translate unprintable chars */
      DEFINE_CONST(target, MAGIC_, ERROR);              /* Handle ENOENT etc as real errors */
      DEFINE_CONST(target, MAGIC_, MIME_ENCODING);      /* Return the MIME encoding */
      DEFINE_CONST(target, MAGIC_, MIME);
      //DEFINE_CONST(target, MAGIC_, APPLE);              /* Return the Apple creator and type */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_COMPRESS);  /* Don't check for compressed files */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_TAR);       /* Don't check for tar files */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_SOFT);      /* Don't check magic entries */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_APPTYPE);   /* Don't check application type */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_ELF);       /* Don't check for elf details */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_TEXT);      /* Don't check for text files */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_CDF);       /* Don't check for cdf files */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_TOKENS);    /* Don't check tokens */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_ENCODING);  /* Don't check text encodings */
      
      /* Defined for backwards compatibility (renamed) */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_ASCII);
      
      /* Defined for backwards compatibility; do nothing */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_FORTRAN);   /* Don't check ascii/fortran */
      DEFINE_CONST(target, MAGIC_, NO_CHECK_TROFF);     /* Don't check ascii/troff */
      
      NODE_SET_PROTOTYPE_METHOD(tpl, "load", LoadAsync);
      NODE_SET_PROTOTYPE_METHOD(tpl, "data", DataAsync);
      
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
