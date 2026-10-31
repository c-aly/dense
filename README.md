# Dense

Publish a folder to an info server that agents can search.

You own an account. A server has one owner and any number of members. A source is a folder you publish: its identity is the id stored in that folder's `.dense/config.json`, and its name is just the folder's name. One folder can feed many servers, and one server can pull from many people's folders. Linking a folder lets that server's members read it.

The `dense` CLI publishes folders. `dense_server` is the API and the MCP endpoint agents call. The CLI talks to `https://api.densehq.com` (override with `DENSE_URL`).

## First run

```
dense signup
dense server create team

cd ~/notes && dense init
dense connect team
dense push

dense server key create team
```

After signup, the CLI uses your session. Agents never use that session. Give them a server key, or, for Claude, the URL from `dense server url`. ChatGPT not supported.

`<server>` is a server's name, or its id when two servers you can see share a name.

## Account

| Command | What it does |
|---|---|
| `dense signup [--email E]` | Create an account and log in. Dense emails a code; enter it, then choose a password. A mistyped code can be retried. An expired one needs a fresh `dense signup`. |
| `dense login [--email E]` | Log in. The session is stored in `~/.dense/config.json`. |

Passwords are prompted, never passed as flags.

## This folder

| Command | What it does |
|---|---|
| `dense init [path]` | Create `.dense/` here (config and a default `.denseignore`). |
| `dense connect <server>` | Publish this folder to a server you can access. The first time, this creates the folder's source and saves its id. Later connects keep that id and refresh the display name. Run it again with another server to feed that one too. |
| `dense disconnect <server>` | Stop publishing this folder to a server. Uploaded files are kept. |
| `dense push` | Upload new and changed files. Only the folder's owner can push, and one push serves every server it feeds. |
| `dense status` | Who you're logged in as, this folder's source id, and which servers it feeds. |

`dense list` was renamed to `dense status`.

## Servers

| Command | What it does |
|---|---|
| `dense server create <name>` | Create a server. You own it. Names are letters, digits, `.` `_` `-`, up to 64, and unique per owner. |
| `dense server list` | Servers you own or belong to, with your role and the owner. |
| `dense server delete <server> [-y]` | Owner only. Deletes the server for everyone. Asks you to type the name unless `-y` is set. Sources and their files are kept. |
| `dense server visibility <server> public` or `private` | Owner only. A server is created private. `public` lets anyone with its URL read the folders feeding it. `private` closes that. |

`dense server remove` was renamed to `dense server delete`.

### Members

| Command | What it does |
|---|---|
| `dense server share <server> <email>` | Owner only. Give an existing dense user access. |
| `dense server unshare <server> <email>` | Owner removes a member. Use your own email to leave a server you don't own. The owner can't be removed this way. |
| `dense server members <server>` | Who has access. |

### Folders

| Command | What it does |
|---|---|
| `dense server sources <server>` | Folders feeding a server: name, owner, and source id. Names can repeat. |

### Agents

A server key (`dns_srv_…`) lets an agent read one server. Any member can mint keys. Keys can't push, share, or manage the server. Paste one as `Authorization: Bearer …`, never in the URL. A key is deleted when the member who minted it leaves or is removed.

| Command | What it does |
|---|---|
| `dense server url <server>` | URL for a Claude custom connector. Sign in with Google using the email on your dense account. The agent can then read that server only. ChatGPT not supported. |
| `dense server key create <server> [--label <name>]` | Mint a key. Printed once, with paste-ready setup for Claude Code and other clients that take a key (Cursor, and so on). |
| `dense server key list <server>` | Keys on the server: all of them if you own it, otherwise yours. Shows a short prefix, never the full key. |
| `dense server key revoke <server> <key-id>` | Revoke one. A unique id prefix works. The owner can revoke any key; a member only their own. |

## Files on your machine

- `~/.dense/config.json` — your session
- `<dir>/.dense/config.json` — this folder's source id and the servers it feeds
- `<dir>/.dense/.denseignore` — ignore patterns for `push`
