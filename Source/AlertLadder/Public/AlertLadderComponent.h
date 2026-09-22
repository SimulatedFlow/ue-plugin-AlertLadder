// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlertLadderTypes.h"
#include "AlertLadderComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAlertRungChangedSignature,
	EAlertState, From, EAlertState, To, EAlertChange, Why);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAlertSearchStartedSignature, FVector, LastKnown);

/**
 * UAlertLadderComponent
 *
 * Put it on an enemy. Report stimuli from whatever perception you already use; the component
 * decides what rung that means and how long it lasts.
 *
 * It does not see, hear or move anything.
 */
UCLASS(ClassGroup = (AlertLadder), meta = (BlueprintSpawnableComponent))
class ALERTLADDER_API UAlertLadderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlertLadderComponent();

	/**
	 * Something happened, at this strength, for this long.
	 *
	 * Strength is 0..1 - a distant footstep is not a muzzle flash. Call it every frame the stimulus
	 * is present; the delta is how long it has been present this frame.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlertLadder")
	void ReportStimulus(float Strength, float DeltaSeconds);

	/**
	 * The target is visible at this position, right now.
	 *
	 * This is the ONLY thing that moves the last known position. Updating it while the target is
	 * out of sight is the difference between a guard searching and a guard cheating.
	 */
	UFUNCTION(BlueprintCallable, Category = "AlertLadder")
	void ReportSeen(FVector TargetLocation, float Strength = 1.0f, float DeltaSeconds = 0.0f);

	/** Somebody else's alarm. Raises this AI's rung to at least Floor, and never lowers it. */
	UFUNCTION(BlueprintCallable, Category = "AlertLadder")
	void ShareAlarm(EAlertState Floor, FVector LastKnown, bool bTakeTheirPosition = true);

	/** Tell every member of the group at once. */
	UFUNCTION(BlueprintCallable, Category = "AlertLadder|Group")
	void RaiseGroup(EAlertState Floor);

	UFUNCTION(BlueprintCallable, Category = "AlertLadder|Group")
	void JoinGroup(UAlertLadderComponent* Other);

	UFUNCTION(BlueprintCallable, Category = "AlertLadder|Group")
	void LeaveGroup();

	UFUNCTION(BlueprintPure, Category = "AlertLadder")
	EAlertState GetRung() const { return State.Rung; }

	UFUNCTION(BlueprintPure, Category = "AlertLadder")
	float GetAwareness() const { return State.Awareness; }

	UFUNCTION(BlueprintPure, Category = "AlertLadder")
	const FAlertState& GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "AlertLadder")
	FAlertRules GetRules() const;

	/** Where to look. Meaningless until there has been contact - check bHasLastKnown. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder")
	FVector GetLastKnownPosition() const { return State.LastKnownPosition; }

	/** How wide the search has spread by now. */
	UFUNCTION(BlueprintPure, Category = "AlertLadder")
	float GetSearchRadius() const;

	/** Back to unaware, and forget the last known position. For a respawn or a new patrol. */
	UFUNCTION(BlueprintCallable, Category = "AlertLadder")
	void ResetAlert();

	/** Step the clock by hand. Public so a fixed-step server or a demo can drive it. */
	UFUNCTION(BlueprintCallable, Category = "AlertLadder")
	void AdvanceTime(float DeltaSeconds);

	UFUNCTION(BlueprintCallable, Category = "AlertLadder")
	void SetAutoTick(bool bEnabled) { bAutoTick = bEnabled; }

	UPROPERTY(BlueprintAssignable, Category = "AlertLadder")
	FAlertRungChangedSignature OnRungChanged;

	UPROPERTY(BlueprintAssignable, Category = "AlertLadder")
	FAlertSearchStartedSignature OnSearchStarted;

protected:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlertLadder")
	bool bOverrideRules = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlertLadder", meta = (EditCondition = "bOverrideRules"))
	FAlertRules RuleOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AlertLadder")
	bool bAutoTick = true;

private:
	UPROPERTY()
	FAlertState State;

	/** Weak on purpose: a dead pack member must not keep the pack alive. */
	UPROPERTY()
	TArray<TWeakObjectPtr<UAlertLadderComponent>> Group;

	void Uebernehmen(const FAlertState& Neu, EAlertChange Warum);
};
