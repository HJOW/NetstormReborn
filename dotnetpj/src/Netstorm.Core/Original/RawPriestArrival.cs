using System.Buffers.Binary;

// 사제 도착의 중립 회복/내용물 부착·자기삭제와 일반 Carrier 위임을 복원한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawPriestArrival.h의 하위 효과 뒤에도 현재 값을 읽는 전역이다.</summary>
public sealed class PriestArrivalState
{
    /// <summary>독립 입력의 현재 권한이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>독립 입력의 현재 적재 상태다.</summary>
    public bool Loading { get; set; }
    /// <summary>독립 입력의 중립 귀환 억제다.</summary>
    public bool SuppressNeutralReturn { get; set; }
    /// <summary>실제 현재 Client 권한이다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>실제 현재 적재 상태다.</summary>
    public Func<bool>? SharedLoading { get; set; }
    /// <summary>실제 현재 중립 귀환 억제다.</summary>
    public Func<bool>? SharedSuppressNeutralReturn { get; set; }
    /// <summary>현재 권한을 소비 위치에서 읽는다.</summary>
    public bool CurrentBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>현재 적재를 소비 위치에서 읽는다.</summary>
    public bool CurrentLoading() => SharedLoading?.Invoke() ?? Loading;
    /// <summary>현재 중립 귀환 억제를 소비 위치에서 읽는다.</summary>
    public bool CurrentSuppressNeutralReturn() => SharedSuppressNeutralReturn?.Invoke() ?? SuppressNeutralReturn;
}

/// <summary>실제 사제 자기삭제와 큰 중립 귀환 몸체의 필수 경계다.</summary>
public sealed record PriestArrivalHooks
{
    /// <summary>사제 가상10의flags0 자기삭제다.</summary>
    public Action<int, uint>? Destroy { get; init; }
    /// <summary>실제 속도/귀환/임의 위치 이동 본체다.</summary>
    public Action<int>? NeutralReturn { get; init; }
}

/// <summary>cpppj/src/o/RawPriestArrival.cpp의 사제 가상3c 전체 wrapper다.</summary>
public sealed class RawPriestArrival
{
    /// <summary>중립 교체/자기삭제의 원문 반환이다.</summary>
    private const uint Replaced = 1001;
    /// <summary>포획 내용물의 원문 마스크다.</summary>
    private const uint CaptureMask = 32;
    /// <summary>현재 사제/신전과 모든 하위 장치의 같은 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>기존 현재 signedHP 본체다.</summary>
    private readonly RawPriestState _priest;
    /// <summary>기존 현재 최대HP/모드 본체다.</summary>
    private readonly RawSquidReward _hitPoints;
    /// <summary>기존 현재 지면 지역 getter다.</summary>
    private readonly RawCanonPlacementTerrain _terrain;
    /// <summary>같은 실제 섬목록 Temple getter다.</summary>
    private readonly RawIslandRuntime _islands;
    /// <summary>기존 실제 float 발자국 getter다.</summary>
    private readonly RawSquidNeighbors _footprint;
    /// <summary>기존 실제 공장/내용물 부착 본체다.</summary>
    private readonly RawContainedCreate _create;
    /// <summary>일반 Carrier 도착의 전체 현재 본체다.</summary>
    private readonly RawCarrierArrival _carrier;
    /// <summary>효과 뒤 현재 모드를 읽는 상태다.</summary>
    private readonly PriestArrivalState _state;
    /// <summary>생성 때 복사한 실제 자기삭제/귀환 함수다.</summary>
    private readonly PriestArrivalHooks _hooks;
    /// <summary>같은 실제 풀과 필수 가상 효과를 첫 조회 전에 확인한다.</summary>
    public RawPriestArrival(RawSidPool pool, RawPriestState priest, RawSquidReward hitPoints, RawCanonPlacementTerrain terrain, RawIslandRuntime islands, RawSquidNeighbors footprint, RawContainedCreate create, RawCarrierArrival carrier, PriestArrivalState state, PriestArrivalHooks hooks)
    {
        if (!ReferenceEquals(pool, priest.Pool) || !ReferenceEquals(pool, hitPoints.Pool) || !ReferenceEquals(pool, terrain.Pool) || !ReferenceEquals(pool, islands.Pool()) || !ReferenceEquals(pool, footprint.Pool) || !ReferenceEquals(pool, create.Pool) || !ReferenceEquals(pool, carrier.Pool) || hooks.Destroy is null || hooks.NeutralReturn is null) throw new ArgumentException("사제 도착 실제 월드/하위 효과 연결 누락");
        Pool = pool; _priest = priest; _hitPoints = hitPoints; _terrain = terrain; _islands = islands; _footprint = footprint; _create = create; _carrier = carrier; _state = state; _hooks = hooks with { };
    }
    /// <summary>현재 raw owner BYTE를 판본별 위치에서 읽는다.</summary>
    private uint Owner(int sid) => Pool.Slot(sid)[Pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
    /// <summary>적재·중립0·귀환 억제를 원문 순서로 소비한다.</summary>
    private bool NeutralEligible(int sid) => !_state.CurrentLoading() && Owner(sid) == 0 && !_state.CurrentSuppressNeutralReturn();
    /// <summary>현재 plain 좌표를 signed32 범위에서0방향 절삭한다.</summary>
    private static int Coordinate(float value)
    {
        if (!float.IsFinite(value) || (double)value < -2147483648.0 || (double)value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "사제 도착 현재 좌표 범위"); return (int)value;
    }
    /// <summary>현재 지역127은0이며 나머지는 같은 섬목록의 현재 Temple SID다.</summary>
    public int TempleAt(float x, float y)
    {
        uint island = _terrain.RegionAt(Coordinate(x), Coordinate(y)); if (island == 127) return 0;
        uint temple = _islands.Temple(unchecked((int)island)); if (temple >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(x), "사제 도착 현재 섬 Temple SID"); return unchecked((ushort)temple);
    }
    /// <summary>원문 축별 signed거리의x float 저장과CD y float 저장을 유지한다.</summary>
    private bool NearTemple(int sid, int temple)
    {
        RawPathPoint point = RawPathSlot.Position(Pool, sid); SquidFootprint bounds = _footprint.Footprint(temple);
        float x = point.X < bounds.Left ? (float)((double)bounds.Left - point.X) : (float)((double)point.X - bounds.Right);
        double y = point.Y < bounds.Top ? (double)bounds.Top - point.Y : (double)point.Y - bounds.Bottom;
        if (Pool.Edition == OriginalEdition.Cd1072) y = (float)y; return (y <= x ? (double)x : y) <= 1.0;
    }
    /// <summary>저HP중립의현재섬복귀/내용물생성→자기삭제·중립귀환·일반Carrier위임 순서를 보존한다.</summary>
    public uint Arrival(int sid, uint targetA, uint targetB)
    {
        if (_state.CurrentBoss())
        {
            if (_priest.CurrentHitPoints(sid) < _hitPoints.MaxHitPoints(sid) && NeutralEligible(sid))
            {
                RawPathPoint point = RawPathSlot.Position(Pool, sid); int temple = TempleAt(point.X, point.Y);
                if (temple != 0 && Owner(temple) == Owner(sid) && NearTemple(sid, temple))
                {
                    ReadOnlySpan<byte> raw = Pool.Slot(sid); _create.Create(temple, raw[10], BinaryPrimitives.ReadUInt16LittleEndian(raw[12..]), CaptureMask, 0, 0); _hooks.Destroy!(sid, 0); return Replaced;
                }
            }
        }
        if (_state.CurrentBoss() && NeutralEligible(sid) && _priest.CurrentHitPoints(sid) >= _hitPoints.MaxHitPoints(sid) / 2) { _hooks.NeutralReturn!(sid); return Replaced; }
        return _carrier.Arrival(sid, targetA, targetB);
    }
}
