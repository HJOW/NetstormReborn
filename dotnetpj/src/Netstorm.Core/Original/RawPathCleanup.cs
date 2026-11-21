// 공간 접두 뒤 같은 raw helper/부모/내용물/의식·환불 상태를 원문 순서로 정리한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawPathCleanup.cpp의 실제 정리 본체와 현재 finder 조회다.</summary>
public sealed class RawPathCleanup
{
    /// <summary>보조 건설물의 실제 부류다.</summary>
    private const uint HelperGenus = 0x404200;
    /// <summary>반환 장소의 실제 부류다.</summary>
    private const uint ReturnPlaceGenus = 0x200;
    /// <summary>너겟/사제를 조회하는 실제 내용물 마스크다.</summary>
    private const uint NuggetMask = 0x20;
    /// <summary>일반 지면 거의올림과 다른 원문 finder float 상수다.</summary>
    private const float FinderBias = 0.9999899864196777f;
    /// <summary>원문 CRT signed64 절삭의 배타적 상한이다.</summary>
    private const double FtolLimit = 9223372036854775808.0;
    /// <summary>부모/helper/내용물의 같은 실제 풀이다.</summary>
    public RawSidPool Pool { get; }
    /// <summary>현재 일반 finder가 사용하는 실제 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재TYPE의 부류/발자국을 읽는 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>효과 뒤 읽는 같은 현재 전역이다.</summary>
    private readonly PathCleanupState _state;
    /// <summary>원문 값형으로 복사한 실제 함수다.</summary>
    private readonly PathCleanupHooks _hooks;
    /// <summary>판본별TYPE 수와 원문 필수 실제 하위를 첫 효과 전에 확인한다.</summary>
    public RawPathCleanup(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, PathCleanupState state, PathCleanupHooks hooks)
    {
        Pool = pool; _hash = hash; _types = types; _state = state; _hooks = hooks with { };
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.ReserveShared is null || hooks.AddShared is null || hooks.Contained is null || hooks.GraphFor is null || hooks.Ritual is null || hooks.Refund is null || hooks.Destroy is null) throw new ArgumentException("Path cleanup 실제 하위 연결 누락");
    }
    /// <summary>원문 CRT signed64 절삭의 낮은DWORD와 비정상0을 보존한다.</summary>
    private static int Truncate(double value) => !double.IsFinite(value) || value >= FtolLimit || value < -FtolLimit ? 0 : unchecked((int)(long)value);
    /// <summary>현재 float 좌표와 원문 float bias를 double에서 더한 뒤 절삭한다.</summary>
    private static int Cell(float value, bool biased) => Truncate((double)value + (biased ? (double)FinderBias : 0));
    /// <summary>현재 rawTYPE의 실제 부류를 조회한다.</summary>
    private uint Genus(int sid)
    {
        int type = Pool.Slot(sid)[10]; if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(sid), "cleanup 현재 raw 타입 범위"); return _types[type].Flags2;
    }
    /// <summary>현재 소유자의 판본별 BYTE다.</summary>
    private byte Owner(int sid) => Pool.Slot(sid)[Pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
    /// <summary>현재 abstract/매장 extra BYTE다.</summary>
    private byte Extra(int sid) => Pool.Slot(sid)[Pool.Edition == OriginalEdition.Patch1078 ? 40 : 35];
    /// <summary>단순 절삭 사각형에서 원문 abstract/dead/부류 또는TYPE 조건의 첫 반환 장소다.</summary>
    public int ReturnPlaceAt(float x, float y, int radius = 1)
    {
        var finder = new RawSquidFinder(Pool, _hash, _types); double dx = x, dy = y, dr = radius; int sid = finder.Begin(new(Truncate(dx - dr), Truncate(dy - dr), Truncate(dx + dr), Truncate(dy + dr)));
        // 원문에는 추가 free/void 필터가 없으며 실제 finder 순서를 유지한다.
        while (sid != 0) { if ((Extra(sid) & 1) == 0 && (Pool.Slot(sid)[11] & 2) == 0 && ((Genus(sid) & ReturnPlaceGenus) != 0 || Pool.Slot(sid)[10] == _state.OutpostType)) return sid; sid = finder.Next(); } return 0;
    }
    /// <summary>현재 거의올림 좌표와 부류별 finder 시작 단계로 첫 같은TYPE를 찾는다.</summary>
    public int NearbyType(float x, float y, uint radius, uint type)
    {
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(type), "cleanup 근처 타입 범위"); int cx = Cell(x, true), cy = Cell(y, true), distance = unchecked((int)radius); var finder = new RawSquidFinder(Pool, _hash, _types);
        int sid = finder.Begin(new(unchecked(cx - distance), unchecked(cy - distance), unchecked(cx + distance), unchecked(cy + distance)), (_types[(int)type].Flags2 & 6) == 0 ? 1u : 0u);
        // 현재 후보의TYPE만 비교하고 다른 필터를 더하지 않는다.
        while (sid != 0) { if (Pool.Slot(sid)[10] == type) return sid; sid = finder.Next(); } return 0;
    }
    /// <summary>실제 OnFormDestroy 공간 접두 이후 helper예약/너겟/의식/현재helper환불·삭제다.</summary>
    public void AfterSpatial(RawPathRuntime runtime, int parent)
    {
        if (runtime.Bytes.Length != RawPathRuntime.LayoutFor(Pool.Edition).Size) throw new ArgumentException("cleanup 경로 판본 폭 불일치"); uint helper = runtime.Helper;
        if (helper != 0 && helper < Pool.Capacity && (Genus((ushort)helper) & HelperGenus) != 0)
        {
            RawPathPoint point = RawPathSlot.Position(Pool, parent); int x = Cell(point.X, true), y = Cell(point.Y, true); var finder = new RawSquidFinder(Pool, _hash, _types); int sid = finder.Begin(new(x - 2, y - 2, x + 2, y + 2));
            // 실제 helper를 찾은 첫 위치에서만 예약하고 할당 실패도 현재helper를 지운다.
            while (sid != 0) { if (sid == helper) { if (_hooks.ReserveShared!()) _hooks.AddShared!((ushort)runtime.Helper, _state.HelperEvent, 1f); runtime.ClearHelper(); break; } sid = finder.Next(); }
        }
        if (Pool.Edition == OriginalEdition.Patch1078 && (Pool.Slot(parent)[11] & 2) == 0)
        {
            RawPathPoint point = RawPathSlot.Position(Pool, parent); int place = ReturnPlaceAt(point.X, point.Y), nugget = _hooks.Contained!(parent, NuggetMask, _state.CurrentNuggetType(), 0);
            if (place != 0 && nugget != 0 && Owner(place) == Owner(parent)) { uint parentGraph = _hooks.GraphFor!(parent), placeGraph = _hooks.GraphFor(place); if (placeGraph == parentGraph && _state.Boss) _hooks.Destroy!(nugget, 0); }
        }
        if (Pool.Slot(parent)[10] == _state.PriestType) { RawPathPoint point = RawPathSlot.Position(Pool, parent); int temple = NearbyType(point.X, point.Y, 2, _state.DaisType); if (temple != 0 && (Extra(temple) & 8) == 0) _hooks.Ritual!(temple, parent, 0); }
        uint remaining = runtime.Helper;
        if (_state.Boss && remaining != 0)
        {
            if (remaining >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(runtime), "cleanup 남은 helper SID 범위"); int target = (ushort)remaining;
            if ((Pool.Slot(target)[11] & 1) == 0)
            {
                if ((Extra(target) & 1) == 0 || (Genus(target) & HelperGenus) == 0) { if (_state.Checking && Pool.Edition == OriginalEdition.Patch1078) throw new InvalidOperationException("cleanup shouldDestroy 원본 assert"); return; }
                _hooks.Refund!(Pool.Slot(target)[10], Owner(target)); _hooks.Destroy!(target, 0);
            }
        }
    }
    /// <summary>cpppj MakePathCleanupHooks처럼 같은 carrier/anchor/Host/건설 환불만 연결하고 예약·의식·삭제 경계는 유지한다.</summary>
    public static PathCleanupHooks MakeHooks(RawSidPool pool, RawCarrierPreDestroy carrier, RawPlayerPlacementAnchor anchor, SquidProcessHost host, RawConstructionPlace place, PathCleanupHooks hooks)
    {
        if (!ReferenceEquals(pool, carrier.Pool) || !ReferenceEquals(pool, anchor.Pool) || !ReferenceEquals(pool, host.Pool()) || !ReferenceEquals(pool, place.Pool)) throw new ArgumentException("cleanup 하위 모듈 SID 풀 불일치");
        return hooks with { Contained = (sid, mask, kind, index) => carrier.FindContained(sid, mask, kind, index), GraphFor = sid => anchor.GraphFor(unchecked((uint)sid)), AddShared = (sid, ev, payload) => host.AddSharedRegular(sid, ev, payload), Refund = place.Refund };
    }
}
