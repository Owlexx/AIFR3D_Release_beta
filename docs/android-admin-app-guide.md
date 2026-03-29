# Android Admin App Guide — AIFR3D 2.2.4 Beta

This app has 3 tabs:

- `Chat`
- `Upload`
- `Command`

It uses the live site at `https://www.north3rnlight3r.com` and the same admin routes used by the website admin section.

## Before You Start

1. Open the app.
2. Allow:
   - notifications
   - full file access
   - audio capture when prompted for the visualizer
3. In `Upload`, log in with the configured admin credentials.

After login, the same admin session is used for:

- uploads
- commands
- website file editing
- live site data
- inquiries, sales, and logs

## Chat Tab

The `Chat` tab includes:

- the AIFR3D OpenAI chat panel
- the catalog player
- the live visualizer
- analyzed metric bars

### Chat

- Type a message in `Prompt`
- Press `Send`

Example prompts:

- `Summarize the current site state in plain English`
- `Give me mix advice for harsh high mids`
- `Explain what the stereo meter means`

### Catalog Player

The player uses the live site catalog feed.

Buttons:

- `Refresh Catalog` — reloads the current track list from the site
- `Play` — starts the selected track
- `Pause` — pauses playback
- `Stop` — stops playback and resets progress

Track list behavior:

- Tap a track button to select it
- The top line shows `Track Title • BPM • Duration`

### Visualizer and Metrics

The visualizer is tied to the selected track and live playback signal.

Displayed metric bars:

- `Tone`
- `Dynamics`
- `Loudness`
- `Stereo`
- `Transient`

What they mean:

- `Tone` — bright vs dark balance
- `Dynamics` — punch and crest movement
- `Loudness` — current playback energy
- `Stereo` — stored stereo width target for the selected track
- `Transient` — attack activity from the live waveform

The summary line below the bars shows the stored analyzed values in plain English when available.

## Upload Tab

The `Upload` tab handles 3 upload types:

- `Catalog`
- `Reference`
- `Website`

### Catalog Upload

Use this for beats and catalog audio.

Fields:

- `Pack Type`
- `Track Title`
- `Description`
- `BPM`
- `Key`
- `Tempo`
- `Price`
- file picker

Example:

- Upload type: `Catalog`
- Title: `Night Drive`
- BPM: `140`
- Price: `$19.99`

Result:

- audio is uploaded to the catalog route
- the website player can use it
- album art fallback is assigned automatically if needed

### Reference Upload

Use this to add licensed songs to the reference intake pool.

Fields:

- `Reference Track Title`
- `Genre`
- file picker

Supported genres:

- `rap`
- `hip-hop`
- `edm`
- `dubstep`
- `pop`
- `rock`

Example:

- Upload type: `Reference`
- Title: `Clean Reference 01`
- Genre: `rap`

Result:

- file is stored in the licensed reference intake path
- next step is to run the rebuild action

### Website Asset Upload

Use this to replace site images directly from the phone.

Fields:

- `Repo-relative target path`
- file picker

Good target path examples:

- `apps/website/assets/brand/north3rnlight3r_album_art.png`
- `apps/website/assets/brand/hero_mascot.png`
- `apps/website/assets/brand/album_art_concept.png`

Use this for:

- album art replacement
- banner replacement
- hero image updates
- photo swaps

## Command Tab

The `Command` tab contains:

- raw command entry
- registered action buttons
- live site data summary
- website file control
- catalog/inquiry/log/sales access

### Raw Command Box

Type a command and press `Run`.

Example commands:

- `curl -s https://www.north3rnlight3r.com/api/v1/health`
- `action:reference.pool.validate`
- `action:reference.pool.rebuild`
- `action:analysis.engine.verify`

Result behavior:

- the app does not say only `Command Complete`
- it reports what happened in plain English

### Registered Action Buttons

If an action exists in the backend registry, it should appear as a button.

Current registered actions:

- `session.save`
- `catalog.refresh`
- `mixer.channel.set_gain`
- `plugin.insert`
- `analysis.run`
- `command.exec`
- `soundpack.upload`
- `reference.pool.validate`
- `reference.pool.rebuild`
- `analysis.engine.verify`

Most useful action examples:

- `reference.pool.validate`
  - checks that licensed reference sources are valid
  - checks that no forbidden paths are used
  - checks required genre counts

- `reference.pool.rebuild`
  - rebuilds the licensed reference pool from uploaded intake files

- `analysis.engine.verify`
  - runs the analysis verification tests

### Live Site Data

This area auto-refreshes and overwrites itself with current information only.

It shows plain-English summaries for:

- page visits
- API requests
- audio plays
- downloads
- inquiry count
- sales count
- latest traffic event
- latest inquiry
- latest sale
- latest site event
- latest admin action

This view is current-state only. It does not keep a large local history on the phone.

### Website File Control

Use this to edit the live website text files directly.

Main actions:

- `Load File`
- `Save File`
- `List Dir`
- `Delete Path`

Common file paths:

- `apps/website/index.html`
- `apps/website/styles.css`
- `apps/website/app.js`
- `apps/website/config.js`
- `apps/website/assets/data/beat_catalog.json`

Examples:

- change front-page copy in `apps/website/index.html`
- update styles in `apps/website/styles.css`
- update site logic in `apps/website/app.js`

### Catalog and Admin Data Buttons

Buttons:

- `List Tracks`
- `Remove Track Key`
- `Inquiries`
- `ADMINLOG`
- `Sales`
- `Record Sale + Receipt`

Examples:

- Put a track key in `File Path / Track Key`, then press `Remove Track Key`
- Press `Inquiries` to load the current inquiry list
- Press `ADMINLOG` to load recent admin activity
- Press `Sales` to load recorded sales

### Record Sale

Fields:

- `Sale Item`
- `Amount`
- `Customer Email`

Example:

- Sale Item: `AIFR3D VST3 Plugin`
- Amount: `29.99`
- Customer Email: `buyer@example.com`

Result:

- the sale is recorded
- a receipt record is created

## Website Admin Parity

The website admin section now mirrors the phone app for:

- catalog upload
- reference upload
- website asset upload
- website file editing
- admin commands
- registered action buttons
- live site data summary

The phone and website admin use the same backend routes, so changes stay aligned.

## Recommended Daily Workflow

1. Open `Upload`
2. Log in
3. Upload catalog audio, references, or new art
4. Open `Command`
5. Run:
   - `action:reference.pool.validate`
   - `action:reference.pool.rebuild` after new references
   - `action:analysis.engine.verify` when checking analyzer behavior
6. Use `Live Site Data` to confirm traffic, downloads, inquiries, and sales
7. Use `Website File Control` for text and layout edits
8. Use `Chat` to test playback, visual metrics, and AIFR3D responses
