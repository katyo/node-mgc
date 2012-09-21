var should = require('should'),
dataset = require('./dataset'),
MGC = require('../mgc'),
FS = require('fs');

var mgc = new MGC('type');

function test(path, mime, size, end){
  mgc.wrap(FS.createReadStream(path, {
    bufferSize: size
  })).on('magic', function(err, res, raw){
    if(err) throw err;
    res.should.be.equal(mime);
    end();
  });
}

mgc.load(function(err){
  if (err) throw err;

  var maxsize = 32 * 1024,
  cnt = 0,
  size = 2;
  function next(){
    if(!cnt--){
      if(size < maxsize){
        console.log('Test size:', size);
        for(var path in dataset){
          cnt++;
          test(path, dataset[path], size, next);
        }
        size *= 2;
      }else{
        console.log('OK');
      }
    }
  }

  next();
});
