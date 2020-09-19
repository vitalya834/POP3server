# Smoke / integration test for the educational POP3 server and client.
#
# Starts the server on a test port, drives a full POP3 session over raw
# TCP (authorization -> transaction -> update), verifies that deletions
# survive into a second session, and finally runs the console client
# with scripted stdin.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File tests\smoke-test.ps1 `
#       [-ServerExe path] [-ClientExe path] [-Port 8125]
#
# Exit code 0 = all checks passed, 1 = at least one failure.

param(
    [string]$ServerExe = "$PSScriptRoot\..\x64\Release\Project21.exe",
    [string]$ClientExe = "$PSScriptRoot\..\x64\Release\POP3client.exe",
    [int]$Port = 8125
)

$ErrorActionPreference = 'Stop'
$script:failures = 0
$script:checks = 0

function Assert-Match {
    param([string]$What, [string]$Actual, [string]$ExpectedPattern)
    $script:checks++
    if ($Actual -match $ExpectedPattern) {
        Write-Host ("  ok   {0}: {1}" -f $What, $Actual)
    } else {
        $script:failures++
        Write-Host ("  FAIL {0}: got '{1}', expected match '{2}'" -f $What, $Actual, $ExpectedPattern) -ForegroundColor Red
    }
}

function Open-Pop3Session {
    param([int]$Port)
    for ($attempt = 0; $attempt -lt 20; $attempt++) {
        try {
            $client = New-Object System.Net.Sockets.TcpClient('127.0.0.1', $Port)
            break
        } catch {
            Start-Sleep -Milliseconds 250
            $client = $null
        }
    }
    if ($null -eq $client) { throw "Cannot connect to 127.0.0.1:$Port" }
    $stream = $client.GetStream()
    $stream.ReadTimeout = 5000
    $reader = New-Object System.IO.StreamReader($stream, [Text.Encoding]::ASCII)
    $writer = New-Object System.IO.StreamWriter($stream, [Text.Encoding]::ASCII)
    $writer.NewLine = "`r`n"
    $writer.AutoFlush = $true
    [pscustomobject]@{ Client = $client; Reader = $reader; Writer = $writer }
}

function Send-Command {
    param($Session, [string]$Command)
    $Session.Writer.WriteLine($Command)
    return $Session.Reader.ReadLine()
}

function Read-UntilDot {
    param($Session)
    $lines = @()
    while (($l = $Session.Reader.ReadLine()) -ne '.') {
        if ($null -eq $l) { throw 'Connection closed inside multi-line reply' }
        $lines += $l
    }
    return $lines
}

if (-not (Test-Path $ServerExe)) {
    Write-Host "Server binary not found: $ServerExe" -ForegroundColor Red
    Write-Host 'Build the solution first (Release|x64), see README.'
    exit 1
}

Write-Host "Starting server on port $Port"
$server = Start-Process -FilePath (Resolve-Path $ServerExe) -ArgumentList $Port -PassThru -WindowStyle Hidden

try {
    # --- Session 1: full scenario -------------------------------------
    $s = Open-Pop3Session -Port $Port
    Write-Host 'Session 1: authorization'
    Assert-Match 'greeting' $s.Reader.ReadLine() '^\+OK POP3 server ready'
    Assert-Match 'CAPA' (Send-Command $s 'CAPA') '^\+OK'
    $null = Read-UntilDot $s
    Assert-Match 'STAT before login' (Send-Command $s 'STAT') '^-ERR'
    Assert-Match 'USER unknown' (Send-Command $s 'USER nobody') '^-ERR'
    Assert-Match 'USER' (Send-Command $s 'USER wladez') '^\+OK'
    Assert-Match 'PASS wrong' (Send-Command $s 'PASS wrong') '^-ERR invalid password'
    Assert-Match 'USER again' (Send-Command $s 'USER wladez') '^\+OK'
    Assert-Match 'PASS' (Send-Command $s 'PASS password') '^\+OK .* 2 messages \(143 octets\)'

    Write-Host 'Session 1: transaction'
    Assert-Match 'STAT' (Send-Command $s 'STAT') '^\+OK 2 143$'
    Assert-Match 'LIST' (Send-Command $s 'LIST') '^\+OK 2 messages'
    $list = Read-UntilDot $s
    Assert-Match 'LIST entries' ($list -join ';') '^1 67;2 76$'
    Assert-Match 'LIST 2' (Send-Command $s 'LIST 2') '^\+OK 2 76$'
    Assert-Match 'RETR 1' (Send-Command $s 'RETR 1') '^\+OK 67 octets'
    $msg = Read-UntilDot $s
    Assert-Match 'RETR 1 subject' ($msg -join ';') 'Subject: Just for fun'
    Assert-Match 'RETR 99' (Send-Command $s 'RETR 99') '^-ERR no such message'
    Assert-Match 'DELE abc' (Send-Command $s 'DELE abc') '^-ERR'
    Assert-Match 'DELE 1' (Send-Command $s 'DELE 1') '^\+OK message 1 deleted'
    Assert-Match 'DELE 1 again' (Send-Command $s 'DELE 1') '^-ERR .*already deleted'
    Assert-Match 'RETR deleted' (Send-Command $s 'RETR 1') '^-ERR .*already deleted'
    Assert-Match 'STAT after DELE' (Send-Command $s 'STAT') '^\+OK 1 76$'
    Assert-Match 'RSET' (Send-Command $s 'RSET') '^\+OK maildrop has 2 messages'
    Assert-Match 'STAT after RSET' (Send-Command $s 'STAT') '^\+OK 2 143$'
    Assert-Match 'NOOP' (Send-Command $s 'NOOP') '^\+OK$'
    Assert-Match 'DELE 2' (Send-Command $s 'DELE 2') '^\+OK message 2 deleted'
    Assert-Match 'QUIT (update)' (Send-Command $s 'QUIT') '^\+OK .*1 messages left'
    $s.Client.Close()

    # --- Session 2: the deletion must have reached the real mailbox ---
    Write-Host 'Session 2: deletion persisted after UPDATE'
    $s2 = Open-Pop3Session -Port $Port
    $null = $s2.Reader.ReadLine()
    $null = Send-Command $s2 'USER wladez'
    Assert-Match 'PASS (1 message left)' (Send-Command $s2 'PASS password') '^\+OK .* 1 messages \(67 octets\)'
    Assert-Match 'STAT' (Send-Command $s2 'STAT') '^\+OK 1 67$'

    # --- Maildrop lock: second concurrent session must be rejected ----
    Write-Host 'Concurrent session: maildrop lock'
    $s3 = Open-Pop3Session -Port $Port
    $null = $s3.Reader.ReadLine()
    $null = Send-Command $s3 'USER wladez'
    Assert-Match 'PASS while locked' (Send-Command $s3 'PASS password') '^-ERR unable to lock maildrop'
    $null = Send-Command $s3 'QUIT'
    $s3.Client.Close()
    $null = Send-Command $s2 'QUIT'
    $s2.Client.Close()

    # --- Console client with scripted stdin ---------------------------
    if (Test-Path $ClientExe) {
        Write-Host 'Console client: scripted session'
        $inputFile = Join-Path $env:TEMP 'pop3client-smoke-input.txt'
        [IO.File]::WriteAllText($inputFile,
            "USER wladez`r`nPASS password`r`nSTAT`r`nRETR 1`r`nQUIT`r`n",
            [Text.Encoding]::ASCII)
        $clientPath = (Resolve-Path $ClientExe).Path
        $output = cmd /c "`"$clientPath`" 127.0.0.1 $Port < `"$inputFile`"" | Out-String
        Remove-Item $inputFile -ErrorAction SilentlyContinue
        Assert-Match 'client login' $output "maildrop has 1 messages"
        Assert-Match 'client RETR' $output 'Subject: Just for fun'
        Assert-Match 'client QUIT' $output 'signing off'
    } else {
        Write-Host "Client binary not found ($ClientExe), skipping client check" -ForegroundColor Yellow
    }
} finally {
    if ($server -and -not $server.HasExited) {
        Stop-Process -Id $server.Id -Force -ErrorAction SilentlyContinue
    }
}

Write-Host ''
if ($script:failures -eq 0) {
    Write-Host "PASSED: $($script:checks) checks" -ForegroundColor Green
    exit 0
} else {
    Write-Host "FAILED: $($script:failures) of $($script:checks) checks" -ForegroundColor Red
    exit 1
}
