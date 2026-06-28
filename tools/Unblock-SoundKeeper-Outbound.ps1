<#
.SYNOPSIS
    Removes the outbound-block firewall rule created by
    Block-SoundKeeper-Outbound.ps1.

.NOTES
    Run from an ELEVATED (Administrator) PowerShell prompt.

.EXAMPLE
    .\Unblock-SoundKeeper-Outbound.ps1
#>
[CmdletBinding()]
param(
    [string]$RuleName = 'Block SoundKeeper Outbound'
)

$ErrorActionPreference = 'Stop'

$isAdmin = ([Security.Principal.WindowsPrincipal] `
    [Security.Principal.WindowsIdentity]::GetCurrent()
).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    throw 'This script must be run from an elevated (Administrator) PowerShell prompt.'
}

$rules = Get-NetFirewallRule -DisplayName $RuleName -ErrorAction SilentlyContinue
if (-not $rules) {
    Write-Host "No rule named '$RuleName' found. Nothing to remove."
    return
}

$rules | Remove-NetFirewallRule
Write-Host "Removed firewall rule '$RuleName'." -ForegroundColor Green
