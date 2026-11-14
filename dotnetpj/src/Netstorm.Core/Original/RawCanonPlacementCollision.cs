using System.Buffers.Binary;

// 일반 배치의 원문 무시 비트값·현재 전역·후보 표시와 실제 finder 거부 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>후보마다 읽는 Client/무시genus의 독립 값 또는 실제 공유 뷰다.</summary>
public sealed class CanonPlacementCollisionState
{
    /// <summary>공유 역할을 연결하지 않았을 때 예측 가능한 ClientSID를 제외하는 전체DWORD다.</summary>
    public uint ClientMode { get; set; }
    /// <summary>공유 마스크를 연결하지 않았을 때 배치genus와 비교하는 원문DWORD다.</summary>
    public uint IgnoredGenus { get; set; }
    /// <summary>연결 시 후보별 현재 건설/경로 공유 마스크를 읽는다.</summary>
    public Func<uint>? SharedIgnoredGenus { get; set; }
    /// <summary>연결 시 같은 실제 역할 owner의 현재 비영 상태를 읽는다.</summary>
    public Func<bool>? SharedServer { get; set; }
    /// <summary>각 후보의 무시 판정 직전에 현재 전체 마스크를 읽는다.</summary>
    public uint ReadIgnoredGenus() => SharedIgnoredGenus is { } read ? read() : IgnoredGenus;
    /// <summary>후보의 초기 SID 관문에서 실제 공유 역할 또는 독립 비영 상태를 읽는다.</summary>
    public bool ReadClientMode() => SharedServer is { } read ? read() : ClientMode != 0;
}

/// <summary>실제 칸별 탐색 요청과 무시 후보까지 포함하는 현재 지형 정책이다.</summary>
public sealed class CanonPlacementCollisionHooks
{
    /// <summary>각 모양에 실제 finder 사각형을 요청하는 전체 지역 정책이다.</summary>
    public Func<CanonPlacementQuery, Func<SquidSearchArea, bool>, bool>? InspectRegions { get; set; }
    /// <summary>후보가 충돌을 통과한 뒤 Next 전에 수행하는 현재 지형/지역 효과다.</summary>
    public Action<CanonPlacementQuery, int>? InspectCandidateTerrain { get; set; }
    /// <summary>장기 실제 모듈은 빌리며 함수 객체는 복사한다.</summary>
    internal CanonPlacementCollisionHooks Copy() => new() { InspectRegions = InspectRegions, InspectCandidateTerrain = InspectCandidateTerrain };
}

/// <summary>0049ade0 무시helper와 일반 후보 충돌/배열 표시·실제 해시 탐색을 연결한다.</summary>
public sealed class RawCanonPlacementCollision
{
    /// <summary>예측 가능한 Client SID 구간의 첫 번호다.</summary>
    private const int FirstClient = 5;
    /// <summary>미리보기 고정 한 변이다.</summary>
    private const int Side = 12;
    /// <summary>미리보기 주변 한 칸을 제외한 내부 첫 인덱스다.</summary>
    private const int Interior = 13;
    /// <summary>현재 raw와 판본을 읽는 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>각 사각형에 실제 finder를 구성하는 공간 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>후보마다 현재 배치/후보 속성을 읽는 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>후보마다 현재 역할/무시 마스크를 읽는 공유 상태다.</summary>
    private readonly CanonPlacementCollisionState _state;
    /// <summary>후보 거부 전에도 표시를 기록하는 실제 미리보기 배열이다.</summary>
    private readonly CanonPlacementPreviewState _preview;
    /// <summary>구성 때 확인한 실제 지역/후보 효과의 함수 사본이다.</summary>
    private readonly CanonPlacementCollisionHooks _hooks;
    /// <summary>판본별 표와 필수 두 정책을 확인하고 같은 실제 자료를 빌린다.</summary>
    public RawCanonPlacementCollision(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, CanonPlacementCollisionState state, CanonPlacementPreviewState preview, CanonPlacementCollisionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(preview); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || hooks.InspectRegions is null || hooks.InspectCandidateTerrain is null) throw new ArgumentException("일반 타입 충돌 자료/지역 경계 누락");
        _pool = pool; _hash = hash; _types = types; _state = state; _preview = preview; _hooks = hooks.Copy();
    }
    /// <summary>원문 조기 분기/판본별 마스크의 전체 반환 비트를 bool로 축소하지 않고 보존한다.</summary>
    public static uint PlacementIgnoreValue(OriginalEdition edition, OriginalTypeRecord placing, OriginalTypeRecord candidate, uint mode, uint ignoredGenus)
    {
        uint own = placing.Flags2, other = candidate.Flags2;
        if (mode == 0 && (own & 0x20000) != 0) return 1;
        if ((other & 0x02000000) != 0) return 1;
        if ((other & 0x120000) != 0) return 1;
        if ((own & 4) != 0 && (other & (edition == OriginalEdition.Patch1078 ? 0x200000u : 0x208000u)) != 0) return 1;
        if ((other & 2) != 0 && (placing.Flags1 & 0x800) == 0) return 1;
        if ((other & 0x30006) != 0 && (own & 0x208100) != 0) return 1;
        if ((ignoredGenus & own) != 0) return other & (edition == OriginalEdition.Patch1078 ? 0x218000u : 0x210000u);
        if (mode == 0 && (own & 0x30000) != 0) return 1;
        return 0;
    }
    /// <summary>일반 타입의 genus는 제한하지 않고 현재 번호/유한 좌표만 검사한다.</summary>
    private OriginalTypeRecord Placing(CanonPlacementQuery query)
    {
        if (query.Type >= _types.Count || !float.IsFinite(query.X) || !float.IsFinite(query.Y)) throw new ArgumentException("일반 타입 충돌 요청 타입/좌표 오류");
        return _types[(int)query.Type];
    }
    /// <summary>현재TYPE→좌표→ClientSID→현재 무시helper→로컬표시→extra/mode 순서의 한 후보다.</summary>
    public bool InspectCandidate(CanonPlacementQuery query, bool localOwner, int candidate)
    {
        OriginalTypeRecord placing = Placing(query); ReadOnlySpan<byte> raw = _pool.Slot(candidate); byte type = raw[10];
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(candidate), "일반 타입 충돌 후보 타입 오류");
        OriginalTypeRecord other = _types[type]; float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..])), y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..])); int firstServer = _pool.Edition == OriginalEdition.Patch1078 ? 15000 : 6000;
        if (_state.ReadClientMode() && candidate >= FirstClient && candidate < firstServer) return true;
        if (PlacementIgnoreValue(_pool.Edition, placing, other, query.Mode, _state.ReadIgnoredGenus()) != 0) return true;
        if (localOwner)
        {
            if (!float.IsFinite(x) || !float.IsFinite(y) || other.FootX < 0 || other.FootY < 0 || other.FootX > 256 || other.FootY > 256) throw new ArgumentOutOfRangeException(nameof(candidate), "일반 타입 충돌 후보 발자국/좌표 오류");
            // 후보의 아래쪽부터 y를 늘리고 각 행에서 x를 늘린다. 중간 float 저장을 추가하지 않는다.
            for (int cy = 0; cy < other.FootY; cy++)
            {
                // 빈 가로 발자국이면 표시/좌표 계산을 수행하지 않는다.
                for (int cx = 0; cx < other.FootX; cx++)
                {
                    double dx = Math.Truncate((double)query.X - ((double)x - cx)), dy = Math.Truncate((double)query.Y - ((double)y - cy));
                    if (dx < 0 || dy < 0 || dx >= placing.FootX || dy >= placing.FootY) continue;
                    double index = dx * Side + dy + Interior;
                    if (index >= _preview.Blocked.Length) throw new ArgumentOutOfRangeException(nameof(candidate), "일반 타입 충돌 미리보기 배열 오류");
                    _preview.Blocked[(int)index] = 1;
                }
            }
        }
        return (raw[_pool.Edition == OriginalEdition.Patch1078 ? 40 : 35] & 1) != 0 && query.Mode == 0;
    }
    /// <summary>실제 finder의 기본 필터/flag0으로 후보를 순회하고 첫 거부 뒤 Next/지형을 생략한다.</summary>
    public bool InspectArea(CanonPlacementQuery query, bool localOwner, SquidSearchArea area)
    {
        Placing(query); var finder = new RawSquidFinder(_pool, _hash, _types);
        // finder가 미리 저장한 next를 유지하며 무시된 후보도 지형 효과 뒤 Next로 진행한다.
        for (int sid = finder.Begin(area); sid != 0; sid = finder.Next())
        {
            if (!InspectCandidate(query, localOwner, sid)) return false;
            _hooks.InspectCandidateTerrain!(query, sid);
        }
        return true;
    }
    /// <summary>지역 정책이 거부 뒤 scan을 재요청하거나 true를 반환해도 첫 거부를 유지한다.</summary>
    public bool Inspect(CanonPlacementQuery query, bool localOwner)
    {
        Placing(query); bool rejected = false;
        bool allowed = _hooks.InspectRegions!(query, area => { if (rejected) return false; rejected = !InspectArea(query, localOwner, area); return !rejected; }); return allowed && !rejected;
    }
    /// <summary>미리보기/기하와 같은 실제 풀인지 확인한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>미리보기의 후보/지역 검사만 실제 충돌로 바꾸고 범위 함수 사본은 보존한다.</summary>
    public static CanonPlacementPreviewHooks MakeCanonPlacementCollisionHooks(RawSidPool pool, RawCanonPlacementCollision collision, CanonPlacementPreviewHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(collision); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, collision.Pool)) throw new ArgumentException("일반 타입 충돌/미리보기의 SID 풀이 다릅니다");
        CanonPlacementPreviewHooks result = hooks.Copy(); result.InspectCollisions = collision.Inspect; return result;
    }
    /// <summary>요청마다 현재 지역 정책을 만들고 실제 패턴/방향과 칸별 finder 범위를 연결한다.</summary>
    public static CanonPlacementCollisionHooks MakeCanonGeometryCollisionHooks(RawSidPool pool, RawCanonPlacementGeometry geometry, Func<CanonPlacementQuery, CanonPlacementGeometryHooks> regions, CanonPlacementCollisionHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(geometry); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, geometry.Pool) || regions is null) throw new ArgumentException("일반 모양/충돌의 SID 풀 또는 지역 정책 오류");
        CanonPlacementCollisionHooks result = hooks.Copy(); result.InspectRegions = (q, scan) => { CanonPlacementGeometryHooks policy = regions(q); return geometry.Inspect(new(q.Type, unchecked((int)q.Argument), unchecked((int)q.Flags), q.X, q.Y), cell => scan(cell.Area), policy); }; return result;
    }
}
