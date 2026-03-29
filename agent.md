AIFR3D Repo Workflow

Purpose
- Keep one release branch, one integration branch, and two product branches with short-lived work branches.
- Keep DSP math authoritative, references curated, and UI/advisory behavior grounded in live analysis.
- Prevent direct edits to release branches and avoid "mystery state" in git history.

Canonical Branch Topology
- `main`
  - Release branch.
  - Tracks the newest known-good state.
  - Only receives tested merges.
- `dev`
  - Main integration branch.
  - Receives tested merges from the product branches.
- `vst-standalone`
  - Owns JUCE plugin, standalone app, DSP, metering, analysis, advisory integration, reference pool loading, and desktop UI.
- `website-admin`
  - Owns website, Cloudflare worker, server, Android admin app, webhooks, admin controls, and remote chat/config surfaces.

Important Naming Correction
- `origin` is the remote, not a branch.
- Use `main` as the branch name and `origin/main` as the remote-tracking branch.

Allowed Work Branches
- Branch off `vst-standalone` for:
  - DSP fixes
  - metering fixes
  - UI fixes for plugin or standalone
  - reference pool fixes
  - advisory/chat fixes tied to desktop products
- Branch off `website-admin` for:
  - admin app changes
  - website changes
  - server/API/chat transport changes
  - webhook changes
  - deployment/configuration changes
- Branch off `dev` only for:
  - repo-wide refactors
  - cross-product integration changes
  - branch reorganization work

Work Branch Naming
- `fix/vst-standalone/<short-slug>`
- `feat/vst-standalone/<short-slug>`
- `refactor/vst-standalone/<short-slug>`
- `fix/website-admin/<short-slug>`
- `feat/website-admin/<short-slug>`
- `refactor/website-admin/<short-slug>`
- `fix/dev/<short-slug>`
- `feat/dev/<short-slug>`
- `refactor/dev/<short-slug>`

Branch Rules
- Do not commit directly to `main`.
- Do not commit directly to `dev` except controlled merge commits or branch-organization commits.
- Do not make large mixed-scope commits.
- One bug family or one feature family per work branch.
- Rebase or merge from parent branch before opening a merge back into that parent.
- Merge work branch back into its direct parent first.
- After the parent branch is stable, merge the parent into `dev`.
- After `dev` passes build and test gates, merge `dev` into `main`.

Required Validation Gates
- `vst-standalone` merge gate:
  - Configure and build desktop targets
  - Run DSP/metering tests
  - Run canonical pipeline smoke tests
  - Confirm no fake meters, fake references, or scripted offline advisory outputs
- `website-admin` merge gate:
  - Run server/API checks
  - Validate chat transport endpoints
  - Validate webhook configuration paths
  - Validate admin app build path
- `dev` merge gate:
  - Run full relevant test suite
  - Verify branch integration does not fork product behavior
  - Verify shared contracts stay consistent
- `main` merge gate:
  - Tag known-good release state
  - Update release notes/build notes
  - Confirm installer/build artifacts come from tested commit only

Product Layer Ownership
- Layer 1: Core measurement truth
  - Canonical DSP math
  - loudness
  - true peak
  - FFT/spectrum
  - stereo and phase metrics
  - reference pool schema and loading
- Layer 2: Product implementation
  - VST
  - standalone
  - desktop UI
  - advisory attachment to live analysis
- Layer 3: Platform and operations
  - website
  - admin app
  - server
  - Cloudflare worker
  - webhooks
  - auth/config
- Layer 4: Release control
  - branch policy
  - tests
  - build scripts
  - release notes
  - deployment discipline

Non-Negotiable Engineering Rules
- No fake meter motion.
- No cached metric displays pretending to be live.
- No fabricated reference data.
- No scripted offline chatbot fallback in production.
- No preset generation from AI output.
- No DAW host manipulation from advisory output.
- Reference pools may define context and targets only; they must never overwrite measured values.
- All advisory output must be grounded in real measured metrics from the live buffer and clearly marked when something is inferred rather than measured.

Immediate Execution Plan
1. Freeze architecture drift.
   - Choose one canonical DSP/metering path.
   - Remove or quarantine duplicate simplified analyzer paths.
2. Restore reference-pool-first workflow.
   - Validate canonical pool loading.
   - remove fake fallback references
   - make reference corridors optional but truthful
3. Repair measurement correctness.
   - true peak
   - LUFS
   - FFT banding
   - stereo width
   - correlation
   - crest factor vs dynamic range separation
4. Repair meter behavior.
   - per-meter ballistics
   - stable but accurate smoothing
   - correct UI update cadence
   - real-unit candles only
5. Repair AIFR3D brain grounding.
   - remove scripted fallbacks
   - remove preset/action chains
   - bind advisory inputs to live DSP and validated reference context
6. Repair test and build discipline.
   - compiler/toolchain setup
   - golden-reference metering tests
   - branch gates in CI or scripted local validation
7. Merge upward.
   - work branch -> product branch
   - product branch -> `dev`
   - `dev` -> `main`

Merge Sequence
1. Create or rename `main`.
2. Create `dev` from current stable working branch.
3. Create `vst-standalone` from `dev`.
4. Create `website-admin` from `dev`.
5. Move active work into short-lived child branches.
6. Merge child branches into their parent product branch.
7. Merge both product branches into `dev`.
8. Run full validation.
9. Merge `dev` into `main`.

Release Discipline
- `main` must always be the latest tested working state.
- If something is experimental, it does not belong on `main`.
- If something changes DSP math, meter scaling, reference logic, or advisory grounding, it requires targeted validation before merge.
