// Copyright 2026 Silvan Teufel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlertLadderTypes.h"
#include "AlertLadderDemoDirector.generated.h"

class UTextRenderComponent;

/**
 * Runs the shipped demo: two guards and an intruder that walks into sight and out of it again.
 *
 * The guards are drawn rather than spawned - an editor viewport is where the store images are
 * taken, and spawning actors from a viewport tick would leave them in the level. Every decision
 * comes from UAlertLadderStatics, which is the plugin.
 */
UCLASS(meta = (DisplayName = "AlertLadder Demo Director"))
class ALERTLADDER_API AAlertLadderDemoDirector : public AActor
{
	GENERATED_BODY()

public:
	AAlertLadderDemoDirector();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder Demo",
		meta = (ClampMin = "16.0", ClampMax = "180.0"))
	float CycleSeconds = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder Demo")
	bool bDrawDemo = true;

	/** Let Tick drive the demo. Off when something else steps it - see StepDemo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AlertLadder Demo")
	bool bAutoRun = true;

	/** Advance the demo by exactly this many seconds and redraw. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "AlertLadder Demo")
	void StepDemo(float Seconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlertLadder Demo")
	TObjectPtr<UTextRenderComponent> BoardText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlertLadder Demo")
	TObjectPtr<UTextRenderComponent> StateText;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AlertLadder Demo")
	TObjectPtr<UTextRenderComponent> RuleText;

private:
	struct FWache
	{
		FAlertState Stand;
		FVector Ort = FVector::ZeroVector;
		FString Name;
		bool bSiehtGerade = false;
	};

	void StartCycle();
	FVector ZielOrt(float T) const;
	bool SichtVon(const FWache& W, const FVector& Ziel) const;
	FString BuildStateText() const;
	FString HeadlineFor() const;
	FString RuleFor() const;
	void DrawScene() const;

	FVector Basis() const;

	TArray<FWache> Wachen;
	FVector Ziel = FVector::ZeroVector;
	float CycleTime = 0.0f;
	bool bBereit = false;
	/** An alarm is already standing - so it is not raised again on every step. See StepDemo. */
	bool bAlarmLaeuft = false;

	FAlertRules Regeln;
};
