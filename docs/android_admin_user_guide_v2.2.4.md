# DawAI Android Admin User Guide (v2.2.4)

## Purpose

Android admin is the private mobile control plane for website + ecosystem operations.

## Fixed 3-Screen UX

1. Chat screen
2. Upload screen (single upload button)
3. Command screen

## Login

1. Open app.
2. Set API base URL to your cloud endpoint if needed.
3. Sign in with admin credentials.
4. Verify session before running admin actions.

## Chat Screen

- Uses websocket chat when available.
- Falls back to HTTP chat endpoint if websocket is unavailable.
- Cloud-only chat path.

## Upload Screen

- Uses one button to select and upload admin pack files.
- No manual path typing required.
- Uses admin session auth.

## Command Screen

- Runs command requests through API.
- Displays `exit_code`, `stdout`, `stderr`, and error values.
- Network/auth failures return explicit error text.

## Website Control Functions

From Android admin you can:
- list/read/write/delete website files
- list/remove catalog tracks
- view inquiries
- view logs
- view sales
- record sales and generate receipts

## Permissions

Required based on task:
- network access (always)
- media/file permissions for uploads
- notification permission for status alerts

## Troubleshooting

### Login failed
- Re-enter credentials.
- Confirm API base URL points to deployed domain.

### Chat failed
- Check cloud model config and token.
- Retry websocket then HTTP fallback.

### Upload failed
- Confirm file permission and active admin session.

## Support

`north3rnlight3rofficial@outlook.com`
