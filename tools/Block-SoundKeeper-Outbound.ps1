<#
.SYNOPSIS
    Adds a Windows Defender Firewall rule that blocks ALL outbound network
    traffic from SoundKeeper64.exe. Defense-in-depth: the audited source has
    no network code, so this proves it cannot call home even if that audit
    were wrong.

.NOTES
    Run from an ELEVATED (Administrator) PowerShell prompt.
    The rule is bound to the absolute exe path, so keep the exe where it is
    after running this. Re-run if you move the exe.

.EXAMPLE
    # From the folder containing SoundKeeper64.exe, in an admin PowerShell:
    .\Block-SoundKeeper-Outbound.ps1
#>
[CmdletBinding()]
param(
    # Full path to SoundKeeper64.exe. Defaults to the copy next to this script.
    [string]$ExePath = (Join-Path $PSScriptRoot 'SoundKeeper64.exe'),

    [string]$RuleName = 'Block SoundKeeper Outbound'
)

$ErrorActionPreference = 'Stop'

# Require elevation — firewall changes need admin.
$isAdmin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    throw 'This script must be run from an elevated (Administrator) PowerShell prompt.'
}

if (-not (Test-Path -LiteralPath $ExePath)) {
    throw "SoundKeeper64.exe not found at: $ExePath. Pass -ExePath '<full path>'."
}
$resolved = (Resolve-Path -LiteralPath $ExePath).Path

# Remove any prior rule with this name so re-running is idempotent.
Get-NetFirewallRule -DisplayName $RuleName -ErrorAction SilentlyContinue |
    Remove-NetFirewallRule

New-NetFirewallRule `
    -DisplayName $RuleName `
    -Direction Outbound `
    -Program $resolved `
    -Action Block `
    -Profile Any `
    -Enabled True `
    -Description 'Blocks all outbound traffic from SoundKeeper64.exe (defense-in-depth).' | Out-Null

Write-Host "Outbound block rule added for:" -ForegroundColor Green
Write-Host "  $resolved"
Write-Host "Verify in: wf.msc  (Outbound Rules -> '$RuleName')  or run Get-NetFirewallRule -DisplayName '$RuleName'"
