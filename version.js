var path = require('path');
var name = 'uvmon';

function version(proc) {
  proc = proc || process;

  var platform = proc.platform;
  if (platform == 'solaris') platform = 'sunos';

  return {
    name: name,
    // these paths are where to look, not guarantees that they exist
    localBuild: relPath('build', 'Release', name)
  }
}

// ensures a leading ./ because require() needs it to do a filesystem path,
// not a module path
function relPath() {
  return '.' + path.sep + path.join.apply(null, arguments);
}

module.exports = version;
