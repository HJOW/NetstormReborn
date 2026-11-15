// 실제 가상표의 접두/전체 몸체와 장기 기본 장치를 현재 위치에서 선택한다.
namespace Netstorm.Core.Original;

/// <summary>같은 Pool의 실제 PostPop 가상 분배와 외곽 호출 수명을 관리한다.</summary>
public sealed partial class RawPostPop
{
    /// <summary>같은 현재 World/Storage의 다리 연결 접두다.</summary>
    private readonly Action<int>? _bridgeConnector;
    /// <summary>같은 현재 섬 받침의 생성/연결 접두다.</summary>
    private Action<int, uint>? _islandPrefix;
    /// <summary>현재 noIsland의 3×3 받침 생성 접두다.</summary>
    private Action<int, uint>? _surfacePrefix;
    /// <summary>최초 Teleport가 실제12 생성자를 호출하는 접두다.</summary>
    private Action<int, uint>? _teleportPrefix;
    /// <summary>단기 파생 대여가 없을 때 실제 장기 자원 몸체를 선택한다.</summary>
    private Action<int, uint>? _defaultResourcePostPop;
    /// <summary>같은 풀의 전체 Carrier/Damageable 후처리다.</summary>
    private RawCarrierPostPop? _carrierPostPop;
    /// <summary>같은 풀의 전용 자산 전체 후처리다.</summary>
    private RawAssetPostPop? _assetPostPop;
    /// <summary>PATCH 직접 Carrier00427720을 호출하는 세 가상표다.</summary>
    private static readonly uint[] PatchCarrierTables = [0x503d80, 0x503e50, 0x50e568];
    /// <summary>CD 직접 Carrier004e6360을 호출하는 두 가상표다.</summary>
    private static readonly uint[] CdCarrierTables = [0x503ee8, 0x506ea0];
    /// <summary>PATCH 직접 Damageable0044bf80을 호출하는 두 가상표다.</summary>
    private static readonly uint[] PatchDamageableTables = [0x50a820, 0x50a8e8];
    /// <summary>CD 직접 Damageable004621c0을 호출하는 두 가상표다.</summary>
    private static readonly uint[] CdDamageableTables = [0x500a58, 0x500b10];
    /// <summary>공통 성공 대신 실제 장기 전용 몸체를 요구하는 기타 PATCH 가상표다.</summary>
    private static readonly uint[] PatchResourceTables = [0x508cc8, 0x502c38, 0x502cf0, 0x502db0, 0x507770, 0x509148, 0x509210, 0x5040c0, 0x504198, 0x50a748, 0x50e888, 0x509470];

    /// <summary>실제 섬 접두를 연결하고 null이면 현재 연결만 반납한다.</summary>
    public void SetIslandPrefix(Action<int, uint>? prefix) => _islandPrefix = prefix;
    /// <summary>실제 표면 접두를 연결하고 null이면 현재 연결만 반납한다.</summary>
    public void SetSurfacePrefix(Action<int, uint>? prefix) => _surfacePrefix = prefix;
    /// <summary>실제12 생성 접두를 연결하고 null이면 현재 연결만 반납한다.</summary>
    public void SetTeleportPrefix(Action<int, uint>? prefix) => _teleportPrefix = prefix;
    /// <summary>PATCH는 TYPE/연결 유무와 무관하게 실제 다리 가상표를 선택한다.</summary>
    public bool HandlesBridge(int sid) => (_pool.Edition == OriginalEdition.Patch1078 || _bridgeConnector is not null)
        && Read(_pool.Slot(sid), 0, 4) == (_pool.Edition == OriginalEdition.Patch1078 ? 0x5034c8u : 0x501ab0u);
    /// <summary>PATCH는 현재 TYPE/접두 유무로 실제 섬 가상표를 가리지 않는다.</summary>
    public bool HandlesIsland(int sid) => (_pool.Edition == OriginalEdition.Patch1078 || _islandPrefix is not null)
        && Read(_pool.Slot(sid), 0, 4) == (_pool.Edition == OriginalEdition.Patch1078 ? 0x506d98u : 0x506960u);
    /// <summary>PATCH는 현재 TYPE/접두 유무로 실제 표면 가상표를 가리지 않는다.</summary>
    public bool HandlesSurface(int sid) => (_pool.Edition == OriginalEdition.Patch1078 || _surfacePrefix is not null)
        && Read(_pool.Slot(sid), 0, 4) == (_pool.Edition == OriginalEdition.Patch1078 ? 0x506f08u : 0x506ad0u);
    /// <summary>서로 다른 Pool의 Carrier/Damageable 장부 조합을 거부한다.</summary>
    public void SetCarrierPostPop(RawCarrierPostPop? body)
    {
        if (body is not null && !ReferenceEquals(body.Pool, _pool)) throw new ArgumentException("Carrier Pop/후처리 풀 불일치", nameof(body));
        _carrierPostPop = body;
    }
    /// <summary>등록한 실제 직접 Carrier 가상표만 처리한다.</summary>
    public bool HandlesCarrier(int sid) => _carrierPostPop is not null
        && (_pool.Edition == OriginalEdition.Patch1078 ? PatchCarrierTables : CdCarrierTables).Contains(Read(_pool.Slot(sid), 0, 4));
    /// <summary>등록한 실제 직접 Damageable 가상표만 처리한다.</summary>
    public bool HandlesDamageable(int sid) => _carrierPostPop is not null
        && (_pool.Edition == OriginalEdition.Patch1078 ? PatchDamageableTables : CdDamageableTables).Contains(Read(_pool.Slot(sid), 0, 4));
    /// <summary>같은 실제 Pool의 전용 자산 전체 몸체만 등록한다.</summary>
    public void SetAssetPostPop(RawAssetPostPop? body)
    {
        if (body is not null && !ReferenceEquals(body.Pool, _pool)) throw new ArgumentException("자산 Pop/후처리 풀 불일치", nameof(body));
        _assetPostPop = body;
    }
    /// <summary>자산 몸체가 지원하는 실제 가상표만 조회한다.</summary>
    public bool HandlesAsset(int sid) => _assetPostPop is not null && _assetPostPop.Handles(sid);
    /// <summary>실제 PATCH 장기 자원 분배만 설치하며 게임 조회/후처리는 실행하지 않는다.</summary>
    public void BindDefaultResourcePostPop(Action<int, uint> body)
    {
        RequirePatch(); ArgumentNullException.ThrowIfNull(body); _defaultResourcePostPop = body;
    }
    /// <summary>선택한 실제 가상 몸체 전체를 현재 Runtime pin 안에서 실행한다. 미구현 몸체는 도달 위치에서 실패한다.</summary>
    private void DispatchPostPop(int sid, uint flags)
    {
        object? owner = _runtimePin?.Invoke();
        try
        {
            uint table = Read(_pool.Slot(sid), 0, 4);
            if (_defaultDevices.AiNotice is not null && !HandlesDerived(sid)
                && (PatchResourceTables.Contains(table) || RawAssetPostPop.HasDedicatedVtable(_pool.Edition, table)
                    || PatchCarrierTables.Contains(table) || PatchDamageableTables.Contains(table)))
            {
                Action<int, uint> selected = _defaultResourcePostPop ?? throw new InvalidOperationException("Native 전용 자산 후처리의 실제 장기 모듈 미설치");
                selected(sid, flags); return;
            }
            // 전체 파생 몸체는 내부 공통 부모가 깊이를 한 번 줄이므로 여기서 다시 호출하지 않는다.
            if (HandlesPlayer(sid)) { RawPlayerWorld selected = _playerPostPop!; selected.PostPop(sid, flags); return; }
            if (HandlesPriest(sid)) { RawPriestPostPopTail selected = _priestPostPop!; selected.PostPop(sid, flags); return; }
            if (HandlesCarrier(sid) || HandlesDamageable(sid))
            {
                bool carrier = HandlesCarrier(sid); RawCarrierPostPop selected = _carrierPostPop!;
                if (carrier) selected.PostPop(sid, flags); else selected.DamageablePostPop(sid, flags); return;
            }
            if (HandlesAsset(sid)) { RawAssetPostPop selected = _assetPostPop!; selected.PostPop(sid, flags); return; }
            if (HandlesBridge(sid)) BridgePrefix(_pool, sid, flags, _bridgeConnector);
            else if (HandlesIsland(sid))
            {
                if (_pool.Edition != OriginalEdition.Patch1078 || (flags & 1) != 0)
                { Action<int, uint> selected = _islandPrefix ?? throw new InvalidOperationException("섬 선택 접두 미연결"); selected(sid, flags); }
            }
            else if (HandlesSurface(sid))
            {
                if (_pool.Edition != OriginalEdition.Patch1078 || (flags & 1) != 0)
                { Action<int, uint> selected = _surfacePrefix ?? throw new InvalidOperationException("표면 선택 접두 미연결"); selected(sid, flags); }
            }
            else if (HandlesForcefield(sid)) { RawForcefieldPostPop selected = _forcefieldPostPop!; selected.Prefix(sid, flags); }
            else if (_pool.Edition == OriginalEdition.Patch1078 && Read(_pool.Slot(sid), 0, 4) == 0x5070c0)
            {
                if ((flags & 1) != 0) { Action<int, uint> selected = _teleportPrefix ?? throw new InvalidOperationException("Teleport 선택 생성 접두 미연결"); selected(sid, flags); }
            }
            else if (!OriginalPopDisplay.Supports(_pool.Edition, Read(_pool.Slot(sid), 0, 4), flags))
                throw new InvalidOperationException("현재 가상 후처리 몸체 미복원");
            PostPop(sid, flags);
        }
        finally { GC.KeepAlive(owner); }
    }
}
