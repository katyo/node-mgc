var MGC = require('../mgc');

var mgc = new MGC('mime'),
    buf = new Buffer('import Options\nfrom os import unlink, symlink');

mgc.load(function(err){
  if (err) throw err;
  mgc.buffer(buf, function(err, result) {
    if (err) throw err;
    console.log(result);
  });
});
