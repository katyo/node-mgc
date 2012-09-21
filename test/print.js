var MGC = require('../mgc'),
FS = require('fs');

var mgc = new MGC('type');

mgc.load(function(err){
  if (err) throw err;
  mgc.wrap(FS.createReadStream(process.argv[2]))
    .on('magic', function(err, res){
      if(err) throw err;
      if(res.search('/') < 0) throw new Error(process.argv[2] + ': "' + res + '" not mime');
      console.log(res);
    });
});
