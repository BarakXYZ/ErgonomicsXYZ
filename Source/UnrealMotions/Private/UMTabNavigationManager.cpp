#include "UMTabNavigationManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Docking/SDockTab.h"
// #include "UMHelpers.h"

/* Main window representation of the app.
 * All other windows are children of it. */
#include "Interfaces/IMainFrameModule.h"

DEFINE_LOG_CATEGORY_STATIC(LogUMTabNavigation, NoLogging, All); // Prod
// DEFINE_LOG_CATEGORY_STATIC(LogUMTabNavigation, Log, All); // Dev

#define LOCTEXT_NAMESPACE "UMTabNavigationManager"

FUMTabNavigationManager::FUMTabNavigationManager()
{
	FInputBindingManager&		InputBindingManager = FInputBindingManager::Get();
	IMainFrameModule&			MainFrameModule = IMainFrameModule::Get();
	TSharedRef<FUICommandList>& CommandList =
		MainFrameModule.GetMainFrameCommandBindings();
	TSharedPtr<FBindingContext> MainFrameContext =
		InputBindingManager.GetContextByName(MainFrameContextName);
	/** We can know the name of the context by looking at the constructor
	 * of the TCommands that its extending. e.g. SystemWideCommands, MainFrame...
	 * */

	InitTabNavInputChords();
	AddCommandsToList(MainFrameContext); // Macros
	MapTabCommands(CommandList);
}

FUMTabNavigationManager::~FUMTabNavigationManager()
{
	TabChords.Empty();
	CommandInfoTabs.Empty();
}

void FUMTabNavigationManager::InitTabNavInputChords(bool bCtrl, bool bAlt, bool bShift, bool bCmd)

{
	TabChords.Reserve(10);

	const FKey NumberKeys[] = { EKeys::Zero, EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four,
		EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine };

	// Setting by default to Control + Shift + 0-9
	const EModifierKey::Type ModifierKeys = EModifierKey::FromBools(bCtrl, bAlt, bShift, bCmd);

	for (const FKey& Key : NumberKeys)
	{
		TabChords.Add(FInputChord(ModifierKeys, Key));
	}
}

void FUMTabNavigationManager::RemoveDefaultCommand(FInputBindingManager& InputBindingManager, FInputChord Command)
{
	const TSharedPtr<FUICommandInfo> CheckCommand = InputBindingManager.GetCommandInfoFromInputChord(
		MainFrameContextName, Command, false /* We want to check the current, not default command */);
	if (CheckCommand.IsValid())
		CheckCommand->RemoveActiveChord(EMultipleKeyBindingIndex::Primary);
}

void FUMTabNavigationManager::MapTabCommands(TSharedRef<FUICommandList>& CommandList)
{
	for (int i{ 0 }; i < TabChords.Num(); ++i)
	{
		CommandList->MapAction(CommandInfoTabs[i], FExecuteAction::CreateLambda([this, i]() { OnMoveToTab(i); }));
	}
}

void FUMTabNavigationManager::OnMoveToTab(int32 TabIndex)
{

	UE_LOG(LogUMTabNavigation, Display, TEXT("OnMoveToTab"));

	FSlateApplication&	SlateApp = FSlateApplication::Get();
	TSharedPtr<SWindow> ActiveWindow = SlateApp.GetActiveTopLevelWindow();

	if (ActiveWindow.IsValid())
	{
		TSharedPtr<SWidget> DockingTabWell = nullptr;
		bool				Found = TraverseWidgetTree(ActiveWindow, DockingTabWell);
		if (!Found)
		{
			UE_LOG(LogUMTabNavigation, Warning, TEXT("No SDockingTabWell found in the active window."));
		}
		else
		{
			FocusTab(DockingTabWell, TabIndex);

			/**
			 * BringToFront is important to mitigate a bug where modal windows
			 * will semi-steal focus from the active window when switching tabs.
			 * Essentially, bring to front helps with solidifying the focus.
			 * */
			ActiveWindow->BringToFront();
		}
	}
	else
	{
		UE_LOG(LogUMTabNavigation, Warning, TEXT("No active top-level window found."));
	}
}

bool FUMTabNavigationManager::TraverseWidgetTree(const TSharedPtr<SWidget>& TraverseWidget,
	TSharedPtr<SWidget>& DockingTabWell, int32 Depth)
{
	// Log the current widget and depth
	UE_LOG(LogUMTabNavigation, Display, TEXT("%s[Depth: %d] Checking widget: %s"),
		*FString::ChrN(Depth * 2, ' '), // Visual indentation
		Depth, *TraverseWidget->GetTypeAsString());

	// Looking for SDockingTabWell as it stores the tabs
	if (TraverseWidget->GetTypeAsString() == "SDockingTabWell")
	{
		DockingTabWell = TraverseWidget;
		UE_LOG(LogUMTabNavigation, Display, TEXT("%s[Depth: %d] Found SDockingTabWell: %s"),
			*FString::ChrN(Depth * 2, ' '), Depth, *TraverseWidget->ToString());
		return true;
	}

	// Recursively traverse the children of the current widget
	FChildren* Children = TraverseWidget->GetChildren();
	if (Children)
	{
		for (int32 i = 0; i < Children->Num(); ++i)
		{
			TSharedPtr<SWidget> Child = Children->GetChildAt(i);
			if (TraverseWidgetTree(Child, DockingTabWell, Depth + 1))
			{
				return true;
			}
		}
	}

	return false;
}

void FUMTabNavigationManager::FocusTab(const TSharedPtr<SWidget>& DockingTabWell, int TabIndex)
{
	FChildren* Tabs = DockingTabWell->GetChildren();
	/* Check if valid and if we have more are the same number of tabs as index */
	if (Tabs && Tabs->Num() >= TabIndex)
	{
		TSharedPtr<SDockTab> TargetTab = nullptr;
		if (TabIndex == 0)
			TabIndex = Tabs->Num(); // Get last child
		TargetTab = StaticCastSharedRef<SDockTab>(Tabs->GetChildAt(TabIndex - 1));
		if (TargetTab.IsValid())
			TargetTab->ActivateInParent(ETabActivationCause::SetDirectly);
	}
}

/**
 * I've tried to call this in a simple loop (instead of by hand), but no go.
 * DOC:Macro for extending the command list (add more actions to its context)
 */
void FUMTabNavigationManager::AddCommandsToList(TSharedPtr<FBindingContext> MainFrameContext)
{
	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[0], "MoveToTabLast", "Focus Last Tab",
		"Moves to the last tab", EUserInterfaceActionType::Button, TabChords[0]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[1], "MoveToTab1", "Focus Tab 1",
		"If exists, draws attention to major tab 1", EUserInterfaceActionType::Button, TabChords[1]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[2], "MoveToTab2", "Focus Tab 2",
		"If exists, draws attention to major tab 2", EUserInterfaceActionType::Button, TabChords[2]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[3], "MoveToTab3", "Focus Tab 3",
		"If exists, draws attention to major tab 3", EUserInterfaceActionType::Button, TabChords[3]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[4], "MoveToTab4", "Focus Tab 4",
		"If exists, draws attention to major tab 4", EUserInterfaceActionType::Button, TabChords[4]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[5], "MoveToTab5", "Focus Tab 5",
		"If exists, draws attention to major tab 5", EUserInterfaceActionType::Button, TabChords[5]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[6], "MoveToTab6", "Focus Tab 6",
		"If exists, draws attention to major tab 6", EUserInterfaceActionType::Button, TabChords[6]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[7], "MoveToTab7", "Focus Tab 7",
		"If exists, draws attention to major tab 7", EUserInterfaceActionType::Button, TabChords[7]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[8], "MoveToTab8", "Focus Tab 8",
		"If exists, draws attention to major tab 8", EUserInterfaceActionType::Button, TabChords[8]);

	UI_COMMAND_EXT(MainFrameContext.Get(), CommandInfoTabs[9], "MoveToTab9", "Focus Tab 9",
		"If exists, draws attention to major tab 9", EUserInterfaceActionType::Button, TabChords[9]);
}
#undef LOCTEXT_NAMESPACE
