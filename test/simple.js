var should = require('should'),
MGC = require('../mgc');

var mgc = new MGC('mime'),
str = '#!/usr/bin/env python\n#--*-- coding: utf-8 --*--\n\nimport Options\nfrom os import unlink, symlink',
buf = new Buffer(str),
cnt = 2,
end = function(){
  if(!--cnt){
    console.log('OK');
  }
};

mgc.load(function(err){
  if (err) throw err;
  mgc.data(buf, function(err, res) {
    if (err) throw err;
    res.should.be.equal('text/x-python; charset=us-ascii');
    end();
  });
  mgc.data(str, function(err, res) {
    if (err) throw err;
    res.should.be.equal('text/x-python; charset=us-ascii');
    end();
  });
});
