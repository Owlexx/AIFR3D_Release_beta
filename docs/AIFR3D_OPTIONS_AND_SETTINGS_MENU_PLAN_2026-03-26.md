# AIFR3D Options Menu And Settings Menu Plan

Date: 2026-03-26
Repository: `/home/north3rnlight3r/Projects/AifredVst-2.2.4-beta`

## Purpose

This document lays out the full plan for the AIFR3D plugin:

- options menu
- settings menu
- quick controls
- per-mode controls
- beginner vs advanced staging
- professional / mastering-level controls

This plan is based on:

- the current plugin code
- `UI PROMPT.txt`
- `GUI METER Map .txt`
- `prelaunchbeta update.txt`
- the repo’s current `TopBar`, `MainView`, `UiModel`, `ApprovalHalo`, and `MixSignatureMeter` structure

This is not implementation code.

This is the design and architecture plan for how the menus should work when the plugin is brought to final polished state.

## Core Design Rule

The menu system must not feel like a random settings dump.

It must do three jobs:

1. expose the most important controls quickly
2. hide complexity until the user asks for it
3. keep every visible option tied to a real behavior

That means:

- quick controls stay in the main top surface
- workflow-critical controls stay near the mode they affect
- deep preferences move into a dedicated settings panel
- no control should exist just because it sounds advanced

## Main Philosophy From Your Notes

Your notes consistently describe the same product direction:

- beginner-readable first
- pro depth second
- no fake movement
- no decorative menus
- no overwhelming wall of settings
- one clear live analysis pipeline
- mode-specific presentation logic
- labeled, understandable advanced meters

The menu architecture should reflect that directly.

## What Already Exists In The Current Plugin

The current plugin already has the beginnings of a menu system.

### 1. Top bar primary mode controls

Current file:

- `Source/UI/TopBar.cpp`

Already present:

- `Analyze`
- `Compare`
- `Reference`

These are mode buttons, not buried settings.

That is correct and should stay.

### 2. Layout variant selector

Current file:

- `Source/UI/TopBar.cpp`

Already present:

- theme/layout `A`
- theme/layout `B`
- theme/layout `C`
- theme/layout `D`

This is also correct, but it needs clearer explanation and product meaning.

### 3. Pulse mode selector

Current file:

- `Source/UI/MainView.cpp`

Already present:

- `Pulse: Loudness`
- `Pulse: Dynamics`
- `Pulse: Stereo`

This came directly from your notes and should stay.

### 4. Reference mode controls

Current file:

- `Source/UI/MainView.cpp`

Already present:

- reference genre box
- pin reference button

### 5. Compare mode controls

Current file:

- `Source/UI/MainView.cpp`

Already present:

- capture Mix A
- capture Mix B
- use Live B

### 6. Advisory/API controls

Current file:

- `Source/UI/MainView.cpp`

Already present:

- API key entry
- save key
- clear key
- ask AI
- attach file
- debug toggle

These should not stay mixed into the same visual level forever.

They need to be reorganized into a proper settings structure.

## The Final Menu System Should Be Divided Into 4 Layers

This is the best way to stage complexity.

### Layer 1. Top Bar Quick Controls

Always visible.

Purpose:

- primary workflow switching
- no deep settings
- only the controls that change what the user is looking at right now

### Layer 2. Context Menus Inside Each Mode

Visible only when needed.

Purpose:

- controls directly related to Analyze, Compare, or Reference mode

### Layer 3. Options Menu

Small, frequent-use preferences.

Purpose:

- fast display/workflow choices
- things users may toggle often during a session

### Layer 4. Full Settings Window

Deep preferences and configuration.

Purpose:

- longer-lived settings
- API settings
- defaults
- performance and persistence behavior
- advanced metering options

## Final High-Level Menu Structure

The plugin should end up with:

### A. Top Bar

- Analyze
- Compare
- Reference
- Layout A/B/C/D
- one `Options` button
- one `Settings` button
- one `Help` button

### B. Context Controls Area

Changes by mode.

- Analyze mode controls
- Compare mode controls
- Reference mode controls

### C. Options Popup

Quick-use toggles and session controls.

### D. Settings Window

Tabbed full preferences.

## Top Bar Plan

The top bar should remain simple and obvious.

## Required Top Bar Controls

### 1. Mode buttons

- `Analyze`
- `Compare`
- `Reference`

Why:

- these are the primary workspaces
- they must stay one click away

### 2. Layout selector

Keep `A/B/C/D`, but rename them in the UI with helper language.

Recommended labels:

- `A: Simple`
- `B: Guided`
- `C: Advanced`
- `D: Pro`

Why:

- your notes explicitly separate simpler and more advanced views
- single-letter labels alone are not good enough for a shipping product

### 3. Options button

Label:

- `Options`

Behavior:

- opens a small non-modal quick menu
- for session-level display and workflow choices

### 4. Settings button

Label:

- `Settings`

Behavior:

- opens the full settings window

### 5. Help button

Label:

- `Help`

Behavior:

- opens:
  - glossary
  - color key
  - mode explanation
  - beginner tips

This is important because your notes repeatedly say the plugin should be easy for beginners without insulting pros.

## Context Menus By Mode

These are not full settings.

These are task-specific controls visible in the current mode.

## Analyze Mode Context Menu

Purpose:

- live diagnostic workflow

Visible controls:

- `View Level`
  - Beginner
  - Advanced
- `Pulse Focus`
  - Loudness
  - Dynamics
  - Stereo
- `Halo Style`
  - Simple
  - Focused
  - Advanced
  - Mastering
- `Analyzer Overlay`
  - Off
  - EQ curve
  - EQ + dynamics
  - EQ + dynamics + stereo
- `Show Live Session Candle`
  - On/Off
- `Show Last 10 Sessions`
  - On/Off

Reasoning:

- these are things a user might change while actively working
- they directly alter how Analyze mode presents the current live buffer

## Compare Mode Context Menu

Purpose:

- A/B diagnostic workflow

Visible controls:

- `Capture Mix A`
- `Capture Mix B`
- `Use Live Buffer As B`
- `Compare Focus`
  - Tonal delta
  - Loudness delta
  - Stereo delta
  - Full delta
- `Compare Snapshot Behavior`
  - Locked A/B
  - Mix A vs Live B
- `Delta Units`
  - Plain English
  - Engineering
  - Both
- `Dual Halo View`
  - Off
  - Planned / Experimental

Note:

- the current repo still has a single compare halo logic path
- your design notes clearly want a future dual-halo compare view
- this control should not be visible until that feature is actually implemented

## Reference Mode Context Menu

Purpose:

- target-alignment workflow

Visible controls:

- `Reference Genre`
- `Pin Reference`
- `Reference Overlay`
  - Ghost fingerprint
  - Corridor cards
  - Band corridor
  - All
- `Reference View`
  - Tight corridor
  - Standard corridor
  - Wide corridor
- `Target Display`
  - Plain English
  - Technical
  - Both

Reasoning:

- these controls change how the target context is visualized
- they should only appear when the user is in Reference mode

## Options Menu Plan

The Options menu is the small, quick-access menu.

This is where the user changes frequent session behavior without opening the full settings window.

## Options Menu Categories

### 1. Display

Quick toggles:

- `Beginner View`
- `Advanced View`
- `Auto View By Layout`
- `High Contrast Labels`
- `Large Readouts`
- `Compact Side Panels`

Why:

- your notes repeatedly call for readable text, layered density, and beginner-first clarity

### 2. Live Visuals

Quick toggles:

- `Pulse Ring On/Off`
- `Background Spectrum On/Off`
- `Halo Spectrum Glow On/Off`
- `Motion Reduction`
- `Show Meter Labels`
- `Show Helper Text`

Why:

- some users will want less visual motion during long sessions
- the product must stay readable

### 3. Session View

Quick toggles:

- `Show Live Candle`
- `Show Session History`
- `Reset Live Session`
- `Clear Compare Snapshots`
- `Reset View Layout`

### 4. Aifred View

Quick toggles:

- `Show Fix List`
- `Show Mix Tips`
- `Show Chat Panel`
- `Keep Last 7 Advisory Outputs`
- `Session-Only History`

### 5. Labeling

Quick toggles:

- `Plain English Labels`
- `Engineering Labels`
- `Plain English + Engineering`
- `Show Color Key`
- `Show “What This Means” Helper Text`

This directly follows your handwritten/menu notes that every advanced meter should still be explained in plain English.

## Full Settings Window Plan

The Settings window should be a proper dedicated panel.

It should not be buried inside the top bar.

Recommended structure:

- left navigation rail
- right content panel
- `Apply`
- `Cancel`
- `Restore Defaults`
- `Reset Current Page`

## Settings Window Pages

### Page 1. General

Purpose:

- default startup behavior

Controls:

- `Startup Mode`
  - Analyze
  - Compare
  - Reference
- `Startup Layout`
  - A
  - B
  - C
  - D
- `Default View Level`
  - Beginner
  - Advanced
  - Auto by layout
- `Remember Last Mode`
  - On/Off
- `Remember Last Layout`
  - On/Off
- `Remember Last Panel Visibility`
  - On/Off

### Page 2. Display

Purpose:

- typography, spacing, panel density, and visual comfort

Controls:

- `Text Size`
  - Medium
  - Large
  - Extra Large
- `Panel Density`
  - Comfortable
  - Standard
  - Compact
- `Color Intensity`
  - Soft
  - Standard
  - Strong
- `Glow Intensity`
  - Low
  - Medium
  - High
- `Animation Intensity`
  - Reduced
  - Standard
  - Full
- `Colorblind-Friendly Palette`
  - Off
  - On

### Page 3. Metering

Purpose:

- how live instruments are presented

Controls:

- `Halo Style`
  - Simple
  - Focused
  - Advanced
  - Mastering
- `Pulse Ring Style`
  - Smooth glow
  - Outward fade
  - Ring spikes
  - Hybrid
- `Analyzer Type`
  - Smooth EQ curve
  - FFT bars
  - EQ + dynamics
  - EQ + dynamics + stereo
- `Show Spectrum Behind Halo`
  - On/Off
- `Show Side-Energy Glow`
  - On/Off
- `Show WXYZ Spatial View`
  - On/Off
- `Show Secondary Engineering Meters`
  - On/Off

This page is one of the most directly supported by your notes.

### Page 4. Analyze Mode

Purpose:

- live-mode presentation defaults

Controls:

- `Analyze Layout Priority`
  - Cockpit
  - Balanced
  - Meter-heavy
- `Default Pulse Focus`
  - Loudness
  - Dynamics
  - Stereo
- `Show Live Fingerprint`
  - On/Off
- `Show Live Session Candle`
  - On/Off
- `Show Session History`
  - On/Off
- `Beginner Analyze Summary`
  - On/Off

### Page 5. Compare Mode

Purpose:

- A/B comparison behavior

Controls:

- `Compare Default`
  - Locked A/B
  - Mix A vs Live B
- `Compare Emphasis`
  - Tonal
  - Loudness
  - Stereo
  - Balanced
- `Show Delta Cards`
  - On/Off
- `Show Compare Fingerprint Overlay`
  - On/Off
- `Show Compare Halo Delta Labels`
  - On/Off
- `Auto-Clear Compare Snapshots On Session End`
  - On/Off

### Page 6. Reference Mode

Purpose:

- target-corridor presentation defaults

Controls:

- `Reference Display Priority`
  - Corridor-first
  - Balanced
  - Live-first
- `Show Ghost Fingerprint`
  - On/Off
- `Show Band Corridor Rows`
  - On/Off
- `Show Reference Color Evaluation On Halo`
  - On/Off
- `Use Pinned Genre By Default`
  - On/Off
- `Fallback Behavior When No Reference Exists`
  - Neutral live-only mode

Important:

- there should be no fake target fallback option
- the only correct behavior is neutral live-only mode when no valid reference exists

### Page 7. History And Candles

Purpose:

- session-memory behavior

Controls:

- `Realtime Candle Window`
  - 0.5 seconds
  - 1 second
  - 2 seconds
- `Session Candle Count`
  - 10
  - 20
  - 30
- `Persist Session History`
  - On/Off
- `Clear Session History`
- `Show Improvement Labels`
  - On/Off
- `Show Engineering Values In Candle Tooltip`
  - On/Off

### Page 8. Aifred / Advisory

Purpose:

- local advisory/client behavior

Controls:

- `OpenAI API Key`
- `Load From Local File`
- `Clear Saved Key`
- `Provider`
  - OpenAI only
- `Model`
  - GPT-5.2 only
- `Keep Session Chat History`
  - On/Off
- `Keep Last 7 Meaningful Advisory Outputs`
  - On/Off
- `Show Inline Error Banners`
  - On/Off

Important:

- provider switching should not be visible
- model switching should not be visible if GPT-5.2-only remains the product rule

### Page 9. Performance

Purpose:

- protect CPU and UI responsiveness

Controls:

- `UI Refresh Rate`
  - 30 Hz
  - 60 Hz
- `Motion Reduction`
  - Off
  - Low
  - Medium
  - High
- `Use Lightweight Spectrum Rendering`
  - On/Off
- `Show Debug Performance Overlay`
  - On/Off
- `Enable Debug Logging`
  - On/Off

This page matters because your notes explicitly say the product must hit smooth 60 FPS / 60 Hz without lag or memory leak behavior.

### Page 10. Accessibility

Purpose:

- readability and usability

Controls:

- `Large Text Mode`
- `High Contrast Mode`
- `Reduced Motion`
- `Plain English First`
- `Tooltips Always On`
- `Color Legend Always Visible`

### Page 11. Files And Data

Purpose:

- persistence, exports, and local storage behavior

Controls:

- `Session Store Path`
- `Export Current Analysis`
- `Export Last Session History`
- `Open Analysis Folder`
- `Reset Local UI State`
- `Reset Compare State`
- `Reset Plugin Settings To Default`

## Beginner Vs Advanced Plan

This needs to be explicit because your notes repeatedly divide the product this way.

## Beginner Mode

Beginner mode should show:

- simpler halo language
- plain-English summaries
- fewer secondary meters
- clear color meaning
- larger labels
- helper text always on

Settings that should be hidden or collapsed in beginner mode:

- advanced analyzer type switching
- engineering-only labels
- deep compare behavior toggles
- debug/performance controls

## Advanced Mode

Advanced mode should show:

- deeper metering
- more labels
- more cards
- engineering units
- reference and compare detail
- advanced fingerprint and candle context

## Professional / Mastering Mode

This should map best to layout `D` and/or advanced settings.

Recommended behavior:

- corridor-heavy reference context
- more visible loudness and true peak detail
- less mascot/playful emphasis
- more mastering-style readouts
- more engineering labels

This should not be a separate primary mode.

It should be a presentation profile inside the settings/layout system.

## Recommended Mapping For Layouts A/B/C/D

This directly answers the intent in your notes.

### Layout A

Name:

- `A: Simple`

Audience:

- beginners
- first-glance diagnosis

Behavior:

- large halo
- plain-English state
- minimal secondary metrics

### Layout B

Name:

- `B: Guided`

Audience:

- beginner/intermediate

Behavior:

- strong halo
- more visible supporting metrics
- still plain-English first

### Layout C

Name:

- `C: Advanced`

Audience:

- intermediate/advanced

Behavior:

- more analyzer detail
- more card detail
- more compare/reference context

### Layout D

Name:

- `D: Pro`

Audience:

- professional/more mastering-oriented workflow

Behavior:

- more engineering granularity
- more corridor detail
- more candlestick and loudness detail

## Recommended Default Settings

These should be the shipping defaults unless testing shows otherwise.

### Default startup

- mode: Analyze
- layout: B
- view level: Beginner
- pulse focus: Loudness
- helper text: On
- plain-English labels: On
- background spectrum: On
- live session candle: On
- session history: On
- secondary engineering meters: Off

Why:

- this gives beginners immediate clarity
- it still keeps the product feeling alive

## Menu Labels That Should Be Used

Your notes are clear that labels matter.

Use labels like:

- `Tone`
- `Stereo`
- `Loudness`
- `Dynamics`
- `What This Means`
- `Target Range`
- `Too Little`
- `Within Range`
- `Too Much`
- `Live Mix`
- `Reference Target`
- `Mix A`
- `Mix B`

Avoid labels like:

- `Optimization opportunity`
- `Neural delta`
- `AI confidence field`
- `Spectral centroid deviation`

unless they are inside advanced glossary/help and translated into plain English immediately beside them.

## What Should Not Be In The Menus

To keep the product professional, the following should not be exposed casually.

### 1. Fake analyzer toggles

No menu item should enable decorative fake movement.

### 2. Provider switching if product is OpenAI-only

If the rule remains OpenAI-only:

- no provider dropdown
- no model roulette UI

### 3. Reference fake-data fallback

If no reference exists:

- show neutral live mode
- do not add a toggle to invent targets

### 4. Duplicate controls in multiple places

Examples:

- pulse mode should not live in top bar, options menu, and settings page all with equal weight
- one location should be the quick control
- the settings page should only set the default

## Recommended Implementation Order

When this menu system is eventually built, do it in this order.

### Phase 1. Clean top bar

- keep mode buttons
- rename layout variants with clearer helper meaning
- add `Options`
- add `Settings`
- add `Help`

### Phase 2. Move context controls into proper mode-specific panels

- Analyze controls
- Compare controls
- Reference controls

### Phase 3. Build the quick Options menu

- only high-frequency session toggles

### Phase 4. Build the full Settings window

- multi-page
- persistent defaults

### Phase 5. Add beginner/advanced gating

- hide deeper controls until requested

### Phase 6. Add help/glossary integration

- color legend
- meter explanations
- plain-English definitions

## Final Recommended Menu Hierarchy

For shipping, the plugin should feel like this:

### Top row

- Analyze
- Compare
- Reference
- Layout
- Options
- Settings
- Help

### Below top row, context-sensitive

- Analyze controls or Compare controls or Reference controls

### Options popup

- fast session toggles only

### Settings window

- General
- Display
- Metering
- Analyze
- Compare
- Reference
- History
- Aifred
- Performance
- Accessibility
- Files

That structure is the cleanest match for your handwritten/menu notes and the current plugin architecture.
