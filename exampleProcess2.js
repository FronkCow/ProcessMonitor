var http = require('http');

var server = http.createServer(function(req, res) {
    res.writeHead(200);
    res.end('This is a process!');
});
server.listen(3000);
