const fs = require('fs');

// Read the contents of the stone_release file to retrieve
// the branch
const stoneRelease = fs.readFileSync('../../../../stone_release', 'utf8').split('\n');
const branch = stoneRelease[2] === 'dev' ? 'master' : stoneRelease[1];
module.exports = {
  appName: 'Stone Dashboard',
  apiKey: process.env.APPLITOOLS_API_KEY,
  browser: [
    { width: 1920, height: 1080, name: 'chrome' },
    { width: 1920, height: 1080, name: 'firefox' },
    { width: 800, height: 600, name: 'chrome' },
    { width: 800, height: 600, name: 'firefox' }
  ],
  showLogs: false,
  saveDebugData: true,
  failCypressOnDiff: true,
  concurrency: 4,
  baselineBranchName: branch
};
