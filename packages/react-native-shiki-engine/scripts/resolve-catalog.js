#!/usr/bin/env node

const fs = require('node:fs')
const path = require('node:path')
const process = require('node:process')

const packageJsonPath = path.join(__dirname, '..', 'package.json')
const packageJsonBackupPath = path.join(__dirname, '..', 'package.json.backup')
const workspaceYamlPath = path.join(__dirname, '..', '..', '..', 'pnpm-workspace.yaml')

const originalContent = fs.readFileSync(packageJsonPath, 'utf8')
fs.writeFileSync(packageJsonBackupPath, originalContent)

const packageJson = JSON.parse(originalContent)

const yaml = fs.readFileSync(workspaceYamlPath, 'utf8')
const catalogMatch = yaml.match(/catalog:\n((?:  .+\n)*)/)

if (!catalogMatch) {
  console.error('Could not find catalog in pnpm-workspace.yaml')
  process.exit(1)
}

const catalog = {}
const catalogLines = catalogMatch[1].trim().split('\n')
for (const line of catalogLines) {
  const colonIndex = line.indexOf(':')
  if (colonIndex > 0) {
    const name = line.slice(0, colonIndex).trim().replace(/^['"]|['"]$/g, '')
    const version = line.slice(colonIndex + 1).trim().replace(/^['"]|['"]$/g, '')
    catalog[name] = version
  }
}

function resolveCatalogRefs(deps) {
  if (!deps)
    return

  for (const [name, version] of Object.entries(deps)) {
    if (version === 'catalog:') {
      if (catalog[name]) {
        deps[name] = catalog[name]
        console.warn(`Resolved ${name}: catalog: -> ${catalog[name]}`)
      }
      else {
        console.error(`Error: No catalog entry found for ${name}`)
        process.exit(1)
      }
    }
  }
}

resolveCatalogRefs(packageJson.dependencies)
resolveCatalogRefs(packageJson.devDependencies)
resolveCatalogRefs(packageJson.peerDependencies)
resolveCatalogRefs(packageJson.optionalDependencies)

fs.writeFileSync(packageJsonPath, `${JSON.stringify(packageJson, null, 2)}\n`)
console.warn('✓ Resolved all catalog: references in package.json')
