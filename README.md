# POP3server

An educational POP3 ([RFC 1939](https://www.rfc-editor.org/rfc/rfc1939))
server and console client written in C++ with WinSock for Windows.
The server keeps a few in-memory test accounts with demo mail and walks
each client through the three protocol states:

```
AUTHORIZATION --USER/PASS--> TRANSACTION --QUIT--> UPDATE
```

> **Warning — for learning only.** Authentication is plaintext
> USER/PASS over an unencrypted TCP connection, accounts are hardcoded
> and mail lives in process memory. Never expose this server to a real
> network or reuse real passwords with it.

## Projects in the solution

| Project      | Binary           | Purpose                                  |
|--------------|------------------|------------------------------------------|
| `Project21`  | `Project21.exe`  | Multi-threaded POP3 server               |
| `POP3client` | `POP3client.exe` | Interactive console POP3 client          |

## Building

Requirements: Visual Studio 2022 (v143 toolset) with the
"Desktop development with C++" workload and a Windows 10/11 SDK.

- **IDE:** open `Project21.sln` and build the `Release|x64`
  configuration (any of the four configurations works).
- **Command line:**

```bash
"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" Project21.sln /p:Configuration=Release /p:Platform=x64
```

Binaries land in `x64\Release\` (or `Debug`, `Win32` variants).

## Running the server

```bash
x64\Release\Project21.exe 8110
```

The single optional argument is the listening port (1–65535,
default **8110**). Each client connection is served on its own thread.

## Running the client

```bash
x64\Release\POP3client.exe 127.0.0.1 8110
```

Both arguments are optional (`host` defaults to `127.0.0.1`, `port` to
`8110`). Type commands at the `pop3>` prompt:

```
+OK POP3 server ready
pop3> USER wladez
+OK wladez is a valid mailbox
pop3> PASS password
+OK wladez's maildrop has 2 messages (143 octets)
pop3> LIST
+OK 2 messages (143 octets)
1 67
2 76
pop3> RETR 1
+OK 67 octets
From: lera
To: wladez
Subject: Just for fun

Hi! How are you?
pop3> QUIT
+OK POP3 server signing off (2 messages left)
```

Any telnet-style client works as well: `telnet 127.0.0.1 8110`.

## Test users

| Login    | Password   | Mailbox                       |
|----------|------------|-------------------------------|
| `wladez` | `password` | 2 demo messages               |
| `azat`   | `12345`    | empty                         |
| `lera`   | `1q2w3e`   | empty                         |

## Supported commands

| Command    | State         | Description                                        |
|------------|---------------|----------------------------------------------------|
| `USER x`   | authorization | Select a mailbox name                              |
| `PASS x`   | authorization | Authenticate; locks the maildrop on success        |
| `STAT`     | transaction   | Message count and total size in octets             |
| `LIST [n]` | transaction   | Scan listing for all messages or message *n*       |
| `RETR n`   | transaction   | Retrieve message *n*                               |
| `DELE n`   | transaction   | Mark message *n* as deleted                        |
| `RSET`     | transaction   | Clear all deletion marks                           |
| `NOOP`     | transaction   | Do nothing, reply `+OK`                            |
| `CAPA`     | any           | Capability list (RFC 2449)                         |
| `QUIT`     | any           | From transaction: enter UPDATE — purge marked mail |

Deletions are only applied when the session ends with `QUIT`
(the UPDATE state). A dropped connection rolls the marks back, as the
RFC requires. Only one session may hold a maildrop at a time; a second
login gets `-ERR unable to lock maildrop`.

## Tests

With the solution built (`Release|x64`):

```bash
powershell -ExecutionPolicy Bypass -File tests\smoke-test.ps1
```

The smoke test starts the server on a spare port, drives the full
protocol scenario over raw TCP (including octet counts, error paths,
deletion persistence across sessions and the maildrop lock) and runs
the console client with scripted input. It exits non-zero if any
check fails.
