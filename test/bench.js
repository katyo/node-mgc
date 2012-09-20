function Bench(){
  this._c = 0; // total count
  this._T = 0; // total time
}

function prec(v, p){
  return parseFloat(v.toFixed(p));
}

Bench.prototype = {
  beg: function(){
    return new Date();
  },
  end: function(seed){
    seed = new Date() - seed;
    this._c ++;
    this._T += seed; // amount
    this._m = '_m' in this ? Math.min(this._m, seed) : seed; // min per op
    this._M = '_M' in this ? Math.max(this._M, seed) : seed; // max per op
    this._a = this._T / this._c; // average per op
  },
  stat: function(p){
    p = p !== undefined ? p : 3;
    return {
      all: prec(this._T, p),
      cnt: this._c,
      min: prec(this._m, p),
      max: prec(this._M, p),
      avg: prec(this._a, p)
    };
  }
};

module.exports = Bench;
