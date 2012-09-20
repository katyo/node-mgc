var fs = require('fs'),
cp = require('child_process');

var binding = {
  targets: [
    {
      target_name: 'mgc',
      sources: ['src/mgc.cc'],
      include_dirs: [],
      cflags: [ '-O3' ],
      libraries: []
    }
  ]
};

function getPaths(){
  var i = 0,
  paths = process.env.PATH.replace(/\/[^\/]+(?:\:|$)/g, ':').split(':'),
  uniqs = {};
  for(; i < paths.length; i++){
    uniqs[paths[i]] = true;
  }
  paths = [];
  for(i in uniqs){
    paths.push(i);
  }
  return  paths;
}

function searchFile(name){
  var paths = getPaths();

}

function whereisFile(name, cb){
  var data = '',
  re = new RegExp('\\/' + name.replace(/[-[\]{}()*+?.,\\^$|#\s]/g, "\\$&") + '$');
  cp.spawn('whereis', [name])
    .on('exit', function(code){
      if(!code){
        var i = 0,
        paths = data.replace(/^[^\:]*\:/, '').split(' ');
        for(; i < paths.length; i++){
          if(paths[i].search(re) > 0){
            cb(null, paths[i], paths[i].replace(re, ''));
            return;
          }
        }
      }
      cb(new Error('Not found'));
    })
    .stdout
    .on('data', function(chunk){
      data += chunk;
    }).setEncoding('utf8');
}

function tryNativeLib(files, cb){
  var error = false,
  i,
  cnt = files.length,
  end = function(err){
    if(error){
      return;
    }
    if(err){
      error = true;
      cb(err);
      return;
    }
    cnt--;
    if(!cnt){
      cb();
    }
  };
  for(i = 0; i < files.length; i++){
    whereisFile(files[i], end);
  }
}

tryNativeLib(['magic.h', 'libmagic.so'], function(err){
  if(err) throw new Error('Native installation of libmagic required');

  var target = binding.targets[0];
  target.libraries.push('-lmagic');

  target.include_dirs.push('/usr/include/nodejs');

  fs.writeFile('binding.gyp', JSON.stringify(binding, function(key, val){
    return val;
  }, 2));
});

