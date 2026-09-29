# (c) 2026 OtherSide Entertainment, Inc
# SPDX-License-Identifier: MIT

$scriptPath = Split-Path $PSCommandPath 
$sourceFile = $scriptPath  | Join-Path -ChildPath BuildInfo.template 
$targetFile = $scriptPath  | Join-Path -ChildPath ../Resources/BuildInfo.h -Resolve
((Get-Content $sourceFile ) | ForEach-Object { $_.replace('{OSE_BUILD_VERSION_MAJOR}', $env:PRJ_VERSION_MAJOR).replace('{OSE_BUILD_VERSION_MINOR}', $env:PRJ_VERSION_MINOR).replace('{OSE_BUILD_NUMBER}',  $env:BUILD_NUMBER).replace('{OSE_BUILD_CL}',  $env:P4_CHANGELIST).replace('{OSE_BUILD_BRANCH}', $env:BRANCH).replace('{OSE_BUILD_CONFIGURATION}', $env:UE_CONFIGURATION)	} | Set-Content $targetFile)
