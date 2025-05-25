# 发布指南

这个指南说明如何通过 GitHub Actions 自动发布 ft8-lib 到 npm。

## 前置要求

### 1. 配置 GitHub Secrets

在 GitHub 仓库设置中添加以下 Secrets：

#### NPM_TOKEN
1. 登录 [npmjs.com](https://www.npmjs.com)
2. 进入 Account Settings > Access Tokens
3. 点击 "Generate New Token" > "Classic Token"
4. 选择 "Automation" 类型
5. 复制生成的 token
6. 在 GitHub 仓库 Settings > Secrets and variables > Actions 中添加：
   - Name: `NPM_TOKEN`
   - Value: 你的 npm token

#### GITHUB_TOKEN
GitHub 会自动提供这个 token，无需手动配置。

### 2. 确保仓库配置正确

- 仓库必须是公开的，或者你有相应的 npm 发布权限
- package.json 中的 repository.url 必须指向正确的 GitHub 仓库
- 所有测试必须通过

## 发布流程

### 方法一：通过 GitHub Actions 手动触发发布

1. 进入 GitHub 仓库的 Actions 页面
2. 选择 "Release" 工作流
3. 点击 "Run workflow"
4. 填写版本号和发布类型：
   - **version**: 具体版本号（如 1.0.1）或留空使用发布类型
   - **release_type**: patch（修补版本）、minor（次要版本）、major（主要版本）
5. 点击 "Run workflow"

### 方法二：通过 Git 标签触发发布

1. 在本地创建并推送标签：
```bash
# 创建标签
git tag v1.0.1
git push origin v1.0.1
```

2. 在 GitHub 上创建 Release：
   - 进入仓库的 Releases 页面
   - 点击 "Create a new release"
   - 选择刚推送的标签
   - 填写发布说明
   - 点击 "Publish release"

## 发布过程说明

### 自动构建流程

当创建 GitHub Release 时，会自动触发以下流程：

1. **多平台构建**：
   - Ubuntu (Linux x64/arm64)
   - Windows (win32 x64/arm64)
   - macOS (darwin x64/arm64)

2. **Node.js 版本测试**：
   - Node.js 16, 18, 20

3. **测试验证**：
   - 运行所有单元测试
   - 验证 CommonJS 和 ES Module 兼容性
   - TypeScript 类型检查

4. **打包**：
   - 生成预编译的原生模块
   - 为每个平台创建 .tar.gz 包

5. **发布**：
   - 将预编译二进制文件上传到 GitHub Release
   - 发布包到 npm registry

### 构建产物

发布后，用户可以通过以下方式安装：

```bash
# 自动下载预编译二进制文件（推荐）
npm install ft8-lib

# 如果预编译文件不可用，将自动从源码编译
npm install ft8-lib --build-from-source
```

## 版本管理

### 版本号规则
遵循 [语义化版本](https://semver.org/lang/zh-CN/)：

- **MAJOR** (主版本号)：不兼容的 API 修改
- **MINOR** (次版本号)：向下兼容的功能性新增
- **PATCH** (修订号)：向下兼容的问题修正

### 发布检查清单

发布前请确认：

- [ ] 所有功能都已测试
- [ ] README.md 已更新
- [ ] 版本号符合语义化版本规范
- [ ] 所有依赖都是稳定版本
- [ ] 没有包含敏感信息

## 故障排除

### 构建失败

如果构建失败，检查：

1. **依赖问题**：确保所有 C++ 编译依赖都正确安装
2. **测试失败**：修复失败的测试用例
3. **权限问题**：确保 GitHub Secrets 配置正确

### 发布失败

如果发布失败：

1. **npm 权限**：确保 NPM_TOKEN 有发布权限
2. **版本冲突**：检查版本号是否已存在
3. **网络问题**：重新运行工作流

### 预编译二进制文件问题

如果用户安装时预编译文件下载失败：

1. 检查 GitHub Release 中是否包含所有平台的二进制文件
2. 确保 package.json 中的 binary 配置正确
3. 验证 GitHub Release 的访问权限

## 监控和维护

### 发布后检查

1. 验证 npm 包页面显示正确
2. 测试从 npm 安装的包
3. 检查不同平台的兼容性
4. 监控用户反馈和问题报告

### 长期维护

- 定期更新依赖项
- 关注安全漏洞报告
- 支持新的 Node.js 版本
- 根据用户反馈改进功能
