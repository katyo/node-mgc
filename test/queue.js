var should = require('should'),
Bench = require('./bench'),
dataset = require('./dataset'),
MGC = require('../mgc'),
FS = require('fs');

var mgc = new MGC('t'),
bench = new Bench();

function test(path, mime, next){
  FS.createReadStream(path).once('data', function(buf){
    var seed = bench.beg();
    mgc.data(buf, function(err, res){
      bench.end(seed);
      res.should.be.equal(mime);
      next();
    });
  });
}

mgc.load(function(err){
  if (err) throw err;

  var keys = Object.keys(dataset),
  i = 0;

  function next(){
    if(i < keys.length){
      var path = keys[i++];
      test(path, dataset[path], next);
    }else{
      console.log({
        mu: process.memoryUsage(),
        st: bench.stat(3)
      });
    }
  }

  next();
});
