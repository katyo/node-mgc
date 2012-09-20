var mgc = require('./build/Release/mgc');

function isStr(data){
  return typeof data == 'string';
}

function isFunc(data){
  return typeof data == 'function';
}

function AStr2Bits(args, num, opts){
  var arg = args[num], pre, opt, bit;
  if(isStr(arg)){
    args[num] = 0;
    do{
      pre = arg;
      for(opt in opts){
        arg = arg.split(opt);
        if(arg.length > 1){
          bit = opts[opt];
          if(isFunc(bit)){
            bit = bit(args);
          }
          args[num] |= bit;
        }
        arg = arg.join('');
      }
    }while(pre != arg);
  }
}

var flags_opts = {
  /* long */
  debug: mgc.DEBUG,
  type: mgc.MIME_TYPE,
  enc: mgc.MIME_ENCODING,
  mime: mgc.MIME,
  all: mgc.CONTINUE,
  error: mgc.ERROR,
  /* short */
  d: mgc.DEBUG,
  t: mgc.MIME_TYPE,
  c: mgc.MIME_ENCODING,
  m: mgc.MIME,
  a: mgc.CONTINUE,
  e: mgc.ERROR,
  /* separators */
  ' ': 0,
  '+': 0,
  '/': 0,
  '|': 0,
  '-': 0
};

function MGC(flags){
  if(!(this instanceof arguments.callee)){
    return new MGC(flags);
  }
  AStr2Bits(arguments, 0, flags_opts);
  return mgc.MGC.apply(this, arguments);
}

module.exports = MGC;
