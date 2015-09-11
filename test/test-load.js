var should = require('should');
var fs = require('fs');
var path = require('path');
var version = require('../version')();

describe('Module loading from various locations', function() {

  var base = process.cwd();
  var buildPath = version.localBuild;
  var buildFile = path.resolve(base, buildPath + '.node');
  var buildModule = path.resolve(base, buildPath);
  var baseModule = path.resolve(base, 'index.js');

  function loadAndTest(path, callback) {
    var uvmon = require(path);
    uvmon.should.have.property('getData');
    uvmon.getData.should.be.type('function');
    var ret = uvmon.getData();
    ret.should.have.property('count');
    ret.should.have.property('avg_ms');
    ret.should.have.property('p50_ms');
    ret.should.have.property('p95_ms');
    ret.should.have.property('max_ms');
    // call this to properly unload the check_cb - only need it when we're
    // doing funky multiple versions of this module at once
    uvmon.stop();
    callback();
  }

  it('works from ' + buildModule, function(done) {
    loadAndTest(buildModule, done);
  });

  it('loads from index.js when built locally', function(done) {
    loadAndTest(baseModule, done);
  });
});
