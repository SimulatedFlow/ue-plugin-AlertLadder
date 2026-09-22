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
