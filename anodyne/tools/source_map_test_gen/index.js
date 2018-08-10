// This script substitutes various tokens in its input and produces source
// maps describing the transformations it made. For a given input file
// F, the script will write out a source map called F.map.
//
// Use: node index.js file1 file2 file3...

var fs = require("fs")
var sourceMap = require("source-map")
var SourceNode = sourceMap.SourceNode

var subs = {
  "s1": "S1Big",
  "s2": "S2Big",
  "r": "s",
  "u2": "¢2", // U+00A2, needs two bytes in UTF-8
  "u3": "€3", // U+20AC, needs three bytes in UTF-8
  "u4": "𐐷4", // U+10437, needs four bytes in UTF-8
  "𐐷": "x", // U+10437, needs two UTF-16 code units
}

function makeSubs(file, text) {
  var offset = 0;
  var col = 0;
  var line = 1;
  var tokStart = 0;
  var nodes = new SourceNode();
  function addToken() {
    if (offset - tokStart > 0) {
      var tok = text.substring(tokStart, offset);
      var tokLen = offset - tokStart;
      if (subs[tok]) {
        nodes.add(new SourceNode(line, col - tokLen, file, subs[tok], tok));
      } else {
        nodes.add(new SourceNode(line, col - tokLen, file, tok));
      }
    }
    tokStart = offset + 1;
  };
  for (; offset < text.length; ++offset, ++col) {
    var c = text.charAt(offset);
    if (c == '\n') {
      addToken();
      nodes.add(new SourceNode(line, col, file, "\n"));
      ++line;
      col = -1;
    } else if (c == ' ') {
      addToken();
      nodes.add(new SourceNode(line, col, file, " "));
    }
  }
  addToken();
  return nodes;
}

process.argv.slice(2).forEach(function (file) {
  var text = fs.readFileSync(file, "utf8");
  var outNode = makeSubs(file, text);
  fs.writeFileSync(file + ".map", outNode.toStringWithSourceMap({
    file: file + ".map"}).map);
});

