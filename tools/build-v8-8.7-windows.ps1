# build-v8-8.7-windows.ps1 — build the pinned V8 8.7.220.3 monolith on Windows.
#
# This is the Windows counterpart of tools/build-v8-8.7-macos.sh; it produces the
# SAME pinned V8 (8.7.220.3) the miniblink Windows CMake build links
# (build-win/, MINIBLINK_V8_ROOT). A 2020 V8 tree is compiled on a 2026 toolchain:
# VS 2022 Build Tools (SDK + linker) + a modern LLVM clang-cl (the DEPS-pinned
# clang 12 cannot compile against VS2022's STL; see memory v8-compiler-version).
#
# Output: $V8Root\v8\out\x64.release\obj\v8_monolith.lib  (+ include\)
#
# Prereqs auto-handled: depot_tools, a REAL python (uv, since the Windows Store
# python stub is dead), the pinned gn, the modern LLVM clang-cl, modern
# jinja2/markupsafe, and a set of small in-tree edits (applied idempotently). The
# edits are done inline here rather than as git patches because the V8 tree files
# are CRLF and a whole-file patch would be line-ending noise.
#
# Re-running is idempotent. Run from a Developer-less plain shell; admin is needed
# once for the long-paths registry key.
#
# Required tools on PATH beforehand: git, uv, winget. (VS 2022 Build Tools with the
# VCTools workload + a Windows 10/11 SDK, and LLVM, are installed by this script if
# absent.)
$ErrorActionPreference = 'Stop'

# ---- Config ----------------------------------------------------------------
$V8Version  = '8.7.220.3'
$V8Root     = if ($env:V8_ROOT) { $env:V8_ROOT } else { "$env:USERPROFILE\build\v8-8.7" }
$DepotTools = if ($env:DEPOT_TOOLS) { $env:DEPOT_TOOLS } else { "$env:USERPROFILE\build\depot_tools" }
$RepoDir    = Split-Path $PSScriptRoot -Parent          # miniblink repo root
$SdkVersion = '10.0.22621.0'                            # an installed Windows SDK
$LlvmDir    = 'C:\Program Files\LLVM'                   # winget LLVM.LLVM default
$PyVersion  = '3.11.8'

function Step($m) { Write-Host "==> $m" -ForegroundColor Cyan }

# ---- 1. Long paths + git tuning (V8/Chromium paths exceed MAX_PATH) ---------
Step 'Enabling long paths + git tuning'
try {
  Set-ItemProperty 'HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem' -Name LongPathsEnabled -Value 1 -Type DWord
} catch { Write-Warning 'Could not set LongPathsEnabled (need admin once); continuing.' }
git config --global core.longpaths true
git config --global core.filemode false
git config --global core.fscache true
git config --global core.preloadindex true
git config --global depot-tools.allowGlobalGitConfig false
New-Item -ItemType Directory -Force -Path (Split-Path $V8Root) | Out-Null

# ---- 2. depot_tools ---------------------------------------------------------
Step 'depot_tools'
if (-not (Test-Path "$DepotTools\gclient.bat")) {
  git clone --depth 1 https://chromium.googlesource.com/chromium/tools/depot_tools.git $DepotTools
}
# Use the local VS toolchain, not Google's internal one; don't self-update mid-build.
[Environment]::SetEnvironmentVariable('DEPOT_TOOLS_WIN_TOOLCHAIN', '0', 'User')
$env:DEPOT_TOOLS_WIN_TOOLCHAIN = '0'
$env:DEPOT_TOOLS_UPDATE = '0'

# ---- 3. A real python (the Store python.exe stub cannot execute; gn invokes
#         python.exe directly via CreateProcess, so a .bat shim is not enough) --
Step 'Python (uv standalone CPython)'
& uv python install $PyVersion 2>&1 | Out-Null
$PyExe = (& uv python find $PyVersion).Trim()
$PyDir = Split-Path $PyExe
if (-not (Test-Path "$PyDir\python3.exe")) { Copy-Item "$PyDir\python.exe" "$PyDir\python3.exe" }  # gn looks for python3
# uv python dir first so gn/hooks find a real python{,3}.exe ahead of the Store stub.
$env:PATH = "$PyDir;$DepotTools;$env:PATH"

# Bootstrap depot_tools (downloads its bundled toolchain on first gclient run).
& gclient --version | Out-Null

# ---- 4. VS 2022 Build Tools + LLVM clang-cl ---------------------------------
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere) -or -not (& $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)) {
  Step 'Installing VS 2022 Build Tools (VCTools + Win11 SDK)'
  winget install --id Microsoft.VisualStudio.2022.BuildTools --silent --accept-package-agreements --accept-source-agreements `
    --override "--quiet --wait --norestart --add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --includeRecommended"
}
$VsPath = (& $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath)
$env:GYP_MSVS_OVERRIDE_PATH = $VsPath
$env:GYP_MSVS_VERSION = '2022'
$env:WINDOWSSDKDIR = "${env:ProgramFiles(x86)}\Windows Kits\10"
if (-not (Test-Path "$LlvmDir\bin\clang-cl.exe")) {
  Step 'Installing LLVM (clang-cl >= 19, required by the VS2022 STL)'
  winget install --id LLVM.LLVM --silent --accept-package-agreements --accept-source-agreements
}

# ---- 5. Fetch + pin V8 ------------------------------------------------------
Step "Fetching V8 $V8Version (gclient sync)"
New-Item -ItemType Directory -Force -Path $V8Root | Out-Null
@"
solutions = [
  { "name": "v8", "url": "https://chromium.googlesource.com/v8/v8.git@$V8Version",
    "managed": False, "custom_deps": {}, "custom_vars": {} },
]
target_os = ["win"]
"@ | Set-Content -Path "$V8Root\.gclient" -Encoding ascii
Push-Location $V8Root
& gclient sync -D --no-history --shallow
Pop-Location

$V8 = "$V8Root\v8"

# ---- 6. Pinned clang download (for the version-check exec_script) + gn ------
Step 'Pinned clang + gn'
Push-Location $V8
& python tools/clang/scripts/update.py | Out-Null   # downloads pinned clang 12 (version check only)
Pop-Location

# ---- 7. Modern jinja2/markupsafe (the bundled 2020 ones break on Python 3.11)
Step 'Modern jinja2/markupsafe'
$jenv = "$V8Root\jenv"
& uv pip install --python $PyExe --target $jenv jinja2 markupsafe 2>&1 | Out-Null
foreach ($pkg in 'jinja2','markupsafe') {
  $dst = "$V8\third_party\$pkg"
  if ((Test-Path $dst) -and -not (Test-Path "$dst.orig")) { Rename-Item $dst "$pkg.orig" }
  elseif (Test-Path $dst) { Remove-Item $dst -Recurse -Force }
  Copy-Item "$jenv\$pkg" $dst -Recurse -Force
}

# ---- 8. Patches -------------------------------------------------------------
Step 'Applying patches'
# 8a. The repo's git patches (semantic + modern-clang fixes). global_proxy_signature,
#     nonfatal_dcheck, dcheck_kmax, macos (constexpr) at the v8 root; build_dir in build/.
function Apply-GitPatch($dir, $patch) {
  Push-Location $dir
  & git apply --reverse --check $patch 2>$null
  if ($LASTEXITCODE -ne 0) {
    & git apply --check $patch 2>$null
    if ($LASTEXITCODE -eq 0) { & git apply $patch; Write-Host "    applied $(Split-Path $patch -Leaf)" }
    else { Write-Warning "    could not apply $(Split-Path $patch -Leaf)" }
  } else { Write-Host "    already applied $(Split-Path $patch -Leaf)" }
  Pop-Location
}
foreach ($p in 'global_proxy_signature','nonfatal_dcheck','dcheck_kmax','macos') {
  Apply-GitPatch $V8 "$RepoDir\patches\v8_8_7_$p.patch"
}
Apply-GitPatch "$V8\build" "$RepoDir\patches\v8_8_7_build_dir.patch"

# 8b. Windows-only in-tree edits (inline; CRLF-safe; idempotent).
# vs_toolchain.py: recognise VS 2022 (the 2020 file only knows 2017/2019).
$vt = "$V8\build\vs_toolchain.py"
$c = [IO.File]::ReadAllText($vt)
if (-not $c.Contains("('2022', '17.0')")) {
  $lines = Get-Content $vt; $out = New-Object System.Collections.Generic.List[string]
  foreach ($ln in $lines) {
    $out.Add($ln)
    if ($ln.Trim() -eq 'MSVS_VERSIONS = collections.OrderedDict([') { $out.Add("  ('2022', '17.0'),") }
    if ($ln.Trim() -eq 'MSVC_TOOLSET_VERSION = {') { $out.Add("   '2022' : 'VC143',") }
  }
  Set-Content -Path $vt -Value $out -Encoding ascii
  Write-Host '    vs_toolchain.py: added VS 2022'
}
# vs_toolchain.py _CopyDebugger: the "Debugging Tools for Windows" SDK feature is
# not installed and not needed for a static monolith — make the missing-DLL fatal
# raise a `continue` instead.
$c = [IO.File]::ReadAllText($vt)
if ($c.Contains("raise Exception('%s not found")) {
  $lines = Get-Content $vt; $start=-1; $end=-1
  for ($i=0; $i -lt $lines.Count; $i++) {
    if ($lines[$i].Trim().StartsWith("raise Exception('%s not found")) { $start=$i }
    if ($start -ge 0 -and $lines[$i].Trim().EndsWith("(debug_file, full_path))")) { $end=$i; break }
  }
  if ($start -ge 0) {
    $out = New-Object System.Collections.Generic.List[string]
    for ($i=0; $i -lt $lines.Count; $i++) {
      if ($i -eq $start) { $out.Add('        continue  # miniblink: Debugging Tools not installed/needed') }
      elseif ($i -gt $start -and $i -le $end) { } else { $out.Add($lines[$i]) }
    }
    Set-Content -Path $vt -Value $out -Encoding ascii
    Write-Host '    vs_toolchain.py: _CopyDebugger non-fatal'
  }
}
# setup_toolchain.py: the hardcoded SDK version passed to vcvarsall must be installed.
$st = "$V8\build\toolchain\win\setup_toolchain.py"
$c = [IO.File]::ReadAllText($st)
if ($c.Contains('10.0.19041.0')) {
  [IO.File]::WriteAllText($st, $c.Replace('10.0.19041.0', $SdkVersion))
  Write-Host "    setup_toolchain.py: SDK -> $SdkVersion"
}
# push_registers_asm.cc: stock clang-cl parses the _WIN64 AT&T asm as Intel; force
# AT&T with a .att_syntax directive at the top of the asm block.
$pr = "$V8\src\heap\base\asm\x64\push_registers_asm.cc"
$c = [IO.File]::ReadAllText($pr)
if (-not $c.Contains('.att_syntax')) {
  $lines = Get-Content $pr; $out = New-Object System.Collections.Generic.List[string]; $done=$false
  foreach ($ln in $lines) {
    if (-not $done -and $ln.Trim().StartsWith('asm(".globl PushAllRegistersAndIterateStack')) {
      $lead = ($ln -replace '\S.*$',''); $rest = $ln.Substring($ln.IndexOf('asm(')+4)
      $out.Add($lead + 'asm(".att_syntax                                       \n"')
      $out.Add($lead + '    ' + $rest); $done=$true
    } else { $out.Add($ln) }
  }
  Set-Content -Path $pr -Value $out -Encoding ascii
  Write-Host '    push_registers_asm.cc: .att_syntax'
}

# ---- 9. clang runtime-lib junction -----------------------------------------
# V8 builds the clang-rt lib path from the pinned clang_version (12.0.0); our LLVM
# ships it under lib/clang/<major>. Junction so lib/clang/12.0.0 -> lib/clang/<major>
# (lld-link /lib errors on a non-existent -libpath, unlike Unix ar).
Step 'clang runtime-lib junction'
$clangDir = Get-ChildItem "$LlvmDir\lib\clang" -Directory | Select-Object -First 1 -ExpandProperty Name
if (-not (Test-Path "$LlvmDir\lib\clang\12.0.0")) {
  cmd /c mklink /J "$LlvmDir\lib\clang\12.0.0" "$LlvmDir\lib\clang\$clangDir" | Out-Null
}

# ---- 10. Configure + build --------------------------------------------------
Step 'gn gen'
$out = "$V8\out\x64.release"
New-Item -ItemType Directory -Force -Path $out | Out-Null
@"
is_debug = false
target_cpu = "x64"
v8_target_cpu = "x64"
v8_monolithic = true
v8_use_external_startup_data = false
is_component_build = false
use_custom_libcxx = false
treat_warnings_as_errors = false
v8_enable_i18n_support = false
is_clang = true
clang_base_path = "C:/PROGRA~1/LLVM"
clang_use_chrome_plugins = false
symbol_level = 1
v8_enable_sandbox = false
dcheck_always_on = true
"@ | Set-Content -Path "$out\args.gn" -Encoding ascii
Push-Location $V8
& gn gen out\x64.release
Step 'autoninja v8_monolith (this takes a while)'
& autoninja -C out\x64.release v8_monolith
Pop-Location

$lib = "$out\obj\v8_monolith.lib"
if (Test-Path $lib) {
  Write-Host ''
  Write-Host "==> Built: $lib  ($([math]::Round((Get-Item $lib).Length/1MB,1)) MB)" -ForegroundColor Green
  Write-Host "==> Headers: $V8\include"
  Write-Host "Point the CMake build at it with: -DMINIBLINK_V8_ROOT=$V8"
} else {
  Write-Error "v8_monolith.lib not produced at $lib"
}
