# AIFR3D Android Admin App (v2.2.4)

## Role In Ecosystem

This app is the mobile admin control layer for the same ecosystem used by website, VST, and standalone products.

## Runtime Contract

- API default: `https://www.north3rnlight3r.com`
- Chat transport: `wss://www.north3rnlight3r.com/ws/chat` with HTTP fallback
- Auth model: admin session token

## Core Capabilities

- Chat: cloud responses with session context
- Upload: single-button admin upload flow
- Command: execute command API and display output
- Website control: file/content/catalog/log/sales/inquiry operations

## Android Project Path

- `apps/android_admin`

## Build (Terminal)

```bash
cd apps/android_admin
./gradlew :app:assembleDebug
```

APK output:
- `apps/android_admin/app/build/outputs/apk/debug/app-debug.apk`

## Install To Connected Device

```bash
./scripts/install_android.sh
```

## Permissions Model

The app requests only what is needed for the action being used.

Key permissions used:
- network access
- media/file access for upload operations
- notification permission for runtime status alerts

## Operator Notes

- Keep API base URL and admin credentials synced with website admin.
- Use the command screen for quick API/route health checks.
- Use website control actions for file/content updates without manual shell access.
