using Netstorm.Core.Original;

// State의 선택된 작은 자식만 같은 장기 Client 물리에 연결한다.
namespace Netstorm.Client;

/// <summary>UI/Kernel/Runtime 반납은 호출자가 수행하며 전체 State 부모를 대신하지 않는다.</summary>
public sealed class ClientStateStorageCleanup
{
    /// <summary>권한/보조 전역·Kernel/Frame과 현재 실제 Surface를 소유한다.</summary>
    private readonly ClientSurfaceGlobals _surface;
    /// <summary>같은 시계 동기화·Zone·경고255DWORD의 실제 장기 소유자다.</summary>
    private readonly ClientLocalStateGlobals _local;
    /// <summary>등록된 다중선택 헤더가 빌리는 현재 실제 개수DWORD다.</summary>
    private readonly IntBox _multipleCount;

    /// <summary>같은 Client 소유자만 허용하며 구성 자체로 어떤 정리도 실행하지 않는다.</summary>
    public ClientStateStorageCleanup(ClientSurfaceGlobals surface, ClientLocalStateGlobals local, IntBox multipleCount)
    {
        ArgumentNullException.ThrowIfNull(surface); ArgumentNullException.ThrowIfNull(local); ArgumentNullException.ThrowIfNull(multipleCount);
        if (!ReferenceEquals(surface.Gameplay, local.Gameplay)) throw new ArgumentException("State 물리 정리의 같은 실제 Client owner 필요", nameof(local));
        _surface = surface; _local = local; _multipleCount = multipleCount;
    }
    /// <summary>004c1f20의 같은 세DWORD만 지우며 표본/시계/프레임을 유지한다.</summary>
    public void ResetClockSynchronization() => RawClockSyncReset.Reset(_local.ClockSynchronization);
    /// <summary>004bfd60의 같은 Zone 앞6BYTE만 지우고 인접WORD는 유지한다.</summary>
    public void ResetZone() => RawStateZoneReset.Reset(_local.Zone.Span);
    /// <summary>실제 World 대여 반납 뒤004af170의 Pool·기록·실패점만 정리한다.</summary>
    public void ResetPool(int capacity = 0)
    {
        var storage = _surface.Storage;
        if (storage.Borrowed()) throw new InvalidOperationException("State Pool 정리 전 실제 World 대여 반납 필요");
        RawSquidPoolReset.Reset(storage.Pool, ref storage.BoardTargetState.InvalidStorage, capacity);
    }
    /// <summary>원문 위치에 선택된 열 개 물리 효과만 실행하고 미지원 효과는 명시 거부한다.</summary>
    public void ResetWorldStorage(StateWorldCleanupEffect effect)
    {
        var storage = _surface.Storage;
        if (effect == StateWorldCleanupEffect.Clear43b540) { RawStateBridgeReset.ResetOwnerColors(storage.Links.OwnerColors); return; }
        if (storage.Borrowed()) throw new InvalidOperationException("State 물리 정리 전 실제 World 대여 반납 필요");
        switch (effect)
        {
            case StateWorldCleanupEffect.Clear472830: RawStateSidLists.ResetRegistered(storage.SidLists.RegisteredHeaders, ResetCount); return;
            case StateWorldCleanupEffect.Clear4c2450: RawTotalMadeStorage.ResetCounts(storage.TotalMade); return;
            case StateWorldCleanupEffect.Clear4b2b50: storage.Hash.Reset(); return;
            case StateWorldCleanupEffect.Clear4328a0: RawStateWorldCleanup.ResetStatePlayerNoMakeWarnings(_local.PlayerNoMakeWarnings); return;
            case StateWorldCleanupEffect.Clear4ab940: RawStateWorldCleanup.ResetStateSpotMap(storage.Spots); return;
            case StateWorldCleanupEffect.Reset568b60: RawIslandRuntime.ResetRawIslandRecords(storage.Islands.Records); return;
            case StateWorldCleanupEffect.Reset54c8a0: storage.Chunks.Reset(); return;
            case StateWorldCleanupEffect.ResetGraph: RawGraphStorage.ResetTable(storage.Graph); return;
            case StateWorldCleanupEffect.Reset422490: RawStateBridgeReset.ResetDecayScan(storage.BridgeDecayScan, storage.BridgeDecayPeriod, _surface.Clock.Frame.Value.Game); return;
            default: throw new ArgumentOutOfRangeException(nameof(effect), "State 물리 정리의 미지원 실제 Effect");
        }
    }
    /// <summary>실제 선택 헤더의 개수만 직접0으로 쓰며 목록 내용/등록 표는 유지한다.</summary>
    private void ResetCount(uint header)
    {
        if (header == 0x5caf28u) { _multipleCount.Value = 0; return; }
        int unused = 0; _surface.Storage.SidLists.ResetCount(header, ref unused);
    }
}
