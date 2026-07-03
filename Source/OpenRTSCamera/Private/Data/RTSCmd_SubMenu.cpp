// Copyright 2026 Winyunq. All Rights Reserved.
#include "Data/RTSCmd_SubMenu.h"
#include "RTSCommandSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

void URTSCmd_SubMenu::Execute_Implementation(AActor* Executor)
{
    // 二进制导航逻辑：子菜单直接命令 UI 面板切换
    // 通过低层级的信号中心广播，由 UI 层级捕获并执行刷新。
    if (!TargetGrid) return;

    UWorld* World = Executor ? Executor->GetWorld() : (GEngine ? GEngine->GetWorldContexts()[0].World() : nullptr);
    if (!World) return;

    if (ULocalPlayer* LP = World->GetFirstLocalPlayerFromController())
    {
        if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
        {
            SignalHub->RequestNavigation(TargetGrid, Executor);
        }
    }
}
