// ─────────────────────────────────────────────────────────────────────────────
// Sora 2nd DoF gather (bokeh) pass -- per-game shell only.
//
// The whole algorithm is not here. It lives in dof/dof_common.hlsli and is shared
// verbatim with the other gather pass.
//
// This pass is also what Sora 1st uses: it has a CoC pass of its own (0x1CA8DE95)
// but no gather of its own, so addon.cpp points both games at this CRC.
//
// This pass has NO per-game content at all: it declares no cb_scene, and the two
// files it replaces were byte-identical. Everything that used to be duplicated
// here -- GatherDOFImproved, the Improved dispatch path, and the preserved 3Dmigoto
// vanilla path -- is now in the common file.
//
// Do not re-add the algorithm here. This file is gated on compiling to the same
// bytecode as before the extraction, modulo the DXBC container hash and the
// D3D9 debug-annotation ordering FXC is free to vary across an include boundary.
// ─────────────────────────────────────────────────────────────────────────────
#define DOF_PASS_GATHER
#include "../../dof/dof_common.hlsli"
