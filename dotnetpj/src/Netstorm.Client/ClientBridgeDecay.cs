using Netstorm.Core.Bridges;
using Netstorm.Core.Original;

// 메뉴와 월드가 같은 Client 전역 다리 스캔을 한 프레임에 한 번 실행한다.
namespace Netstorm.Client;

/// <summary>현재 저장소의 범위·커서·기한을 초기화하지 않고 실제 적격 칸만 분배한다.</summary>
public static class ClientBridgeDecay
{
    /// <summary>World 없는 메뉴에서도 같은 커서를 진행하며 현재 권한의 identity를 먼저 검증한다.</summary>
    public static void RunSurface(RawSurfaceStorage storage, ClientGameplayGlobals globals, FrameTimeBox time,
        bool paused, Action<int> cell)
    {
        ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(time); ArgumentNullException.ThrowIfNull(cell);
        if (!ReferenceEquals(storage.Authority(), globals.Authority)) throw new InvalidOperationException("공통 다리 스캔의 현재 권한 소유자 불일치");
        if (storage.Pool.Edition != OriginalEdition.Patch1078) return;
        var scan = new RawBridgeDecayScan(storage.Pool, storage.Types, storage.BridgeDecayScan,
            () => storage.BridgeDecayPeriod,
            () => new BridgeDecayMode { Server = globals.Authority.Server, Authority = globals.Authority.Boss, Editor = globals.World.Editor != 0 }, cell);
        scan.Frame(time, paused);
    }

    /// <summary>실제 적격 칸의 호출 위치에서만 현재 전체 Runtime을 빌리고 같은 세션에 전달한다.</summary>
    public static void Run(RawSurfaceStorage storage, ClientGameplayGlobals globals, FrameTimeBox time, bool paused)
        => RunSurface(storage, globals, time, paused, sid =>
        {
            Host.HostSession session = globals.RequireSession();
            session.RunPinnedWorldFrame(() => session.DecayBridgeCell(time.Value, sid));
        });
}
