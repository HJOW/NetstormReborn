using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 짧은 실행기와 Session callback을 보관하지 않는 Client 수명의 커서 데이터다.
namespace Netstorm.Client;

/// <summary>ClientCursorGlobals.h의 고정 수명/배치/격자/회전 DWORD를 소유한다.</summary>
public sealed class ClientCursorGlobals
{
    /// <summary>동일30DWORD/측정/현재Gump 저장소다.</summary>
    public CursorLifecycleState Lifecycle { get; } = new();
    /// <summary>동일 허용/경고/그림/설명 저장소다.</summary>
    public CursorPlacementState Placement { get; } = new();
    /// <summary>55a2d8의 현재 전체 격자 DWORD다.</summary>
    public UIntBox SnapToGrid { get; } = new();
    /// <summary>54db60의 현재 회전 모드이며 일반 reset으로 옵션을 지우지 않는다.</summary>
    public UIntBox RotationMode { get; } = new();
    /// <summary>같은 데이터 주소를 유지하고 현재 checking/화면 원점을 보존하는 일반 World 준비다.</summary>
    public void ResetForGeneralWorld()
    {
        RawCheckingFlag? checking = Lifecycle.Checking is { } current ? new RawCheckingFlag(current) : null;
        ReadOnlyMemory<int>? pixels = Placement.SharedScreenOrigin;
        Lifecycle.Reset(); Lifecycle.Checking = checking; Placement.Reset(); Placement.SharedScreenOrigin = pixels; SnapToGrid.Value = 0;
    }
}
