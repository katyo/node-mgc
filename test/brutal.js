var should = require('should'),
Bench = require('./bench'),
dataset = require('./dataset'),
MGC = require('../mgc'),
FS = require('fs');

var mgc = new MGC('t'),
bench = new Bench(),
count = 0;

function end(){
  if(!--count){
    console.log({
      mu: process.memoryUsage(),
      st: bench.stat(3)
    });
  }
}

function test(path, mime){
  ++count;
  FS.createReadStream(path).once('data', function(buf){
    var seed = bench.beg();
    mgc.data(buf, function(err, res){
      bench.end(seed);
      res.should.be.equal(mime);
      end();
    });
  });
}

mgc.load(function(err){
  if (err) throw err;

  for(var path in dataset){
    test(path, dataset[path]);
  }
});

