# Private Admin MCP Architecture

Date: 2026-03-13

## Purpose

Define the private admin-agent architecture for the Android app using the OpenAI Responses API with the `mcp` tool type.

This is the control-plane path for:
- website maintenance
- Cloudflare operations
- Wrangler deploys
- repo/file operations
- analyzer actions
- model registry management
- command registry management

## OpenAI Contract

Use the Responses API with `tools` entries of type `mcp`.

Two supported patterns:
- OpenAI connectors for supported third-party services
- private remote MCP server for AIFR3D admin tools

For AIFR3D, the main implementation is a private remote MCP server.

## What Uses Connectors vs Private MCP

### OpenAI Connectors

Use only if needed for external content services such as:
- Dropbox
- Gmail
- Google Calendar
- Google Drive
- Microsoft Teams
- Outlook Calendar
- Outlook Email
- SharePoint

These are optional and not the main admin path.

### Private Remote MCP Server

Required for:
- Cloudflare API actions
- Wrangler deploy actions
- website file/content actions
- repo/git actions
- admin command registry
- model registry
- audio analysis and reference pool tools
- device/admin orchestration

Inference:
- Cloudflare and Wrangler are not built-in OpenAI connectors.
- They must be exposed through the private MCP server.

## Approval Policy

Default rule:
- require approval for anything that writes, deletes, deploys, pushes, modifies DNS, rotates secrets, or executes shell commands

Allow auto-approval only for:
- read-only repo/file inspection
- read-only website inspection
- read-only log/status actions
- safe analyzer/report generation

Examples:

- `repo.read_file`: never
- `repo.search`: never
- `website.fetch_public`: never
- `catalog.list`: never
- `audio.analyze_file`: never

- `repo.write_file`: always
- `repo.delete_path`: always
- `git.commit`: always
- `git.push`: always
- `wrangler.deploy`: always
- `cloudflare.dns_update`: always
- `cloudflare.kv_put`: always
- `commands.add`: always
- `commands.remove`: always

## Authorization Handling

Per OpenAI MCP behavior:
- do not assume authorization tokens are stored by OpenAI
- send `authorization` values on every Responses create request that needs them

Implication for app design:
- Android app should not hold long-lived Cloudflare or Wrangler secrets directly
- app authenticates to private admin backend
- private admin backend supplies scoped credentials/tokens when building the Responses request

## MCP Server Tool Groups

### Repo Tools
- `repo.read_file`
- `repo.write_file`
- `repo.delete_path`
- `repo.list_dir`
- `repo.search`
- `repo.diff`

### Git Tools
- `git.status`
- `git.log`
- `git.diff`
- `git.commit`
- `git.push`

### Website Tools
- `website.content_get`
- `website.content_set`
- `website.asset_upload`
- `website.asset_remove`
- `catalog.list`
- `catalog.add`
- `catalog.remove`

### Cloudflare Tools
- `cloudflare.kv_get`
- `cloudflare.kv_put`
- `cloudflare.kv_delete`
- `cloudflare.dns_get`
- `cloudflare.dns_update`
- `cloudflare.worker_tail`

### Wrangler Tools
- `wrangler.deploy`
- `wrangler.rollback`
- `wrangler.whoami`
- `wrangler.secret_put`

### Analyzer Tools
- `audio.analyze_file`
- `audio.analyze_reference_delta`
- `reference_pool.validate`
- `reference_pool.rebuild`

### Registry Tools
- `models.list`
- `models.add`
- `models.remove`
- `commands.list`
- `commands.add`
- `commands.remove`

## Model Registry

Do not hard-code future model growth in the Android UI.

Store model metadata in a private registry with:
- `id`
- `label`
- `enabled`
- `purpose`
- `tool_profile`
- `default_verbosity`

Initial registry should support:
- `gpt-5`
- `gpt-5-mini`
- any account-specific GPT-5.x models you explicitly add

Important:
- validate exact model IDs at runtime against your account
- do not assume undocumented aliases exist

## Android App Tabs

Target tabs:
- `Chat`
- `Analyzer`
- `Beat Catalog`
- `Commands`

Optional later:
- `Deploy`
- `Models`

## Termux-Like Constraint

OpenAI MCP tool calling does not itself provide Android shell access.

To behave like “Codex mobile”:
- the chat issues tool calls
- the private MCP server routes server-side tasks
- a local Android executor or Termux bridge handles on-device file/system actions

Without that executor, the model cannot truly operate “exactly like Termux.”

## Safety Rules

- only trust private or official MCP servers
- log every MCP call and approval decision
- keep destructive tools approval-gated
- treat tool output as untrusted data
- never embed untrusted URLs from tool outputs without validation
- keep public website and private admin agent completely separate
