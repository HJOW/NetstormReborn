using System.Buffers.Binary;

// 도착의 현재 조회 순서·판본별 경제 차이·마지막 회피 검색을 원문대로 실행한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawCarrierArrival.cpp의 전체 부모와 후보/목표 검증 본체다.</summary>
public sealed class RawCarrierArrival
{
    /// <summary>현재 Carrier를 판정하는 부류다.</summary>
    private const uint CarrierGenus = 0x30000;
    /// <summary>현재 사제를 판정하는 부류다.</summary>
    private const uint PriestGenus = 0x200000;
    /// <summary>포획 후보를 선택하는 부류다.</summary>
    private const uint PickupGenus = 0x20a000;
    /// <summary>포획/너겟 내용물의 원문 마스크다.</summary>
    private const uint NuggetMask = 32;
    /// <summary>정상 도착 완료 반환이다.</summary>
    private const uint Done = 1000;
    /// <summary>새 요청/자기 교체 반환이다.</summary>
    private const uint Replaced = 1001;
    /// <summary>일반 장애물 회피의 후속 사건이다.</summary>
    private const uint ObstacleEvent = 0x1da1a;
    /// <summary>현재 좌표 helper의 원문 float bias다.</summary>
    private const float AlmostCeil = 0.9999899864196777f;
    /// <summary>모든 부모/내용물/목표가 공유하는 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 일반 finder의 네 단계 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 부류·발자국·너겟 비용 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>기존 실제 내용물 조건 조회다.</summary>
    private readonly RawCarrierPreDestroy _carrier;
    /// <summary>기존 실제 귀환/근처TYPE getter다.</summary>
    private readonly RawPathCleanup _cleanup;
    /// <summary>기존 실제 사제 HP/지면 불가 getter다.</summary>
    private readonly RawPriestState _priest;
    /// <summary>기존 실제 GraphFor다.</summary>
    private readonly RawPlayerPlacementAnchor _anchor;
    /// <summary>기존 실제 공장/내용물 부착 본체다.</summary>
    private readonly RawContainedCreate _create;
    /// <summary>CD 위치 소리의 기존 실제 중심 getter다.</summary>
    private readonly RawSquidNeighbors _footprint;
    /// <summary>효과 뒤 읽는 같은 현재 상태다.</summary>
    private readonly CarrierArrivalState _state;
    /// <summary>생성 때 복사한 실제 함수 값이다.</summary>
    private readonly CarrierArrivalHooks _hooks;
    /// <summary>같은 실제 풀/TYPE와 모든 선택 가능한 큰 하위를 첫 조회 전에 확인한다.</summary>
    public RawCarrierArrival(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, RawCarrierPreDestroy carrier, RawPathCleanup cleanup, RawPriestState priest, RawPlayerPlacementAnchor anchor, RawContainedCreate create, RawSquidNeighbors footprint, CarrierArrivalState state, CarrierArrivalHooks hooks)
    {
        if (!ReferenceEquals(pool, carrier.Pool) || !ReferenceEquals(pool, cleanup.Pool) || !ReferenceEquals(pool, priest.Pool) || !ReferenceEquals(pool, anchor.Pool) || !ReferenceEquals(pool, create.Pool) || !ReferenceEquals(pool, footprint.Pool) || types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.Path is null || hooks.GenusAt is null || hooks.CdPriestPickupPriest is null || hooks.Pickup is null || hooks.Escape is null || hooks.ChooseReturn is null || hooks.ChooseGeyser is null || hooks.GoReturn is null || hooks.GoGeyser is null || hooks.MinedPacket is null || hooks.Destroy is null || hooks.CdAddSp is null || hooks.CdOwnerPacket is null || hooks.PlayAt is null || hooks.Robust is null) throw new ArgumentException("Carrier 도착 실제 풀/필수 하위 효과 연결 누락");
        Pool = pool; _hash = hash; _types = types; _carrier = carrier; _cleanup = cleanup; _priest = priest; _anchor = anchor; _create = create; _footprint = footprint; _state = state; _hooks = hooks with { };
    }
    /// <summary>현재 raw 좌표 두 float를 읽는다.</summary>
    private RawPathPoint Position(int sid) => RawPathSlot.Position(Pool, sid);
    /// <summary>현재 rawTYPE의 실제 부류를 읽는다.</summary>
    private uint Genus(int sid) { int type = Pool.Slot(sid)[10]; if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "Carrier 도착 현재 타입 범위"); return _types[type].Flags2; }
    /// <summary>현재 owner BYTE를 판본별 위치에서 읽는다.</summary>
    private uint Owner(int sid) => Pool.Slot(sid)[Pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
    /// <summary>현재 abstract/buried/특수 BYTE를 판본별 위치에서 읽는다.</summary>
    private byte Extra(int sid) => Pool.Slot(sid)[Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35];
    /// <summary>현재 rawWORD를 옆 stale BYTE 없이 읽는다.</summary>
    private uint Word(int sid, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(Pool.Slot(sid)[offset..]);
    /// <summary>float 합을 double에 유지한 정상 지도 좌표 절삭이다.</summary>
    private static int Cell(float value, bool biased = false)
    {
        double adjusted = (double)value + (biased ? (double)AlmostCeil : 0);
        if (!double.IsFinite(adjusted) || adjusted < 0 || adjusted >= 256) throw new ArgumentOutOfRangeException(nameof(value), "Carrier 도착 정상 지도 좌표"); return (int)adjusted;
    }
    /// <summary>중립0은 AI표를 읽지 않고 나머지는 현재 Player를 조회한다.</summary>
    private bool Controlled(int sid)
    {
        uint owner = Owner(sid); if (owner >= _state.ControlledOwners.Length) throw new ArgumentOutOfRangeException(nameof(sid), "Carrier 도착 Player 범위");
        return owner != 0 && (_hooks.CurrentControlled?.Invoke(owner) ?? _state.ControlledOwners[owner]);
    }
    /// <summary>원문9×9의 방향별 현재 동맹 DWORD를 읽는다.</summary>
    private bool Allied(uint first, uint second)
    {
        if (first >= 9 || second >= 9) throw new ArgumentOutOfRangeException(nameof(first), "Carrier 도착 동맹표 범위"); return (_hooks.CurrentAlliance?.Invoke(first, second) ?? _state.Alliances[first * 9 + second]) != 0;
    }
    /// <summary>PATCH 공통 Robust 접두 이후 정상1000을 반환한다.</summary>
    private uint CheckedDone(int sid) { if (Pool.Edition == OriginalEdition.Patch1078 && _state.Debug && !_hooks.Robust!(sid)) throw new InvalidOperationException("Carrier 도착 isRobust 원본 assert"); return Done; }
    /// <summary>현재 실제 경로가 있을 때만 그 같은 대상 DWORD를 쓴다.</summary>
    private void SetTargets(int sid, uint first, uint second) => _hooks.Path!(sid)?.SetTargets(first, second);
    /// <summary>현재 거의올림 위치의 첫 허용 후보와 특수 부류 여부를 원문 순서대로 조회한다.</summary>
    public (int Sid, bool Special) PickupCandidate(int sid, float x, float y)
    {
        int cx = Cell(x, true), cy = Cell(y, true); var finder = new RawSquidFinder(Pool, _hash, _types); bool patch = Pool.Edition == OriginalEdition.Patch1078;
        // 추가 dead/free/owner 필터 없이 실제 finder 순서를 사용한다.
        for (int candidate = finder.Begin(new(cx, cy, cx, cy)); candidate != 0; candidate = finder.Next())
        {
            uint genus = Genus(candidate); if ((genus & PickupGenus) == 0) continue; bool accept = true;
            if ((genus & PriestGenus) != 0)
            {
                bool immobile = _priest.GroundImmobile(candidate); accept = immobile;
                if (patch) accept = Owner(sid) != Owner(candidate) && (Genus(sid) & PriestGenus) == 0 && (immobile || (_state.CurrentAllowAlliedPickup() && Allied(Owner(sid), Owner(candidate))));
                else if (Owner(candidate) == Owner(sid) || ((Genus(sid) & PriestGenus) != 0 && !_hooks.CdPriestPickupPriest!())) accept = false;
                else if (_state.CurrentAllowAlliedPickup() && Allied(Owner(sid), Owner(candidate))) accept = true;
            }
            if (accept) return (candidate, (genus & 0x2000) != 0);
        }
        return (0, false);
    }
    /// <summary>현재 extra/사망/간헐천TYPE 뒤 비행 부류 또는 같은Graph를 확인한다.</summary>
    public bool ValidGeyser(int sid, int target)
    {
        if ((Extra(target) & 9) != 0 || (Pool.Slot(target)[11] & 2) != 0 || Pool.Slot(target)[10] != _state.CurrentGeyserType()) return false;
        return (Genus(sid) & 0x20000) != 0 || _anchor.GraphFor((uint)sid) == _anchor.GraphFor((uint)target);
    }
    /// <summary>PATCH 대상 진단·현재 owner/extra/dead와 Graph를 원문 순서로 확인한다.</summary>
    public bool ValidReturn(int sid, int target)
    {
        if (Pool.Edition == OriginalEdition.Patch1078 && _state.Debug && (Genus(target) & 0x200) == 0 && Pool.Slot(target)[10] != _state.OutpostType) throw new InvalidOperationException("Carrier return 대상 원본 assert");
        if (Owner(target) != Owner(sid) || (Extra(target) & 9) != 0 || (Pool.Slot(target)[11] & 2) != 0) return false;
        return (Genus(sid) & 0x20000) != 0 || _anchor.GraphFor((uint)sid) == _anchor.GraphFor((uint)target);
    }
    /// <summary>현재 내용물/대상/깊이와 큰 운반 요청·판본별 환원·회피를 실제 부모 순서로 실행한다.</summary>
    public uint Arrival(int sid, uint targetA, uint targetB)
    {
        bool patch = Pool.Edition == OriginalEdition.Patch1078;
        if ((Genus(sid) & CarrierGenus) == 0) { if (patch) throw new InvalidOperationException("Carrier 도착 isCarrier 원본 assert"); return Done; }
        if ((Genus(sid) & PriestGenus) != 0 && _hooks.Path!(sid) is { } priestPath && priestPath.Helper != 0) return Done;
        if (Owner(sid) == 0) return Done;
        int nugget = _carrier.FindContained(sid, NuggetMask, _state.CurrentNuggetType(), 0);
        RawPathPoint point = Position(sid); int ground = _hooks.GenusAt!(point.X, point.Y, 0x10000000); point = Position(sid); _ = _hooks.GenusAt(point.X, point.Y, 0x2000);
        point = Position(sid); int place = _cleanup.ReturnPlaceAt(point.X, point.Y, 1), captured = _carrier.FindContained(sid, NuggetMask, _state.PriestType, 0);
        if (captured != 0) { place = 0; ground = 0; nugget = 0; }
        point = Position(sid); int dais = _cleanup.NearbyType(point.X, point.Y, 1, _state.DaisType);
        if (dais != 0) { RawPathPoint daisPoint = Position(dais); if ((Extra(dais) & 1) != 0 || (double)point.X != (double)daisPoint.X - 3f || (double)point.Y != (double)daisPoint.Y - 3f) dais = 0; }
        point = Position(sid); var pickup = PickupCandidate(sid, point.X, point.Y); int currentContent = _carrier.FindContained(sid, NuggetMask, 0, 0); _ = _carrier.FindContained(sid, 1, 0, 0); RawPathRuntime? currentPath = _hooks.Path!(sid);
        if (currentPath is null || currentPath.MoveAwayDepth == 0)
        {
            if ((_types[Pool.Slot(sid)[10]].Flags1 & 0x1000000) == 0 && pickup.Sid != 0 && (pickup.Special || currentContent == 0)) _hooks.Pickup!(sid);
            else if (dais != 0)
            {
                if (_state.CurrentBoss())
                {
                    if (captured != 0 && Allied(Owner(sid), Owner(dais)) && _carrier.FindContained(dais, NuggetMask, _state.PriestType, 0) == 0) { _create.Create(dais, _state.PriestType, Word(captured, 12), NuggetMask, 0, 0); _hooks.Destroy!(captured, 0); }
                    _ = _hooks.Escape!(sid, 5f, 0); return Replaced;
                }
                if (patch) return CheckedDone(sid);
            }
            else if (place == 0 || nugget == 0)
            {
                if (ground != 0 && _anchor.GraphFor((uint)ground) == _anchor.GraphFor((uint)sid))
                {
                    if (patch && !_state.CurrentBoss()) return CheckedDone(sid); bool replaced = false;
                    if (_state.CurrentBoss() && !Controlled(sid))
                    {
                        if (targetB == 0 || !ValidReturn(sid, unchecked((ushort)targetB))) targetB = unchecked((uint)_hooks.ChooseReturn!(sid));
                        SetTargets(sid, (uint)ground, targetB); if (targetB != 0) { replaced = true; _hooks.GoReturn!(sid, unchecked((ushort)targetB)); }
                    }
                    if (_state.CurrentBoss() && nugget == 0 && Word(ground, 12) != 0) { _create.Create(sid, _state.CurrentNuggetType(), 0, NuggetMask, 0, 0); _hooks.MinedPacket!(ground); }
                    if (replaced) return Replaced;
                }
            }
            else if (Owner(place) == Owner(sid) && _anchor.GraphFor((uint)place) == _anchor.GraphFor((uint)sid))
            {
                if (!patch)
                {
                    uint kind = Word(nugget, 18); if (kind >= _types.Count || !float.IsFinite(_types[(int)kind].Cost)) throw new ArgumentOutOfRangeException(nameof(sid), "CD 너겟 현재 비용 범위");
                    double cost = _types[(int)kind].Cost; if (cost < -2147483648.0 || cost >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(sid), "CD 너겟 비용 절삭 범위");
                    _hooks.CdAddSp!(Owner(sid), (int)cost); _hooks.CdOwnerPacket!(Owner(sid)); float[] center = _footprint.Center(place); _hooks.PlayAt!(center[0], center[1], "guy.wav", 0); if (_state.CurrentBoss()) _hooks.Destroy!(nugget, 0);
                }
                else if (!_state.CurrentBoss()) return CheckedDone(sid);
                if (_state.CurrentBoss() && !Controlled(sid))
                {
                    if (targetA == 0 || !ValidGeyser(sid, unchecked((ushort)targetA))) { targetA = unchecked((uint)_hooks.ChooseGeyser!(sid)); SetTargets(sid, targetA, 0); }
                    if (targetA != 0) { _hooks.GoGeyser!(sid, unchecked((ushort)targetA)); return Replaced; }
                }
            }
        }
        if (_state.CurrentBoss())
        {
            point = Position(sid);
            if ((Genus(sid) & PriestGenus) == 0 || _cleanup.NearbyType(point.X, point.Y, 1, _state.DaisType) == 0)
            {
                int x = Cell(point.X), y = Cell(point.Y); var finder = new RawSquidFinder(Pool, _hash, _types);
                // 현재 점의 비표면 후보만 원문 순서로 검사한다.
                for (int candidate = finder.Begin(new(x, y, x, y), 1); candidate != 0; candidate = finder.Next())
                {
                    if ((Genus(candidate) & CarrierGenus) != 0 && candidate != sid && candidate != pickup.Sid) return _hooks.Escape!(sid, 1f, uint.MaxValue);
                    if (Pool.Slot(candidate)[10] == _state.IsleStalagType && !Controlled(sid)) return _hooks.Escape!(sid, 2f, ObstacleEvent);
                }
                if (!Controlled(sid) && (Genus(sid) & 0x20000) == 0)
                {
                    point = Position(sid); RawPathPoint[] positions = [point, new(point.X, (float)((double)point.Y + 2f))]; uint[] genus = [0x4200, 0x200]; float[] delays = [1, 2];
                    // 마지막 두 점은 현재 위치와y+2이며 plain 절삭한 실제 비표면 finder를 사용한다.
                    for (int index = 0; index < positions.Length; index++)
                    {
                        int px = Cell(positions[index].X), py = Cell(positions[index].Y); var obstacles = new RawSquidFinder(Pool, _hash, _types);
                        // 첫 일치 장애물만 실제 회피를 요청한다.
                        for (int candidate = obstacles.Begin(new(px, py, px, py), 1); candidate != 0; candidate = obstacles.Next()) if ((Genus(candidate) & genus[index]) != 0) return _hooks.Escape!(sid, delays[index], ObstacleEvent);
                    }
                }
            }
        }
        return CheckedDone(sid);
    }
}
