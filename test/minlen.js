var should = require('should'),
dataset = require('./dataset'),
MGC = require('../mgc'),
FS = require('fs');

var mgc = new MGC('t');

function pass(size){
  var count = 0,
  error = 0;

  function end(){
    if(!count){
      if(error){
        console.log('trylen:', size, 'errors:', error);
        pass(size + 1);
      }else{
        console.log('minlen:', size);
      }
    }
  }

  function test(path, mime){
    ++count;
    ++error;
    FS.createReadStream(path, {
      bufferSize: size
    }).once('data', function(buf){
      mgc.data(buf, function(err, res){
        if(res == mime){
          --error;
        }
        --count;
        end();
      });
    });
  }

  for(var path in dataset){
    test(path, dataset[path]);
  }
}

mgc.load(function(err){
  if (err) throw err;

  pass(1);
});
