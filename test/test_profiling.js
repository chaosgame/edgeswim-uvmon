
var child_process = require('child_process')
var fs = require('fs')
var path = require('path')

var nodeunit = require('nodeunit')
var uvmon = require('../index.js')

var VALID_LOGFILE = path.join(__dirname, 'profiling.log')
var INVALID_LOGFILE = path.join(__dirname, 'invalid', 'profiling.log')

exports.tearDown = function (done) {
  if (fs.existsSync(VALID_LOGFILE)) {
    fs.unlinkSync(VALID_LOGFILE)
    uvmon.stopProfiling()
  }

  done()
}

exports.testProfilingWithoutFile = function (test) {
  try {
    uvmon.startProfiling()
  } catch (e) {
    test.equal(e.message, 'Expected startProfiling(filename)')
    test.done()
  }
}

exports.testProfilingInvalidFile = function (test) {
  try {
    uvmon.startProfiling(INVALID_LOGFILE)
  } catch (e) {
    test.equal(e.message, 'Could not open file "' + INVALID_LOGFILE + '" with error "no such file or directory"')
    test.done()
  }
}

exports.testProfilingTwice = function (test) {
  try {
    uvmon.startProfiling(VALID_LOGFILE)
    uvmon.startProfiling(VALID_LOGFILE)
  } catch (e) {
    test.equal(e.message, 'startProfiling() called when profiling is already enabled')
    test.done()
  }
}

exports.testProfilingOk = function (test) {
  uvmon.startProfiling(VALID_LOGFILE)
  child_process.execSync('sleep 1')

  /* uvmon picks up on slow event loops only _after_ they have completed.
   * If we run the code below with the blocking code above, we will have
   * stopped uvmon before the slow event loop had a chance to complete
   * and the file will be empty.
   */
  setImmediate(function () {
    uvmon.stopProfiling()

    var data = uvmon.getData()
    test.ok(data.max_ms > 1000)

    var stats = fs.statSync(VALID_LOGFILE)
    test.ok(stats.size > 0)
    test.done()
  })
}
