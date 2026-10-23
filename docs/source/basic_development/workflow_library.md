# Workflow Library and the two-view benchmark

PoMVG GUI separates two responsibilities:

- **Plugin Test Lab** validates one plugin's ABI, dependencies, loading contract, and plugin-owned tests.
- **Workflow Library** provides reusable, multi-module `.posflow` experiments.

Open the library from the **Workflow** button in Behavior Workspace or from **File → New from Workflow Library…**. Under **Test**, select **Two-view Estimation Benchmark** and create an editable project.

## Built-in Workflows

### Combine: current default v27

The ETH3D, Strecha and 1DSfM entries currently default to **v27.0.0**, selected by `workflows/profiles/current-combined.json`. ETH3D and Strecha share HIGH, a 0.75 match ratio, mutual matching, up to 30,000 features per image and 8 threads; 1DSfM reads official features and matches. Existing projects are not migrated automatically.

### Combine: historical v23 (certified B / sparse demand / PA20)

Under **General workflows → SfM**, choose **PoMVG Combine — ETH3D**, **Strecha**,
or **1DSfM**. The historical **v23: Certified B / sparse demand / PA20** used `po_core 97ea607c` (solver source equivalent to `e2dbfe57`).
Historical packages and existing editable projects are preserved.

Create a new editable project, open the leftmost Loader's InputControl, associate
the shared dataset root and choose `specific_data`. Defaults are `meadow`,
`castle-P30`, and `Alamo`, respectively. ETH3D/Strecha require images and calibration;
1DSfM uses official feature/match metadata and does not require images. Each run
processes one scene; add an explicit scene Loop for a batch. The built-in package
contains no host paths and requires no historical experiment checkpoint.

Two-view scheduling stays ON and explicitly uses `pair_scheduler_mode=demand`.
The minimum raw match count is 20. Higher-match candidates are preferred while
bounded waves fill coverage, degree and cycle deficits in the accepted-pose graph;
pairs with at least 100 matches are also selected on demand. Existing
geometric/inlier admission gates still decide which estimated edges are published.
Historical v21 and v22 packages are retained. Compared with v22, only the
scheduler mode changes; the certified B backend is unchanged.

The frontend produces measured relative poses and RA. Raw matches and features
enter `raw_candidates` TrackBuilder, with verified two-view matches as seed Prior.
Candidate observations still require geometric certification before optimization
or HAR. Combine retains the reviewed LiGT / PoseLib P3P+PA2 / GroupSAC routing,
Core minimum 10 G3 inliers, geometry evidence, R review before observation deactivation,
and the boundary check. Initial PA G2 translation is disabled. Per-batch global
two-step refinement stays enabled without a local window, followed by Fresh5,
PA20 and HAR2/G2 cleaning. Later observation budgets, PCG, phase-end recovery and
point-elimination experiments remain disabled. GT is evaluation-only.

Output controls save frontend checkpoints, intermediate/final poses, final valid
Tracks, quality certificates and a PLY point cloud. Preserve C/R/U/A roles and
evaluate C/R/U. Reuse exported frontend checkpoints for later controlled comparisons;
recomputing the image frontend does not promise byte-identical historical results.

Packages and the default version are content-bound by the App-owned catalogue.
Check them with `python3 -B tools/generate_builtin_sfm_workflows.py --check --current-combined`
and `python3 -B tools/generate_workflow_families.py`. Reopen the workflow library
after updating the GUI and create the current default project; v23 remains historical, and open projects are not migrated.

### Ours: PoseEngine stable-R2 (Strecha)

The general-workflow package **Ours: PoseEngine stable-R2 Reconstruction -
Strecha** freezes

```text
Strecha loader → OpenMVG SIFT
  → OpenCV USAC-MAGSAC (rotation reservoir) → Chatterjee RA → Track=2
  → PoSDKPoseEngine(initial, backend_profile=stable_r2)   # stable-R2 + PA-1
  → Fresh5-1
  → PoSDKPoseEngine(expansion, backend_profile=stable_r2) # Frontier recovery + PA-2
  → Fresh5-2
  → posdk_reconstruction (HAR + attraction + clean)
```

Each PoseEngine INI keeps an isolated `[PoSDKPoseEngine]` section and selects
the frozen historical stable-R2 backend with `backend_profile=stable_r2`.
It does not embed `[LiGTStable]` / `[method_PA]`, and it does not take the
independent G3-consensus Core. The default scene remains `castle-P30`. GT
poses are evaluator-only.

### Ours: PoseEngine stable-R2 (1DSfM)

The general-workflow package **Ours: PoseEngine stable-R2 Reconstruction -
1DSfM** is the same frozen backend as the ETH3D Images PoseEngine flow, but
the frontend is official 1DSfM features/matches:

```text
1DSfM loader (Alamo features/matches)
  → OpenCV USAC-MAGSAC (rotation reservoir) → Chatterjee RA → Track=2
  → PoSDKPoseEngine(initial, backend_profile=stable_r2)   # stable-R2 + PA-1
  → Fresh5-1
  → PoSDKPoseEngine(expansion, backend_profile=stable_r2) # Frontier recovery + PA-2
  → Fresh5-2
  → posdk_reconstruction (HAR + attraction + clean)
```

There is no Preprocessor. Loader-supplied GT poses, Tracks, and points stay
evaluator-only. Default scene is `Alamo`.

### Ours: SfMEngine P1-full (ETH3D Images)

The general-workflow package **Ours: SfMEngine P1-full - ETH3D Images** keeps the
same image frontend as the 2026-09-10 P1-full run, then collapses the backend
into one `posdk_sfm_engine` node:

```text
ETH3D loader → OpenMVG SIFT (30k)
  → OpenCV USAC-MAGSAC (rotation reservoir) → Chatterjee RA → Track=2
  → posdk_sfm_engine   # frozen R2/PA, Fresh5, weak-pose, HAR+attraction+clean
```

The public INI only has `backend_contract=p1_full`. Child LiGT/PA/Fresh/HAR
options stay inside the plugin. Default scene is `facade`. GT poses are
evaluator-only. Drag **PoMVG SfM Engine** from Modules onto any workspace that
already has tracks, RA poses, cameras, features, raw matches, and Rij.

## OpenCV-PoEngine real-dataset workflows

Under **General Workflows**, PoMVG provides **OpenCV-PoEngine-Strecha**,
**OpenCV-PoEngine-ETH3D**, and **OpenCV-PoEngine-1DSfM**. The prefix identifies
the exact algorithm family rather than acting as a display label. Future
`GNC-RANSAC-PoEngine-*` entries remain separate templates and must not inherit
or overwrite these OpenCV settings. The built-ins contain no owner-machine
`configPath` or `outputBaseDir`; each performs one run by default.

After creating the editable project, open the leftmost DatasetLoader node and
use its InputControl to choose and associate the common parent directory of the
dataset scenes. Strecha defaults to `castle-P30`, with `images` and
`gt_dense_cameras` below the scene. ETH3D defaults to `facade`, with
`images/dslr_images_undistorted` and `dslr_calibration_undistorted` below the
scene. 1DSfM defaults to `Alamo`: double-click its loader, use **Download
dataset** to open the official acquisition page, then associate the root that
contains the downloaded scene metadata. Its images are a separate optional
download for RGB colorization. Change `specific_data` for one other scene, or
add a Var Workspace loop for an explicit multi-scene experiment.

Strecha computes SIFT features and matches through `posdk_preprocessor`; ETH3D
and 1DSfM supply features and matches directly. Every workflow then runs the
verified deterministic profile: 50k OpenCV USAC-MAGSAC at 1 px and seed 31,
six-step PPO angle-2sin/Cauchy refinement, bounded weighted local-cycle LCC
with synchronized match masks, Chatterjee rotation averaging, minimum-length-3
tracks, and `PoRobustSfMEngine`. The backend uses incremental robust LiGT with
normal-PCG, joint R+t Cauchy PA, gauge normalization, consensus
triangulation, and far/structural point filtering (`m3=0.15`). Relative- and
global-pose GT reach `gt_input`. Output Control writes G2O/PLY/PB, and MeshLab
export stays below `./meshlab_export`. ETH3D additionally inserts Fresh5 after
`PoRobustSfMEngine`, then terminates with the same `posdk_reconstruction`
module used by the 1DSfM-like Strecha flow (`har_attraction_clean`).
`matches2tracks` keeps a lossless support-two reservoir so Fresh can recover
observations from the original Tracks plus loader raw matches; PoRobust still
reconstructs at three observations. MeshLab exports the cleaned HAR points;
Output Control still writes the PoRobust poses/points/tracks for comparison.
A local association belongs only to the
materialized project; portable publication still requires signed `.posdata`.

### Chessboard-calibrated image reconstruction

The Developer Workflow Library also provides **Chessboard-Calibrated Image
Reconstruction**. It replaces the Strecha loader with two explicit
`data_images` nodes. Associate the chessboard set with **Calibration Images**
and the separate scene set with **Reconstruction Images**. Before running,
open `method_calibrator` and set the horizontal/vertical inner-corner counts,
physical square size, camera model, and distortion model. The template defaults
to a 9-by-6 inner-corner board with 25 mm squares and a pinhole radial-K3 model.

Only `data_camera_models` crosses from calibration to reconstruction. Board
poses, board features, and board matches keep calibration-view IDs and are
therefore intentionally absent from the SfM branch. Scene images independently
produce SIFT features and matches, which are checked against the calibrated
camera domain before entering the same USAC-MAGSAC, Chatterjee, Track,
`PoRobustSfMEngine`, and MeshLab chain used by the Strecha workflow. PA keeps
intrinsics, distortion, and observation coordinates fixed. Use image sets from
the same physical camera with unchanged focus/zoom, resolution, crop, and
orientation; this workflow does not rescale intrinsics for changed image
geometry. Output Control saves the calibrated camera model together with the
reconstruction poses, tracks, and points.

For 1DSfM, **GLOMAP-style-PoRobust-1DSfM** is the upgraded production profile.
It keeps the PoMVG two-view estimator and PoRobust backend. USAC-MAGSAC first
applies the validated estimator-mask `30 inliers / 0.25 ratio` safety gate; an
explicit calibrated pre-RA verifier independently recounts support at 1 pixel,
reapplies `30 / 0.25`, retains the largest component, and assigns inlier-count
edge weights. Two `solver_profile=glomap` Chatterjee passes are each followed
by a synchronized 10-degree post-RA filter. The twice-filtered graph builds
length-[3,100] core tracks with 10-pixel `merge_nearby` same-view conflict
handling. RobustLiGT uses camera-spread guards. Global PA jointly optimizes R+t
for every participating non-gauge view while keeping every mapped intrinsic/
distortion block and observation coordinate fixed; it uses the 0.25--4.0
solution-scale gate and the complete local view-geometry
gate. Recall round zero clears and rebuilds structure from fixed poses, while
rounds one through three complete, merge, and refine it around three
fixed-intrinsics pose-completion rounds. These stages revalidate the wider
two-view correspondence reservoir, not the post-RA core graph. Support-two
structure is admitted only after the final pose-completion round, under
bilateral nearby-core guidance, and never feeds back to PnP. Supplied 1DSfM
poses remain evaluator-only, while every camera consumer reads the loader's
explicit per-view camera mapping.

### Ours two-stage Pose-only Fresh4-HAR

The built-in **Ours: Two-stage Pose-only Fresh4 HAR - 1DSfM** freezes the
currently reproducible Ellis Island chain and selects `Ellis_Island` by default:

```text
min-10 OpenCV two-view -> Chatterjee RA -> support-2 reservoir
  -> unbounded Incremental LiGT (private Track>=4 control pool)
     + joint R/t pose-only PA1
  -> fixed-pose Fresh4 (raw matches, Track>=3 + verified Track=2, 4 px)
  -> fixed-core bounded delta-t LiGT + staged joint R/t pose-only PA2
  -> fixed-pose Final Fresh2 (verified Track=2 recall, 2 px)
  -> fixed-pose HAR -> Evaluator / PLY / MeshLab
```

The initial Track reservoir disables the `method_matches2tracks` maximum-length
filter. Both LiGT stages likewise set the maximum solver Track length to zero,
and both Fresh transactions set `maximum_component_observations=0`; zero is the
explicit unlimited value in these contracts. Stage one keeps the Ellis-validated
private Track>=4 LiGT control pool. Its `incremental_rank_irls` solve fails
closed below a deliberately permissive `1e-8` global relative-rank threshold,
then PA1 jointly optimizes camera R+t only.

The first fixed-pose Fresh4 transaction reads the loader's immutable raw-match
evidence bank, rebuilds Track>=3 at the 4-pixel gate, and already materializes
verified, feature-ownership-safe Track=2. Those support-two rows are therefore
available to stage two, but they receive weight `0.1` rather than controlling
the skeleton. The bounded stage fixes the high-support core and solves only
rank-certified weak-view translation increments. Its local relative-rank gate
is `1e-6`, delta prior weight is `0.25`, and each camera-center correction is
hard-bounded by `0.05` of the robust scene radius. The emitted pose hierarchy
keeps core views fixed in PA2 and releases only certified weak views for joint
R+t pose-only refinement; Track=2 PA residuals again receive weight `0.1`.
PA2 enables both its whole-stage solution gate and per-view geometry gate, and
disables a second output-gauge normalization so the certified stage-one core is
not recentered or rescaled at the Engine boundary.

Final Fresh tightens the fixed-pose validation gate to 2 pixels and again
retains ownership-safe Track=2. Both Fresh transactions complete their
ownership audit in private staging and require the candidate to retain at least
50% of all input Tracks, 80% of input Track>=3, and 90% of prior Views. A failed
gate rolls back the entire candidate to the input reservoir. HAR then
reconstructs structure with all poses,
per-view camera mappings, intrinsics, distortion, feature pixels, and Track
topology fixed. Neither PA contains point variables, and the Engine's legacy
analytical reconstruction remains disabled. HAR disables surfaces, Rij/tij
edges, ray-tube attraction, and terminal clean by default. The preconfigured
20-pixel discovery/1-pixel feasibility strong ray-tube profile remains an
explicit terminal ablation. Supplied 1DSfM poses, tracks, and points remain
evaluator-only.

The separate paper Strecha workflow pins the manuscript's single-thread,
sample-20, 50000-hypothesis GNC-RANSAC protocol at seed 31 and a normalized
`4e-4` inlier threshold. It performs one final LiRP consensus refit only when
the MSAC score improves, then applies six-step PPO/Cauchy refinement and the
GT-free 0.5-degree `global_sync` gate. Ground truth is evaluator-only, and
accepted-pair coverage must be reported with Relative R/t accuracy.

## LiGT global-translation accuracy workflows

Under **Test**, use **ETH3D LiGT Translation Accuracy** for one locally
associated ETH3D scene or **Simulated LiGT Translation Accuracy** for a
reproducible synthetic matrix. Both terminate at `method_LiGT`, but their
frontends intentionally differ. The ETH3D chain evaluates the complete
Direct-gated frontend:

```text
ETH3D cameras + features + matches
  -> two_view_estimator (LiRPFast + PPOopt + Direct rotation-graph gate)
  -> estimated relative poses -> Chatterjee global rotation averaging
  -> synchronized inlier matches + loader features -> method_matches2tracks
clean tracks + cameras + averaged global R -> method_LiGT
ETH3D global-pose GT -> method_LiGT gt_input
```

The estimator requires at least 201 correspondences per view pair. Its
rotation-graph gate retains the largest connected component and clears
`is_inlier` on every other view pair before `method_matches2tracks` runs. The
largest-component step is specific to global reconstruction: unlike the
pairwise paper protocol, Chatterjee averaging and LiGT require one connected
component to define a single global gauge. Consequently, the LiGT node never
consumes the loader's prebuilt `data_tracks`; its tracks are rebuilt only from
the estimator output retained by the same Direct gate as the relative-pose
graph.

The simulated workflow remains a LiGT-only global-translation isolation experiment
(it does not run global PA):

```text
simulator relative-pose GT -> Chatterjee global rotation averaging
simulator tracks + cameras + averaged global R -> method_LiGT
simulator global-pose GT -> method_LiGT gt_input
```

No `PoGlobalSfMEngine`, position averaging, or bundle-adjustment stage runs
after LiGT, so its translation output remains the evaluated result. Evaluator
converts both estimated and GT poses to camera centers, performs a 7-DoF
similarity alignment to remove the monocular gauge, and records per-camera
`translation_error` together with `rotation_error_deg`. Output Control also
writes `LiGT_global_poses.g2o` below each run's output directory.

The ETH3D template defaults to `facade` and one run. The simulated template
uses 12 views and 1000 points, and crosses pixel noise
`0, 0.5, 1.0, 2.0` with deterministic seeds `41, 42, 43`, for 12 runs. It
feeds exact simulator relative rotations through rotation averaging so the
noise curve measures LiGT's translation estimate from noisy tracks without
mixing in a separate two-view estimator. In contrast, the ETH3D template
measures downstream global translation from the selected LiRPFast + PPOopt
Direct-gate relative-pose and inlier-match frontend.

The built-in workflow connects `visual_simulator` to `two_view_estimator`, including simulator ground truth through `gt_input`. Its default experiment matrix combines five bound estimator variants with four pixel-noise levels and two point counts, for 40 runs. Bound variants keep estimator and algorithm parameters together, preventing invalid Cartesian combinations.

## Selecting coupled variants in Var Workspace

The **Coupled algorithm variants** rows form a user-defined bound dimension,
not ordinary loops. A built-in workflow supplies validated presets. Edit,
duplicate, add, or delete variants in the materialized `.posdk` project, and
bind only parameters that must change together to keep a configuration valid.
For example, `estimator + algorithm` normally belongs in the tuple, while
refinement or optimization switches may remain fixed or become ordinary Loop
axes when they are intentionally swept.

Use the round checks to select variants, add only independent scenario axes such
as `noise_level` and `num_points` below, set `Parallel`/`Jobs`/`Repeat`, verify
the total version count, and click **OK**.
The run count is:

```text
coupled_var × non_coupled_var1 × non_coupled_var2 × … × Repeat
```

Bindings inside one variant form a tuple and are never cross-multiplied. The
in-app **? Guide** explains these steps and shows the current run count. The
editor offers only real Method parameters and validates parameter existence and
duplicate bindings on save. Changes affect only the editable `.posdk`; the
signed, immutable `.posflow` preset is unchanged.

Evaluator reports rotation and translation-direction errors against simulation ground truth. Profiler reports Method and core runtime. A successful workflow proves end-to-end data and output integration; it does not by itself prove accuracy. A small workflow matrix also does not replace the opt-in, high-repeat in-process timing benchmark used for publication-grade microbenchmarks.

## Robust two-view outlier benchmark

The built-in **Robust Two-view Outlier Benchmark** productizes the historical
`test_gnc_ransac.cpp` contract. It uses `opengv_simulator`, whose
`outlier_fraction` parameter generates real synthetic outliers, and connects
its camera, feature, match, and plural relative-pose GT ports directly to
`two_view_estimator`.

This complete matrix is built into the unrestricted source Developer build
only. The Barath, DEGENSAC, and paper-derived QDEGSAC backends are experimental
native plugins and are not all certified for the packaged DMGs or available as
validated installable `.pospkg` files. Consumer and Developer Kit builds
therefore do not advertise a workflow whose exact dependency closure is
unavailable.

Ten coupled variants compare GNC-RANSAC LiRP at sample sizes 20 and 30,
OpenGV Stewenius five-point RANSAC, OpenCV Essential LMedS, OpenCV USAC Default
and USAC MAGSAC, Barath MAGSAC and MAGSAC++, native DEGENSAC, and paper-derived
QDEGSAC. The DEGENSAC and QDEGSAC variants select their distinct plugin IDs
directly and set display-only `algorithm=DEGENSAC` and `algorithm=QDEGSAC`
labels, so Evaluator cannot merge them or inherit the base variant's `fast`
label. Independent axes cover outlier fractions
`0.1, 0.3, 0.5, 0.7` and deterministic seeds `41, 42, 43`, with 300
correspondences and noise 1.0. The default matrix is therefore
`10 × 4 × 3 = 120` runs. Explicit seeds make scenes reproducible and
visible in Evaluator/Profiler tables.
An `assign_value` variable rule copies each `random_seed` into
`two_view_estimator`, fixing the GNC-RANSAC, OpenGV RANSAC, OpenCV USAC
Default/MAGSAC, default `sampler_id=0` Barath, native DEGENSAC, and paper-derived
QDEGSAC sampling sequences as well as the scene without introducing another
Cartesian-product dimension. OpenCV LMedS uses its fixed internal sequence
because its public API exposes no per-call seed. The
OpenGV adapter maps the common iteration budget to `ransac_max_iterations` and
converts the pixel threshold through both cameras' focal lengths instead of
passing `1.0 px` as a normalized bearing residual.

`qdegsac_two_view_estimator` is an independent implementation derived from
Frahm and Pollefeys (CVPR 2006); it is neither the authors' original code nor a
renamed DEGENSAC backend. This workflow's general 3D scenes evaluate integration,
outlier robustness, AUC, and runtime. They do not by themselves validate
QDEGSAC's quasi-degenerate advantage, which requires the plugin's dedicated
dominant-plane tests.

LMedS is only guaranteed when inliers exceed 50%, so a failed or inaccurate
70% outlier cell is an algorithm limitation to report, not a zero-valued result
to synthesize. RHO is deliberately excluded: OpenCV documents it for
`findHomography`, while this benchmark uses a general non-planar 3D scene, and
`findFundamentalMat + RHO` would be an unsupported, mislabeled baseline.

The common maximum RANSAC sampling budget is 50000; QDEGSAC treats it as a
total hypothesis budget shared across full-model estimation, hierarchical
selection, and model completion, rather than 50000 trials per rank.
MAGSAC-only bundle adjustment and adaptive inlier post-processing are disabled
so runtime refers to the robust estimator stage rather than an extra refinement
stage. A missing result remains a failed scenario; it must never be represented
as zero error or a pass.

GNC-RANSAC also declares the native method quality gate explicitly: `1e-3`
for the inner maximum acceptable residual, `5e-3` for the final maximum mean
residual, `0.2` for the final minimum inlier ratio, and `10.0` for the GNC
indicator. These are the defaults of `method_GNC_RANSAC_LiRP`; the wrapper no
longer substitutes a hidden `2e-4` final threshold that rejects otherwise
valid inlier sets in the 1 px noise scenario.

The simulator explicitly converts OpenGV's camera-pose convention to PoMVG's
`x_j = R_ij x_i + t_ij` convention before publishing GT. Without that
conversion the workflow can still complete while Evaluator reports artificial
errors of tens of degrees or nearly 180 degrees from comparing inverse poses.

Var Workspace axes remain ordered structured dimensions in result artifacts.
The default tables therefore expose independent `variant`, `noise_level`, and
`num_points` columns beside Evaluator `Algorithm` and the Method-owned
`EvalCommit`. Profiler uses the same dimensions as aggregation keys, so timing
for different point counts is never averaged together. The legacy
`tagProfileCommit` field no longer rewrites Method configuration.

`.posflow` is an immutable distribution unit, while `.posdk` is editable. Before materialization, the GUI validates the package schema, SHA-256 binding, nodes, connections, declared dependencies, experiment size, parameter bindings, and path policy. Built-in packages are protected by the App signature. External Marketplace packages remain fail-closed until publisher-signature verification is available.

## DatasetLoader InputControl and the dataset service

Add a DatasetLoader to Behavior Workspace and double-click the node. A Loader
that ships `input_controls/input_control.qml` exposes an InputControl beside its
LogViewer. The plugin owns dataset-specific guidance, official-source links,
scene/sequence fields, and expected directory-tree presentation. The App owns
the privileged operation through a node-scoped `datasetContext`.

Choose a local root and click **Associate with node**. Compatibility comes only
from adjacent INI metadata: `@type: path`, `@path_mode: folder`, and a stable
`@dataset_binding`; PoMVG updates only the exact node instance. Signed
Marketplace `.posdata` dependencies are still installed automatically by the
headless dataset service during workflow installation. A signed package
produces a portable `${dataset:...}` declaration; a local folder is host-only
and publishing fails until it is packaged as a signed `.posdata`. Removing the
local-path registry does not make a stale absolute path publishable.

The removed global File-menu window does not remove dataset security services.
Marketplace installation still verifies archive and manifest hashes, publisher Key
ID, license/acknowledgement metadata, and the required directory binding before
any Store commit. Consumer and Developer dataset stores are isolated.

## Exact workflow publication

The Developer publish wizard records the process's actual active plugin set in
`project.posdk`. Core modules are marked `builtin`; dynamic plugins carry their
exact SemVer and immutable generation SHA-256. `workflow create` therefore uses
an exact version constraint for current GUI exports, and `workflow lock` cannot
silently replace a tested `1.x` plugin with a newer `2.x`. Lock resolution uses
SemVer precedence, not lexical ordering, and writes the verified dataset
license back over the temporary `REQUIRED` value before packaging.
