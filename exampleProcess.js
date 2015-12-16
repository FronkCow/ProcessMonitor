var http = require('http');

var server = http.createServer(function(req, res) {
    res.writeHead(200);
    res.end('This is a process!');
});
server.listen(3000);

var fs = require('fs');
fs.writeFileSync('exampleWork.txt', listPrimes(10000));


function listPrimes( nPrimes ) {
    var primes = [];
    for( var n = 2;  nPrimes > 0;  n++ ) {
        if( isPrime(n) ) {
            primes.push( n );
            --nPrimes;
        }
    }
    return primes;
}

function isPrime( n ) {
    var max = Math.sqrt(n);
    for( var i = 2;  i <= max;  i++ ) {
        if( n % i === 0 )
            return false;
    }
    return true;
}
