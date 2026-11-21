// 사제와 공통 Carrier의 실제 정지 부모를 지면·HP·finder와 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawPriestHalt.h의 하위 효과 사이에서 다시 읽는 현재 상태다.</summary>
public sealed class PriestHaltState
{
    /// <summary>독립 입력의 사제 지면/저HP 정지 모드다.</summary>
    public bool Battle { get; set; }
    /// <summary>독립 입력의 현재 권한이다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>공통 Halt가 조회하는 첫 현재 Carrier TYPE다.</summary>
    public uint CarrierType { get; set; } = 156;
    /// <summary>PATCH에서만 조회하는 너겟 운반 TYPE다.</summary>
    public uint NuggetCarrierType { get; set; } = 187;
    /// <summary>실제 장기 모드 owner를 소비 위치에서 읽는다.</summary>
    public Func<bool>? SharedBattle { get; set; }
    /// <summary>실제 Client 권한을 소비 위치에서 읽는다.</summary>
    public Func<bool>? SharedBoss { get; set; }
    /// <summary>실제 첫 Carrier TYPE owner다.</summary>
    public UIntBox? SharedCarrierType { get; set; }
    /// <summary>실제 너겟 운반 TYPE owner다.</summary>
    public UIntBox? SharedNuggetCarrierType { get; set; }
    /// <summary>현재 지면 모드다.</summary>
    public bool CurrentBattle() => SharedBattle?.Invoke() ?? Battle;
    /// <summary>현재 권한이다.</summary>
    public bool CurrentBoss() => SharedBoss?.Invoke() ?? Boss;
    /// <summary>현재 첫 운반 TYPE다.</summary>
    public uint CurrentCarrierType() => SharedCarrierType?.Value ?? CarrierType;
    /// <summary>현재 너겟 운반 TYPE다.</summary>
    public uint CurrentNuggetCarrierType() => SharedNuggetCarrierType?.Value ?? NuggetCarrierType;
}

/// <summary>정지 부모가 호출하는 실제 가상 공간·포획·너겟 전달 함수다.</summary>
public sealed record PriestHaltHooks
{
    /// <summary>사제 가상48을 flags0으로 호출한다.</summary>
    public Action<int, uint>? Unpop { get; init; }
    /// <summary>최초 좌표를 스냅한 가상90을 flags0으로 호출한다.</summary>
    public Action<int, float, float, uint>? Pop { get; init; }
    /// <summary>첫 Carrier의 실제 포획 반환이다.</summary>
    public Func<int, uint>? CarrierPickup { get; init; }
    /// <summary>PATCH 너겟 전달의 실제 반환이다.</summary>
    public Func<int, uint>? NuggetTransfer { get; init; }
}

/// <summary>cpppj/src/o/RawPriestHalt.cpp의 사제 가상38과 공통Carrier Halt다.</summary>
public sealed class RawPriestHalt
{
    /// <summary>지면 확인의 원문 float bias다.</summary>
    private const float GroundBias = 0.9999f;
    /// <summary>공간 복귀 helper의 서로 다른 원문 float bias다.</summary>
    private const float SnapBias = 0.9999899864196777f;
    /// <summary>원문 부모가 이동을 계속하는 반환이다.</summary>
    private const uint Moving = 10;
    /// <summary>원문 지면/저HP 정지 반환이다.</summary>
    private const uint Stopped = 11;
    /// <summary>원문 낙하/운반 상태 변경 반환이다.</summary>
    private const uint Changed = 12;
    /// <summary>모든 현재 슬롯과 하위 장치의 동일 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>실제 일반 finder의 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 후보 TYPE/발자국 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재 지면의 같은65536BYTE다.</summary>
    private readonly byte[] _spots;
    /// <summary>기존 실제 Carrier 지면/낙하 본체다.</summary>
    private readonly RawCarrierCheck _check;
    /// <summary>현재 signed HP를 읽는 기존 사제 상태다.</summary>
    private readonly RawPriestState _priest;
    /// <summary>같은 실제 최대HP/모드 본체다.</summary>
    private readonly RawSquidReward _hitPoints;
    /// <summary>효과 뒤 현재 값을 다시 읽는 상태다.</summary>
    private readonly PriestHaltState _state;
    /// <summary>함수 값을 복사한 실제 하위 효과다.</summary>
    private readonly PriestHaltHooks _hooks;
    /// <summary>같은 실제 풀/지도/TYPE와 모든 선택 하위를 첫 효과 전에 확인한다.</summary>
    public RawPriestHalt(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, RawCarrierCheck check, RawPriestState priest, RawSquidReward hitPoints, PriestHaltState state, PriestHaltHooks hooks)
    {
        if (!ReferenceEquals(pool, check.Pool) || !ReferenceEquals(pool, priest.Pool) || !ReferenceEquals(pool, hitPoints.Pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || spots.Length != 65536 || hooks.Unpop is null || hooks.Pop is null || hooks.CarrierPickup is null || hooks.NuggetTransfer is null) throw new ArgumentException("사제 Halt 실제 월드/하위 효과 연결 누락");
        Pool = pool; _hash = hash; _types = types; _spots = spots; _check = check; _priest = priest; _hitPoints = hitPoints; _state = state; _hooks = hooks with { };
    }
    /// <summary>원문 float 두 입력의 합을 double에 유지하고 정상 지도 범위에서0방향 절삭한다.</summary>
    private static int Cell(float coordinate, float bias)
    {
        double value = (double)coordinate + bias;
        if (!double.IsFinite(value) || value < 0 || value >= 256) throw new ArgumentOutOfRangeException(nameof(coordinate), "사제 Halt 정상 지도 좌표"); return (int)value;
    }
    /// <summary>지면/저HP 분기의 공간 콜백 전에 보관한 최초 좌표를 사용한다.</summary>
    public uint Halt(int sid)
    {
        RawPathPoint initial = RawPathSlot.Position(Pool, sid);
        if (_state.CurrentBattle())
        {
            int x = Cell(initial.X, GroundBias), y = Cell(initial.Y, GroundBias); bool ground = (_spots[y * 256 + x] & 6) != 0;
            if (!ground)
            {
                if (_state.CurrentBoss()) { _hooks.Unpop!(sid, 0); _hooks.Pop!(sid, Cell(initial.X, SnapBias), Cell(initial.Y, SnapBias), 0); } return Stopped;
            }
            if (_priest.CurrentHitPoints(sid) < _hitPoints.MaxHitPoints(sid) / 2 && _state.CurrentBoss())
            {
                _hooks.Unpop!(sid, 0); _hooks.Pop!(sid, Cell(initial.X, SnapBias), Cell(initial.Y, SnapBias), 0); return Stopped;
            }
        }
        return CommonHalt(sid);
    }
    /// <summary>현재 가상cc·일반 finder·판본별 운반 반환 조건과 마지막 기본10을 보존한다.</summary>
    public uint CommonHalt(int sid)
    {
        if (_check.Check(sid)) return Changed;
        RawPathPoint point = RawPathSlot.Position(Pool, sid); int x = Cell(point.X, 0), y = Cell(point.Y, 0);
        var finder = new RawSquidFinder(Pool, _hash, _types); bool patch = Pool.Edition == OriginalEdition.Patch1078;
        // 기본0단계/버킷 순서와 buried 제외는 기존 실제 finder에 맡긴다.
        for (int candidate = finder.Begin(new(x, y, x, y)); candidate != 0; candidate = finder.Next())
        {
            byte type = Pool.Slot(candidate)[10];
            if (candidate != sid && type == _state.CurrentCarrierType()) { uint result = _hooks.CarrierPickup!(candidate); if (patch ? result == Changed : result != 0) return Changed; }
            else if (patch && candidate != sid && type == _state.CurrentNuggetCarrierType()) { if (_hooks.NuggetTransfer!(candidate) == Changed) return Changed; }
        }
        return Moving;
    }
}
