require('bufferjs');

function AddOn(alts, prefix, suffix){
  for(var i = 0; i < alts.length; i++){
    try{
      return require((prefix || '') + alts[i] + (suffix || ''));
    }catch(e){}
  }
  throw new Error('Unable to load addon');
}

var mgc = AddOn(['Release', 'Debug', 'default'], './build/', '/mgc');

function AStr2Bits(args, num, opts){
  var arg = args[num], pre, opt;
  if(typeof arg == 'string'){
    args[num] = 0;
    do{
      pre = arg;
      for(opt in opts){
        arg = arg.split(opt);
        if(arg.length > 1){
          args[num] |= opts[opt];
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
  '+': 0,
  '|': 0,
  ',': 0,
  ':': 0
};

function MGC(flags){
  if(!(this instanceof arguments.callee)){
    return new MGC(flags);
  }
  AStr2Bits(arguments, 0, flags_opts);
  return mgc.MGC.apply(this, arguments);
}

var minlen_ = 16 * 1024;

mgc.MGC.prototype.wrap = function(stream, minlen){
  var self = this, data;
  minlen = minlen || minlen_;
  function end(){
    self.data(data, function(err, res){
      stream.emit('magic', err, res, data);
    });
  }
  return stream
    .on('end', end)
    .on('data', function beg(buf){
      data = data ?
        Buffer.concat(data, buf) /* collect data */
        : buf; /* initialize data */
      if(data.length >= minlen){ /* detect */
        stream.removeListener('data', beg);
        stream.removeListener('end', end);
        end();
      }
    });
};

module.exports = MGC;
