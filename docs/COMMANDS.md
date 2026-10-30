# Dense Commands

Talks to `https://api.densehq.com`. After install, the binary is named `dense` and
should be on your `PATH` — open a new terminal and run `dense --help`.

## Typical first run

```
dense signup                  # create an account (emails you a code, then asks for a password)

dense server create team      # you become the owner
dense server share team bob@example.com   # bob must already have an account

cd ~/notes && dense init
dense connect team            # feed this directory into the server
dense push                    # upload files

dense server key create team  # paste into Cursor / Claude Code
dense server url team         # OR for Claude.ai / ChatGPT connectors:
```

Everything after `signup` / `login` uses your session. Agents use a **server key**
or (for Claude / ChatGPT) the URL from `dense server url`, never your login.

## Account

| Command | What it does |
|---|---|
| `dense signup [--email E]` | Create an account and log in. Dense server emails verification code. Enter it, then choose a password. A mistyped code can be retried; an expired or locked one needs a fresh `dense signup`. |
| `dense login [--email E]` | Log in. Stores session in `~/.dense/config.json`. |

Passwords are only ever prompted, never passed as flags.

## Servers

A server has one owner and any number of members. `<server>` is its name, or its
id if two servers you can see share a name (e.g. you and a colleague each own a `docs`).

| Command | What it does |
|---|---|
| `dense server create <name>` | Create a server; you're the owner. Names: letters, digits, `.` `_` `-`, up to 64; unique per owner. |
| `dense server list` | Servers you own or belong to, with your role and the owner. |
| `dense server url <server>` | Print the MCP URL for **Claude.ai / ChatGPT** custom connectors. Paste it under Settings → Connectors; you'll sign in with Google using the same email as your dense account. The agent can then read that server only. For Cursor, Claude Code, and other key-based clients, use `dense server key create` instead. |
| `dense server share <server> <email>` | Owner only. Give an existing dense user access. |
| `dense server unshare <server> <email>` | Owner removes a member. Use your own email to leave a server you don't own. |
| `dense server members <server>` | Who has access. |
| `dense server remove <server> [-y]` | Owner only. Deletes the server for everyone (asks you to type its name). Sources and their files are kept. |

### Agent keys

An agent reads a server through `/mcp`, authenticated by a **server key**
(`dns_srv_…`). Any member can mint keys; give each agent its own. Keys are
read-only: they can't push, share, or manage the server.

| Command | What it does |
|---|---|
| `dense server key create <server> [--label <name>]` | Mint a key. Printed **once**, with paste-ready setup for Claude Code and `mcpServers` JSON clients (Cursor, …). The key identifies the server; the URL is the shared `/mcp` endpoint. |
| `dense server key list <server>` | Keys on the server: all of them if you own it, otherwise just yours. Shows a short prefix, never the full key. |
| `dense server key revoke <server> <key-id>` | Revoke one (a unique id prefix works). The owner can revoke any key; a member only their own. |

Paste the key into the agent's MCP config as `Authorization: Bearer …` (never in the
URL). A key is deleted automatically when the member who minted it leaves or is removed.

**Claude / ChatGPT vs Cursor / Claude Code**

| Client | How to connect |
|---|---|
| Claude.ai, ChatGPT | `dense server url <server>` → add as a custom connector; sign in with Google |
| Cursor, Claude Code, SDKs | `dense server key create <server>` → paste URL + bearer key into MCP config |

## This directory (a source)

A source is owned by whoever pushes it. Members of a server can each connect
their own directories to it, and one directory can feed several servers.

| Command | What it does |
|---|---|
| `dense init [path]` | Create `.dense/` (config + default `.denseignore`) in a directory. |
| `dense connect <server>` | Register this directory as your source and link it to a server you own or belong to. Run again with another server to feed that one too. |
| `dense push` | Scan the directory and upload new/changed files. Only the source's owner can push; one push serves every server it feeds. Files over the size limit (10 MB by default) are rejected and reported. |
| `dense list` | Show this directory's source id and servers, and who you're logged in as. |

## Files on your machine

- `~/.dense/config.json` — account session (mode `0600`)
- `<dir>/.dense/config.json` — this source's id and linked servers
- `<dir>/.dense/.denseignore` — ignore patterns for `push`
