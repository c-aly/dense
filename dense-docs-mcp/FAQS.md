# Frequently Asked Questions


## How do I get started?

First, you'll need to sign up using `dense signup` - enter your email, verify, and set a password. Then, you can follow the setup guide under *First Run* in *README.md*, or you can simply follow the instructions labeled *next* after running `dense` commands. Basically, after signing up, you create a server via `dense server create <name>`, enter whatever folder you want to create your first `dense` project in and run `dense init`, then use `dense connect <server>` from inside the project to connect. After that, you can just use `dense push` whenever you like to push to the server. Automation templates coming soon at *densehq.com*!


## Why can't I connect to ChatGPT?

Unfortunately, ChatGPT's authorization method isn't compatible with Dense *private* servers. You can use Codex/Claude Code/Cursor by creating a key, or you can use Claude.ai by adding a custom *connector* using the URL from `dense server url <server>`. Or, if your server is public, you should be able to use ChatGPT.


## How do I let people access my server?

If your server is *private*, they'll need to have a Dense account (or create one). Then you can use `dense server share <email>` to grant them access. You're still the owner, and you have elevated permissions (delete, disconnect others' sources, unshare). If your server is *public*, you can either mint a key via `dense server key create <server>` and share it publicly, or you can share your public url from `dense server url <server>


## Who can read my files? 

All servers are private unless you set them as public. Members can access files (through MCP), and so can anyone holding a key. Running `dense server visibility <server> public` lets anyone with the URL read the folders linked to it.


## How do I take files off a server? 

Using `dense disconnect <server>` stops that server from seeing the folder, and the uploaded files stay. Deleting the server does the same for every folder linked to it. Right now, there is no command to clean up all files - `dense purge` coming within the next few days! In the meantime, if it's urgent, you can reach me at *hello@densehq.com* and I'll clean up anything left over instantly.


## Why did share fail? 

Most likely, the other person needs to create a dense account. Alternatively if you're trying to unshare, note that the owner can't be removed with unshare, you'll have to delete the server instead. Working on ownership transfer.


## I renamed or copied the folder. Is that a new source? 

The id in .dense/config.json is the source. Renaming only changes the display name. A copied folder keeps that id until it isn't yours, and then dense connect creates a new one.


## I have another question, who can I contact?

You can reach me at *hello@densehq.com* and I'll get back to you within 24hrs, if not sooner! Please feel free to send any feedback or feature requests as well.
