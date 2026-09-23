# AlertLadder

Alert states, last known position and shared alarms for Unreal Engine 5.8.

An AI that forgets as fast as it notices can be out-waited by stepping behind a crate for two
seconds. AlertLadder makes noticing fast and forgetting slow, and it keeps the two apart: awareness
climbs at six times the rate it falls, and it does not start falling at all until a delay has passed.

* Four rungs - Unaware, Suspicious, Searching, Alerted - climbed in one step, descended one at a time
* Hysteresis on every threshold, so a guard on a boundary does not flicker and re-bark every frame
* The last known position moves **only** while the target is actually visible
* The search spreads outwards from that position the longer nobody finds anything
* Shared alarms raise a group's floor and never lower it
* Every rule is a pure function the component and the tests both call

Documentation: https://wiki.teufel-engineering.com/en/AlertLadder/documentation
Support: teufelsilvan@gmail.com

Unreal Engine 5.8 - Win64 - one runtime C++ module - no third-party code - full source included.

<!-- SF-STORE-BLOCK:BEGIN -->
## 🛒 Source-available — see before you buy

This repository contains the **full source** of a commercial Unreal Engine plugin. It is **source-available, not open source**: read it, evaluate it, then buy a license to use it. See **the Fab Content License Agreement / Unreal Engine EULA (purchase required)**.

**Get it / Buy:**
- **Buy on Fab** (this plugin): https://www.fab.com/listings/1a9482f3-ecca-4a77-b0c7-2789abad523b
- Fab store — all our UE5 plugins: https://www.fab.com/sellers/Silvan%20Teufel

### 📬 **Free UE5 Snippet-Pack**

10 ready-to-use C++/Blueprint building blocks (subsystems, versioned saves, async nodes, editor tooling) — MIT licensed. Get it by joining the newsletter — plus a heads-up when something new ships. Double opt-in, unsubscribe in one click, no address sharing.

👉 **[Get the free pack](https://silvan.teufel-engineering.com/newsletter/plugins/?q=gh)**

_© 2026 Silvan Teufel. All rights reserved._
<!-- SF-STORE-BLOCK:END -->
