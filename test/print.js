var MGC = require('../mgc'),
FS = require('fs');

var mgc = new MGC('t');

mgc.load(function(err){
  if (err) throw err;
  mgc.wrap(FS.createReadStream(process.argv[2]))
    .on('magic', function(err, res){
      if(err) throw err;
      console.log(res);
    });
});
