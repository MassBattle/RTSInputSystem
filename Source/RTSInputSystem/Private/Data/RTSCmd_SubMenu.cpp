// Copyright 2026 Winyunq. All Rights Reserved.
#include "Data/RTSCmd_SubMenu.h"
#include "RTSCommandSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

void URTSCmd_SubMenu::Execute_Implementation(AActor* Executor)
{
	if (!TargetGrid)
	{
		return;
	}

	// Mass-only selections have no Actor executor. Transient loadout buttons are
	// owned by a local-player subsystem, so resolve that outer chain first.
	ULocalPlayer* LocalPlayer = GetTypedOuter<ULocalPlayer>();
	UWorld* World = Executor ? Executor->GetWorld() : GetWorld();
	if (!LocalPlayer && World)
	{
		LocalPlayer = World->GetFirstLocalPlayerFromController();
	}

	if (!LocalPlayer && GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game || Context.WorldType == EWorldType::PIE)
			{
				World = Context.World();
				LocalPlayer = World ? World->GetFirstLocalPlayerFromController() : nullptr;
				if (LocalPlayer)
				{
					break;
				}
			}
		}
	}

	if (LocalPlayer)
	{
		if (URTSCommandSubsystem* SignalHub = LocalPlayer->GetSubsystem<URTSCommandSubsystem>())
		{
			SignalHub->RequestNavigation(TargetGrid, Executor);
		}
	}
}
