# AlertLadder — Alert States, Last Known Position & Shared Alarms

**An AI that forgets as fast as it notices can be out-waited behind a crate.**

Two seconds of cover and the guard is back to humming. Players find that within an hour and it hollows
out every stealth encounter in the game. AlertLadder keeps noticing and forgetting apart, on purpose
and by a wide margin.

---

## 0. Supported engine and platforms

* Unreal Engine **5.8**
* **Win64.** The plugin's `PlatformAllowList` is Win64 only; Mac and Linux are not supported.
* One runtime C++ module, no third-party code, full source included.
* No dependency on AIModule, the Perception system or Behaviour Trees. Report a stimulus from
  whatever you already use — `AIPerception`, a trace, a trigger volume, a sound event.

---

## 1. The five-minute install

1. Add an **Alert Ladder** component to the enemy.
2. Where your perception reports something, add one line:

```
ReportSeen(TargetLocation)              // the target is visible right now
ReportStimulus(Strength, DeltaSeconds)  // a noise, a body, a door left open
```

3. Bind `OnRungChanged(From, To, Why)` and switch behaviour.
4. When the rung is `Searching`, walk towards `GetLastKnownPosition()` and sweep
   `GetSearchRadius()`.

That is all of it. The component runs its own clock and decays on its own.

---

## 2. The asymmetry: rising is six times faster than falling

```
RisePerSecond      0.6    at full stimulus strength
CalmDelaySeconds   4.0    nothing decays at all until this has passed
FallPerSecond      0.08
```

A guard at full awareness needs under two seconds of clear sight to get there, and over twelve
seconds of nothing at all to come back down — **after** four seconds in which it does not start.

Every stimulus resets the delay. That is what stops a player flickering in and out of cover from
holding a guard permanently half-aware: each glimpse pushes the clock back to zero.

Both functions are linear, so two half-steps and one whole step give the same answer. The plugin
takes the step's start time into account when a step straddles the delay boundary, so a frame rate
that changes does not change how quickly a guard calms down.

---

## 3. The ladder

```
Unaware = 0    Suspicious = 1    Searching = 2    Alerted = 3
```

Climbed in **one step**: a muzzle flash takes a guard from Unaware to Alerted without visiting the
rungs in between.

Descended **one rung at a time**, and only after falling a whole hysteresis band below the threshold.
An AI that goes from hunting to humming in a single frame is the oldest bug in stealth, and it reads
to a player as the game giving up on them.

```
SuspiciousAt 0.25   SearchingAt 0.55   AlertedAt 0.85   Hysteresis 0.08
```

The hysteresis is not decoration. Without it a guard sitting exactly on a boundary changes state every
frame — and every change fires a bark, so the guard stutters a line forty times a second.

`NextRung(Awareness, Current, Rules)` takes the current rung as an input for exactly this reason:
climbing needs the full threshold, descending needs to fall a band below it. The same awareness can
mean two different rungs depending on which way it arrived.

---

## 4. The last known position, and what makes a search honest

`ReportSeen` is the **only** thing that moves `LastKnownPosition`.

That single rule is the difference between a guard searching and a guard cheating. Update the
position while the target is out of sight — which is trivially easy to do by accident, because the
target's transform is right there — and the guard walks to where the player *is*, through walls,
without ever having seen them. Players feel that immediately, even when they cannot name it.

`ReportStimulus` raises awareness without touching the position. A noise makes a guard alert; it does
not tell the guard where you are standing now.

`bHasLastKnown` is false until there has been contact. `GetLastKnownPosition` is meaningless before
then, and returning the origin silently would send a search party to the middle of the map.

---

## 5. The search spreads

```
InitialSearchRadius      250    where the search starts, the moment contact is lost
SearchGrowthPerSecond    120    per second of searching
MaxSearchRadius         1600
```

A search that stays a fixed circle around the last sighting is a search the player waits out by
standing eight metres away. A search that grows is one they have to keep moving away from.

`GetSearchRadius()` is what your behaviour tree uses to pick the next point to check. The plugin does
not move anything and does not know what a navmesh is.

`OnSearchStarted(LastKnown)` fires when the rung reaches `Searching`.

---

## 6. Shared alarms raise the floor and never lower it

```
ShareAlarm(Floor, LastKnown, bTakeTheirPosition)
RaiseGroup(Floor)
JoinGroup(Other) / LeaveGroup()
```

`RaiseToFloor` lifts a rung to at least the floor and leaves anything higher alone. Setting the group
to the caller's level instead — which is the obvious implementation — calms down the one guard who is
already in the fight, which is precisely backwards.

**Raise the alarm on the edge, not every frame.** An alarm re-applied every tick keeps overwriting the
other guards' awareness with the floor value, so the moment one of them decays a point below the rung
it is yanked straight back up and sits there sawing. Call `ShareAlarm` when somebody *starts*
shouting. The shipped demo makes exactly this mistake in its first version and the comment in
`AlertLadderDemoDirector.cpp` records what it looked like.

Group membership is held with weak pointers: a dead pack member does not keep the pack alive.

---

## 7. What AlertLadder is not

* **It does not see or hear anything.** It has no perception, no traces, no cones. You report; it
  decides what that means and how long it lasts.
* **It does not move anything.** It tells you where to look and how wide. Getting there is your
  navigation's job.
* **It is not a behaviour tree.** It pairs with one. `OnRungChanged` is the signal a tree reads.
* **It is not replicated.** Replicate the rung — one byte — not the component. Awareness is a
  server-side number.

---

## 8. Console commands

| Command | What it does |
|---|---|
| `AlertLadder.Dump` | Every AI in the level with an alert component: rung, awareness, time since the last stimulus, time searching, search radius and whether it has a last known position. |
| `AlertLadder.Calm` | Reset every alert component in the level to Unaware. For testing an encounter again without reloading. |

---

## 9. API reference

### `UAlertLadderComponent`

`ReportStimulus(Strength, DeltaSeconds)`, `ReportSeen(TargetLocation, Strength, DeltaSeconds)`,
`ShareAlarm(Floor, LastKnown, bTakeTheirPosition)`, `RaiseGroup(Floor)`, `JoinGroup`, `LeaveGroup`,
`GetRung()`, `GetAwareness()`, `GetState()`, `GetRules()`, `GetLastKnownPosition()`,
`GetSearchRadius()`, `ResetAlert()`, `AdvanceTime(float)`, `SetAutoTick(bool)`.

Delegates: `OnRungChanged(From, To, Why)`, `OnSearchStarted(LastKnown)`.

`EAlertChange` says *why* the rung moved — `Escalated`, `Calmed` or `Shared` — so a bark for "I heard
something" and a bark for "over here!" can differ.

`AdvanceTime` is public and `bAutoTick` can be switched off, for a server on a fixed step, a replay
being scrubbed, or a demo running in an editor viewport.

### `UAlertLadderStatics` — the rules, on their own

`NormaliseRules`, `NextRung`, `Rise`, `Fall`, `SearchRadius`, `RaiseToFloor`, `AwarenessForRung`,
`Advance`, `ApplyStimulus`.

No world, no actor, no perception, no clock. The component calls exactly these and so do the tests,
which is the only way the debug draw and the behaviour cannot drift apart.

`Fall` takes the seconds-since-stimulus **at the start of the step**, not at the end. Passing the end
value makes one long step forget more than several short ones covering the same time.

### Project Settings > Plugins > AlertLadder

`Rules` is the project default; a component can override it with `bOverrideRules` and `RuleOverride`.
`bLogChanges` writes a line whenever an AI changes rung.

---

## 10. The demo level

`Content/AlertLadder/Maps/L_AlertLadderDemo` — two guards on their posts and an intruder that walks
into sight, breaks contact behind a low wall, and reappears on the other side.

Worth watching for, in order:

* **Under two seconds** from the first glimpse to Alerted.
* **The shared alarm.** The moment the left guard reaches Alerted, the right one jumps to Searching
  without having seen anything at all.
* **Contact is lost and the bar does not fall.** It holds through the delay and only then begins to
  sink, far more slowly than it rose.
* **The cross on the floor stops moving** the instant sight is lost, and the circle around it widens.
* **The descent, one rung at a time**, all the way back to Unaware.

The director drives `UAlertLadderStatics` directly, so every bar and every word on the board is the
plugin's own output.

**If the level looks frozen**, the viewport is not set to realtime. Either switch realtime on, or
call `StepDemo(Seconds)` on the director yourself — that is what the screenshot run does.

---

## 11. Troubleshooting

**Awareness never rises.** Nothing is calling `ReportStimulus` or `ReportSeen`, or the strength is
zero. `AlertLadder.Dump` shows the awareness; a value stuck at zero is the first clue.

**Guards notice instantly from any distance.** Scale the strength by distance before reporting it.
The plugin takes the number you give it; a figure at the edge of a cone is not the same stimulus as
one standing in front of you.

**Guards never calm down.** `CalmDelaySeconds` is long and `FallPerSecond` is small — that is the
default and it is deliberate. Check also that nothing is re-sharing an alarm every frame (section 6).

**A guard walks straight to the player through a wall.** Something is calling `ReportSeen` while the
target is out of sight. That call is the only thing that should ever move the last known position.

**The bark fires over and over.** The hysteresis is too small for how fast awareness is moving, or a
shared alarm is being raised every tick.

**The whole pack calms down when one guard does.** Something is assigning the group a rung instead of
raising a floor. `RaiseToFloor` never lowers.

**A guard searches the middle of the map.** `bHasLastKnown` is false — there has been no contact yet.
Check it before using the position.
