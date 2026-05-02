const fs = require('fs');
const path = 'd:\\CloudSeaManor\\02_工程主代码\\CloudSeamanor\\include\\CloudSeamanor\\app\\GameApp.hpp';
const backupPath = 'd:\\CloudSeaManor\\02_工程主代码\\CloudSeamanor\\include\\CloudSeamanor\\app\\GameApp.hpp.bak';
const tempPath = 'd:\\CloudSeaManor\\02_工程主代码\\CloudSeamanor\\include\\CloudSeamanor\\app\\GameApp.hpp.temp';

// Read the backup file
let content = fs.readFileSync(backupPath, 'utf8');

// Write to temp file
fs.writeFileSync(tempPath, content, 'utf8');
console.log('Written to temp file');

// Try to replace the original
try {
    // First try to delete original
    fs.unlinkSync(path);
    console.log('Deleted original');
    // Then rename temp to original
    fs.renameSync(tempPath, path);
    console.log('SUCCESS - File replaced');
} catch (e) {
    console.log('Error:', e.message);
    // If rename fails, the file is still locked
    // The backup has the correct content
}
