// 같은 Client의 연결/응답모드/보정 DWORD 세 참조만 원문 순서로 초기화한다.
namespace Netstorm.Core.Original;

/// <summary>원문005c8bf4/005c8c00/0055b4ac의 실제 공유 저장소다.</summary>
public sealed record ClockSyncResetGlobals
{
    /// <summary>현재 동기화 연결 token이며 메모리를 소유하거나 해제하지 않는다.</summary>
    public UIntBox Connection { get; init; } = new();
    /// <summary>게임 시각 응답모드이며 인접 송수신 카운터와 별개다.</summary>
    public UIntBox ReplyMode { get; init; } = new();
    /// <summary>State/저장 reader와 같은 현재 게임 보정 DWORD다.</summary>
    public UIntBox GameTicks { get; init; } = new();
}

/// <summary>004c1f20의 세 DWORD 쓰기만 수행한다.</summary>
public static class RawClockSyncReset
{
    /// <summary>연결→응답모드→보정을0으로 쓴다. 시계/표본/카운터/큐를 초기화하지 않는다.</summary>
    public static void Reset(ClockSyncResetGlobals globals)
    {
        ArgumentNullException.ThrowIfNull(globals); globals.Connection.Value = 0; globals.ReplyMode.Value = 0; globals.GameTicks.Value = 0;
    }
}
