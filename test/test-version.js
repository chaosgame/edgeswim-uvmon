var should = require('should');
var version = require('../version');
var path = require('path');

describe('local build binary paths', function() {
  it('has a localBuild', function() {
    var v = version();
    v.should.have.property('localBuild');
    v.localBuild.should.endWith(path.join('build', 'Release', 'uvmon'));
    v.localBuild.should.startWith('.');
  });
});
