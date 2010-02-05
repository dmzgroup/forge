var sys = require ('sys');
exports.dump = function (x){sys.puts(x===undefined?'undefined':JSON.stringify(x,null,1))}
exports.inspect = function(x){sys.puts(sys.inspect(x))}