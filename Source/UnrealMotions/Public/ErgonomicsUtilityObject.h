#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "HotkeySectionInfo.h"
#include "ErgonomicsUtilityObject.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUtilityObjectAction);

/**
 *
 */
UCLASS()
class UNREALMOTIONS_API UErgonomicsUtilityObject : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	void ConstructChords(
		bool bCtrl = true, bool bAlt = false,
		bool bShift = true, bool bCmd = false);

	UFUNCTION(BlueprintCallable, Category = "UnrealMotions | Utilities")
	void ClearHotkeys(EHotkeySection Section, bool bClearSecondary = false);

	UFUNCTION(BlueprintCallable, Category = "UnrealMotions | Utilities")
	void SetHotkeysToNums(
		EHotkeySection Section = EHotkeySection::TabNavigation,
		bool		   bClearConflictingKeys = true,
		bool bCtrl = true, bool bAlt = false,
		bool bShift = true, bool bCmd = false);

	UFUNCTION(BlueprintCallable, Category = "UnrealMotions | Utilities")
	void DebugBookmarks();

	UFUNCTION(BlueprintCallable, Category = "UnrealMotions | Utilities")
	void GetHotkeyMappingState(
		EHotkeySection Section, TArray<FString>& OutMappingState);

public:
	TArray<FInputChord> Chords;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FHotkeySectionInfo HotkeySectionInfo;
	UPROPERTY(BlueprintAssignable)
	FOnUtilityObjectAction OnUtilityObjectAction;
};