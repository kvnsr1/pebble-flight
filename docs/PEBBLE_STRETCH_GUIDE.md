# Pebble Motion Guide for Codex

Reusable guidance for building directional navigation, icon transitions, and focus changes in native Pebble apps.

## 1. Purpose and scope

Use this document as an implementation brief, not as a requirement to reproduce a weather app. The reference pattern comes from Dribble's hourly navigation: a small neighboring icon grows into the focused position while the previously focused icon shrinks into a neighboring position. A short delay between those movements, plus staggered movement within each vector image, gives the transition a flowing, elastic quality.

Apply the underlying relationship to transit departures, flights, calendar events, music, tasks, cards, settings, or other ordered content. Choose motion that communicates the app's actual navigation model. A three-slot carousel is one application; apps with a single detail view or a conventional list need different layouts.

This guide distinguishes **observed reference behavior** from **recommended implementation choices**. Timing defaults below are starting points, not Pebble platform requirements. The reference was inspected at commit `d84b9317b0d63b6ad6cd8d2f378918cff3da20c3` on October 21, 2026.

## 2. What the motion should communicate

The user's selection changes, and the visual emphasis travels with it. The incoming item becomes larger or more prominent; the outgoing item becomes smaller or less prominent. The relationship between items remains easy to follow throughout the transition.

Use three independent concepts:

1. **Translation:** move an element between meaningful layout positions.
2. **Scale:** change its prominence as it enters or leaves focus.
3. **Stagger:** offset the start times of related movements so they follow one another.

An optional fourth concept, **vector deformation**, staggers groups of points inside the icon itself. Parts of the shape move before other parts catch up. This creates the stretchy quality seen in Dribble.

These operations preserve an icon's identity. A sun moving and growing does not morph into a cloud. If the destination uses another symbol, exchange assets at a defined handoff or use a separate transition designed for that change.

## 3. Separate navigation semantics from animation direction

Represent navigation as actions such as `NEXT_ITEM`, `PREVIOUS_ITEM`, `OPEN_DETAIL`, and `BACK`. Map buttons to those actions, then choose the appropriate visual movement. Avoid using `UP` to mean both a button press and the physical direction in which graphics move.

For a vertical layout with previous, focused, and next slots:

| User action | Incoming item | Outgoing focused item | Focus change |
| --- | --- | --- | --- |
| Next item, commonly Down | Bottom slot → focus | Focus → top slot | Incoming grows; outgoing shrinks |
| Previous item, commonly Up | Top slot → focus | Focus → bottom slot | Incoming grows; outgoing shrinks |

Notice that pressing Down can move the icons upward. The motion follows the content's movement through the layout.

For a horizontal layout, rotate the spatial mapping to left/right. For a detail screen, a directional slide or a short emphasis change may communicate the action better than a carousel. Keep navigation direction consistent across the app.

## 4. Build the basic transition first

Start with whole-icon translation and scaling. Add vector deformation only after selection, layout, rendering, and cleanup work reliably.

A transition needs:

- A stable identity for each participating item.
- The source and destination selection.
- Start and end rectangles for the incoming and outgoing visuals.
- References or owned copies of the source assets.
- A motion profile and a single completion owner.

The typical sequence is:

1. Snapshot the visible source content before changing or releasing its resources.
2. Resolve the destination content and calculate its layout.
3. Start the outgoing icon's movement immediately.
4. Start the incoming icon after a short delay.
5. Reveal each destination visual only when its animated counterpart hands off.
6. Restore normal rendering, release temporary resources, and return to idle.

Keep the authoritative selection and displayed content coherent. One workable policy is to commit the destination selection when the transition starts, retain source snapshots for animation, and block further single-step navigation until completion. Another policy can work, but define it explicitly.

Avoid showing a stationary destination icon underneath its moving copy. Suppress only the normal visuals replaced by temporary animation layers. Keep a valid visual present during the delay before an incoming icon starts.

The far neighboring slot can appear at completion or receive its own small entrance. Animate it only if that movement helps explain the change.

## 5. Add the stretchy vector effect

Pebble Draw Command assets provide vector paths that can be manipulated through `GDrawCommandImage` and the draw-command APIs. Dribble's custom KiMaybe code groups an image's vector points into spatial bands and animates those groups with different start times.

These are **groups of path points**, not cropped bitmap strips. The connected path deforms as some points advance ahead of others.

### Preparation

For each participating icon:

1. Obtain an owned mutable copy of the vector image, or otherwise guarantee exclusive ownership during the transition.
2. Preserve an immutable copy of every original point needed for the transformation.
3. Assign each point a band index using its original coordinate along the chosen sweep axis.
4. Precompute its transformed start and end positions.
5. During animation, interpolate from those saved positions, never from the previous frame's modified points.

Four bands are a practical initial choice. For a vertical sweep, use the original Y coordinate; for a horizontal sweep, use X. Clamp the band index so points on the far boundary stay within `0..band_count-1`. Handle negative coordinates and zero-sized bounds explicitly.

Reverse the band order to reverse which edge leads. The sweep direction inside an icon is separate from its overall travel direction. Dribble uses a bottom-to-top band sequence for the Up action and a top-to-bottom sequence for Down, even though the icons travel in the opposite direction through the slots. Treat this as an artistic choice, not a universal mapping rule.

### Point transformation

For an icon with source bounds `(bx, by, bw, bh)`, map each original point into the start and destination rectangles. Preserve the intended aspect ratio and alignment; center or anchor the image within the rectangle as the design requires.

For simple uniform scaling, the conceptual mapping is:

```text
start_point = start_origin + (original_point - bounds_origin) * start_scale
end_point   = end_origin   + (original_point - bounds_origin) * end_scale

local_time  = clamp((elapsed_ms - icon_delay_ms - band_index * band_stagger_ms)
                    / band_duration_ms, 0, 1)
progress    = ease(local_time)
draw_point  = lerp(start_point, end_point, progress)
```

This is pseudocode, not a drop-in SDK implementation. Coordinate conversion must respect the asset's draw-command format, including precise fixed-point coordinates where present. Preserve command attributes, path closure, fill rules, and colors. Decide whether stroke widths remain constant or scale; test the result at the smallest icon size.

Use sufficiently wide signed intermediate arithmetic. Precompute expensive transforms outside frame callbacks where possible. Keep point counts and temporary copies bounded.

## 6. Timing and easing

Keep the timing parameters independent and name them for what they actually control.

| Parameter | Suggested starting value | Purpose |
| --- | --- | --- |
| Band count | 4 | Amount of internal deformation |
| Outgoing icon delay | 0 ms | Immediate response to the button |
| Incoming icon delay | 100 ms | Lets the outgoing item lead |
| Movement duration per band | 120 ms | How long each point group moves |
| Delay between bands | 20 ms | Strength of the stretch |
| Small/large icon relationship | About 1:2 | Clear change in focus; adapt to layout |
| Easing | Explicit ease-in-out | Smooth acceleration and settling |

With those illustrative values, each icon needs `120 + 3 × 20 = 180 ms`; the incoming icon ends at about `100 + 180 = 280 ms`. The complete transition must wait for the longest participating animation, including text and backgrounds.

For arbitrary settings:

```text
icon_end_ms = icon_delay_ms
            + (band_count - 1) * band_stagger_ms
            + band_duration_ms

transition_end_ms = maximum end time across every participating visual
```

Explicit easing makes behavior predictable. Start without overshoot; staggered points already create elasticity. Add a small overshoot only when it improves the particular element. Keep scale positive and content within safe bounds. If the SDK's normalized progress type cannot represent overshoot safely, evaluate an overshoot function in a separate signed representation rather than returning an out-of-range normalized progress value.

Increase the band delay for stronger stretching, or reduce it for a more unified icon. Increasing band count also changes total duration and CPU/memory work. Recalculate the timeline whenever these parameters change.

## 7. Native Pebble implementation structure

Use native C rendering and the Pebble animation/layer APIs. A web prototype can demonstrate the behavior, but it does not replace the native implementation.

A reusable separation of responsibilities is:

| Component | Responsibility |
| --- | --- |
| App model | Selection, content identities, navigation boundaries |
| Layout | Slot rectangles, anchors, screen-specific spacing |
| Asset provider | Load resources and document ownership |
| Motion controller | State, timeline, cancellation, completion |
| Vector transformer | Point snapshots, band assignment, interpolation |
| Renderer | Draw static content or temporary animated copies |
| Motion profile | Durations, delays, easing, enabled effects |

Treat these as module boundaries, not mandatory filenames. Reuse an existing project's structure when appropriate. Keep app-specific fields such as forecast hours, flight numbers, or task status outside the generic animation engine.

Use property animations for ordinary layer-frame movement where suitable. Internal vector deformation requires custom animation updates that modify the copied draw commands and mark the relevant layer dirty. A single master timeline can compute all band progress values, or separate animations can drive individual bands. Either approach must share one reliable completion policy.

Avoid combining layer translation with vector coordinates that already include the same translation. Choose one consistent coordinate system for each animated object.

## 8. State, input, and resource ownership

A small controller can use `IDLE`, `PREPARING`, `ANIMATING`, and `FINISHING` states. If asynchronous content loading is needed, give that loading behavior its own explicit policy.

For ordinary single presses, choose one input policy: ignore presses while busy, queue one next intent, or interrupt and retarget from the current visual state. Ignoring input while busy is the simplest baseline. Do not accidentally accumulate a long navigation queue.

For held buttons, consider fast scrolling with simpler motion or immediate updates. Respect list boundaries. Wraparound is a product decision; a jump from last to first often benefits from a snap or a distinct transition rather than pretending the items are adjacent.

Resource rules:

- Never mutate a shared cached vector image unless exclusive ownership is guaranteed.
- Retain source assets until all readers and animations have finished.
- Load or clone assets during preparation, not repeatedly inside draw callbacks.
- Handle missing assets or allocation failures by showing a valid static destination.
- Cancel pending timers and scheduled animations when leaving the screen.
- Complete exactly once, including when canceled, interrupted, or partially initialized.
- Release temporary images, point buffers, animation objects, and layers according to the target SDK's ownership rules.
- Avoid destroying an animation from inside its own update/teardown callback if that would invalidate an active callback; defer cleanup when needed.

For cancellation, define whether the UI snaps to the source or destination. The model, text, focus indicator, and icon must all settle on the same item.

## 9. Adapt the pattern to the app and device

Use screen dimensions and app layout tokens to calculate positions. Do not assume a 144 × 168 rectangular display, fixed margins, or that every supported device can afford the same motion cost. Account for round displays, clipping, safe text areas, and different pixel densities.

The reference uses 25 px neighboring icons and a 50 px focused icon. Preserve the hierarchy when useful, but choose sizes appropriate to each screen and asset. Keep icons legible in both color and monochrome.

Offer reduced or disabled motion where the app supports it. On constrained targets, simplify to whole-icon movement or static updates. Aplite disables animation in Dribble; that is a decision in the reference app, not a statement that Pebble's animation APIs are universally unavailable on Aplite.

| App situation | Useful adaptation |
| --- | --- |
| Ordered departures, flights, events | Move the neighboring item into focus and grow its icon |
| Music or media selection | Animate artwork or a symbol; keep essential labels readable |
| Single detail screen | Use a directional entrance or brief emphasis rather than inventing neighboring slots |
| Dense menu | Move the selection indicator; reserve icon deformation for a small accent |
| Status refresh on the same item | Use a restrained update; avoid suggesting navigation occurred |
| Bitmap-only assets | Use supported bitmap/layer movement or prepared size variants; vector deformation requires vector paths |

Text can move in parallel or use a short stagger, but it should not inherit internal vector distortion. Background transitions are optional. Keep them subordinate to the navigation and suitable for monochrome screens. Essential information should remain understandable throughout the motion.

## 10. Reference implementation: what to learn and what to change

Dribble supplies a useful native example, not a production-ready universal animation framework.

Observed setup:

- Three icon positions: previous, current, next.
- Small icons are 25 px; the focused icon is 50 px.
- The outgoing icon starts immediately; the incoming icon is delayed 100 ms.
- Both icons use simultaneous translation and scaling.
- KiMaybe divides vector points into four bands.
- `KM_DURATION_MS` is 200, but it is not the actual total icon duration: the transformer divides it by four for 50 ms per band and uses `200 × 0.15 = 30 ms` between band starts.
- Nominally, the last band would finish after `50 + 3 × 30 = 140 ms`, or 240 ms for the delayed incoming icon.
- Text has a separate 300 ms animation and its own easing; those constants should not be mistaken for the icon's timing.

**Completion detail to fix when adapting:** the inspected transform code invokes its finished callback when band index `KM_LINEAR_SLICES / 2` completes. With four bands, this is index 2, before index 3 finishes. The image controller then hides the temporary visual, and later cleanup can dispose of remaining work. Do not copy this completion condition. Complete after all scheduled bands finish, or after the master timeline reaches the true endpoint. The nominal timing above describes the scheduled bands, not a guarantee that every band remains visible for its full duration in the reference.

The reference also contains weather-specific precipitation layouts and accessory-icon offsets. Replace those with generic layout/anchor inputs; do not carry those special cases into unrelated apps.

## 11. Implementation checklist for Codex

Before coding, inspect the existing project, its target SDK, supported platforms, rendering architecture, asset formats, and repository instructions. Read the installed SDK headers/docs for exact API signatures and ownership behavior. Verify the reference revision if using newer source. Check the repository's license and asset permissions before copying implementation or resources; this guide does not grant reuse rights.

Then:

1. Identify the navigation relationship and define source/destination layouts.
2. Implement correct static rendering and whole-icon translation/scale.
3. Add the incoming/outgoing stagger.
4. Add optional vector-point bands behind a configurable motion profile.
5. Coordinate text, backgrounds, and focus indicators with one completion owner.
6. Handle cancellation, missing resources, repeated input, and reduced motion.
7. Build for the app's declared targets and inspect in the available emulator/device environments.

Verify both directions, first/last-item behavior, held buttons, rapid presses, repeated navigation cycles, leaving mid-animation, missing icons, and fallback rendering. Inspect small/large icon legibility, vector-path deformation, clipping, text consistency, completion timing, and memory stability. Test color, monochrome, round, and dense displays when those are supported targets. Report which targets were built and which were visually checked; identify any unavailable validation.

## 12. Copy-ready implementation brief

> Use this guide to add motion appropriate to this native Pebble app's navigation model. Inspect the existing project and target SDK first. Preserve item identity as focus changes, using translation and scaling where the layout supports them. Let the outgoing visual lead and stagger the incoming visual by roughly 100 ms. Where suitable PDC vector assets are available, optionally add four spatial groups of vector points with short staggered starts to create a restrained stretchy effect. Start with 120 ms per band and 20 ms between bands, then tune for the device and content. Keep all parameters configurable. Separate navigation semantics, layout, app data, asset ownership, and animation state. Use owned point snapshots, explicit easing, one reliable completion owner, and deterministic cancellation. Adapt sizes and positions for each declared target, provide a simpler-motion fallback, and verify both directions and resource cleanup. Treat the Dribble source as a behavior reference; correct its early-completion condition and remove weather-specific assumptions. Deliver the native implementation with build results and a concise account of visual validation and limitations.

## 13. Source map

All links below are pinned to the inspected commit so the observations remain reproducible.

- [Dribble README](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/README.md): project overview and description of KiMaybe.
- [Viewer and button handling](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/windows/viewer.c): navigation mapping, input gating, held-button behavior, wrapping, animation settings.
- [Image animation controller](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/animation/image_animation.c): start/end rectangles, incoming/outgoing stagger, temporary images, progressive handoff.
- [Vector transformation](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/kimaybe/transform.c): point grouping, sweep order, interpolation, scheduling, completion condition.
- [KiMaybe types and band count](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/kimaybe/kimaybe.h).
- [Layout](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/layout.h): icon dimensions and platform-aware positions.
- [Common timing](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/animation/animation.h), [text animation](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/animation/text_animation.c), and [custom curves](https://github.com/erlog135/dribble/blob/d84b9317b0d63b6ad6cd8d2f378918cff3da20c3/app/src/c/gfx/animation/animation.c): companion effects that are separate from the vector-band timing.
