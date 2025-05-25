#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const os = require('os');

// 获取平台和架构信息
const platform = os.platform();
const arch = os.arch();

// 获取 Node.js ABI 版本
const nodeAbi = `node-v${process.versions.modules}`;

// 映射平台名称以匹配 node-pre-gyp 的约定
const platformMap = {
  'darwin': 'darwin',
  'linux': 'linux',
  'win32': 'win32'
};

const archMap = {
  'x64': 'x64',
  'arm64': 'arm64'
};

const mappedPlatform = platformMap[platform];
const mappedArch = archMap[arch];

if (!mappedPlatform || !mappedArch) {
  console.error(`Unsupported platform/arch: ${platform}/${arch}`);
  process.exit(1);
}

// 源文件路径（gyp 构建输出）
const sourcePath = path.join(__dirname, '..', 'build', 'Release', 'ft8_lib.node');

// 目标文件路径（node-pre-gyp 期望的位置）
const targetDir = path.join(__dirname, '..', 'lib', 'binding', 'Release', `${nodeAbi}-${mappedPlatform}-${mappedArch}`);
const targetPath = path.join(targetDir, 'ft8_lib.node');

console.log(`Moving binary from ${sourcePath} to ${targetPath}`);

try {
  // 检查源文件是否存在
  if (!fs.existsSync(sourcePath)) {
    console.error(`Source file not found: ${sourcePath}`);
    process.exit(1);
  }

  // 创建目标目录
  if (!fs.existsSync(targetDir)) {
    fs.mkdirSync(targetDir, { recursive: true });
    console.log(`Created directory: ${targetDir}`);
  }

  // 复制文件
  fs.copyFileSync(sourcePath, targetPath);
  console.log(`Successfully moved binary to: ${targetPath}`);

} catch (error) {
  console.error(`Error moving binary: ${error.message}`);
  process.exit(1);
}
