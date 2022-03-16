// Copyright 2008 Google Inc. All Rights Reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Test the (deprecated as of JS 1.5) properties of the RegExp function.
var re = /((\d+)\.(\d+))/;
var s = 'abc123.456def';

re.exec(s);

assertEquals(s, RegExp.input);
assertEquals('123.456', RegExp.lastMatch);
assertEquals('456', RegExp.lastParen);
assertEquals('abc', RegExp.leftContext);
assertEquals('def', RegExp.rightContext);

assertEquals(s, RegExp['$_']);
assertEquals('123.456', RegExp['$&']);
assertEquals('456', RegExp['$+']);
assertEquals('abc', RegExp['$`']);
assertEquals('def', RegExp["$'"]);

assertEquals('123.456', RegExp['$1']);
assertEquals('123', RegExp['$2']);
assertEquals('456', RegExp['$3']);
for (var i = 4; i < 10; ++i) {
  assertEquals('', RegExp['$' + i]);
}

// They should be read only.
RegExp['$1'] = 'fisk';
assertEquals('123.456', RegExp['$1']);

// String.prototype.match and String.prototype.replace (when given a
// regexp) and also RegExp.prototype.test should all behave as if
// RegExp.prototype.exec were called.
s = 'ghi789.012jkl';
s.match(re);
assertEquals(s, RegExp.input);
assertEquals('789.012', RegExp.lastMatch);
assertEquals('012', RegExp.lastParen);
assertEquals('ghi', RegExp.leftContext);
assertEquals('jkl', RegExp.rightContext);
assertEquals(s, RegExp['$_']);
assertEquals('789.012', RegExp['$&']);
assertEquals('012', RegExp['$+']);
assertEquals('ghi', RegExp['$`']);
assertEquals('jkl', RegExp["$'"]);
assertEquals('789.012', RegExp['$1']);
assertEquals('789', RegExp['$2']);
assertEquals('012', RegExp['$3']);
for (var i = 4; i < 10; ++i) {
  assertEquals('', RegExp['$' + i]);
}

s = 'abc123.456def';
s.replace(re, 'whocares');
assertEquals(s, RegExp.input);
assertEquals('123.456', RegExp.lastMatch);
assertEquals('456', RegExp.lastParen);
assertEquals('abc', RegExp.leftContext);
assertEquals('def', RegExp.rightContext);
assertEquals(s, RegExp['$_']);
assertEquals('123.456', RegExp['$&']);
assertEquals('456', RegExp['$+']);
assertEquals('abc', RegExp['$`']);
assertEquals('def', RegExp["$'"]);
assertEquals('123.456', RegExp['$1']);
assertEquals('123', RegExp['$2']);
assertEquals('456', RegExp['$3']);
for (var i = 4; i < 10; ++i) {
  assertEquals('', RegExp['$' + i]);
}

s = 'ghi789.012jkl';
re.test(s);
assertEquals(s, RegExp.input);
assertEquals('789.012', RegExp.lastMatch);
assertEquals('012', RegExp.lastParen);
assertEquals('ghi', RegExp.leftContext);
assertEquals('jkl', RegExp.rightContext);
assertEquals(s, RegExp['$_']);
assertEquals('789.012', RegExp['$&']);
assertEquals('012', RegExp['$+']);
assertEquals('ghi', RegExp['$`']);
assertEquals('jkl', RegExp["$'"]);
assertEquals('789.012', RegExp['$1']);
assertEquals('789', RegExp['$2']);
assertEquals('012', RegExp['$3']);
for (var i = 4; i < 10; ++i) {
  assertEquals('', RegExp['$' + i]);
}

// String.prototype.replace must interleave matching and replacing when a
// global regexp is matched and replaced with the result of a function, in
// case the function uses the static properties of the regexp constructor.
re = /(.)/g;
function f() { return RegExp.$1; };
assertEquals('abcd', 'abcd'.replace(re, f));
