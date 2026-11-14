// 로컬 배치 미리보기의 배열·현재 지면·방향 관계와 PATCH/CD 지도 경계를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>일반/사제 경로가 함께 쓰는 원문 미리보기와 관계 전역이다.</summary>
public sealed class CanonPlacementPreviewState
{
    /// <summary>x가 바깥 차원인 고정12×12 차단BYTE 배열이다.</summary>
    public byte[] Blocked { get; } = new byte[144];
    /// <summary>0이 아니면 표면 소유 관계를 허용하는 현재 편집기DWORD다.</summary>
    public uint Editor { get; set; }
    /// <summary>0이 아니면 소유자×9+요청자의 현재 방향 관계를 읽는다.</summary>
    public uint UseAlliances { get; set; }
    /// <summary>행 소유자→열 요청자 방향을 보존하는 현재81DWORD다.</summary>
    public uint[] Alliances { get; } = new uint[81];
}

/// <summary>미리보기의 실제 decoder 범위와 후보/지역 정책 경계다.</summary>
public sealed class CanonPlacementPreviewHooks
{
    /// <summary>타입/별도 패턴 인자로 원문 정수 사각형을 조회한다.</summary>
    public Func<CanonPlacementQuery, SquidSearchArea>? Bounds { get; set; }
    /// <summary>현재 미리보기 배열과 원래 요청으로 전체 후반 충돌/지역을 처리한다.</summary>
    public Func<CanonPlacementQuery, bool, bool>? InspectCollisions { get; set; }
    /// <summary>원문 함수 객체의 값 사본을 만든다.</summary>
    internal CanonPlacementPreviewHooks Copy() => new() { Bounds = Bounds, InspectCollisions = InspectCollisions };
}

/// <summary>로컬 미리보기 표시와 배치 허용을 구별하며 실제 표면의 현재 상태를 읽는다.</summary>
public sealed class RawCanonPlacementPreview
{
    /// <summary>원문 지도 한 변이다.</summary>
    private const int MapSide = 256;
    /// <summary>원문 미리보기 한 변이다.</summary>
    private const int PreviewSide = 12;
    /// <summary>PATCH가 조기 거부하는 첫 지도 행이다.</summary>
    private const int PatchRowEnd = 254;
    /// <summary>소유 관계가 필요한 실제 표면 genus 비트다.</summary>
    private const uint Surface = 2;
    /// <summary>현재 raw 슬롯과 판본을 제공하는 실제 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>표면 조회마다 현재 genus를 읽는 실제 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>원래 칸별 점유BYTE 지도다.</summary>
    private readonly byte[] _spots;
    /// <summary>원래 칸별 표면SID 지도다.</summary>
    private readonly ushort[] _surfaceMap;
    /// <summary>현재 차단 배열·편집기/관계 전역이다.</summary>
    private readonly CanonPlacementPreviewState _state;
    /// <summary>구성 때 복사한 실제 범위/후반 함수다.</summary>
    private readonly CanonPlacementPreviewHooks _hooks;
    /// <summary>판본별 표·고정 두 지도와 필수 함수가 모두 있는지 먼저 확인한다.</summary>
    public RawCanonPlacementPreview(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, byte[] spots, ushort[] surfaceMap, CanonPlacementPreviewState state, CanonPlacementPreviewHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(spots); ArgumentNullException.ThrowIfNull(surfaceMap); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (types.Count != (pool.Edition == OriginalEdition.Patch1078 ? 188 : 171) || spots.Length != MapSide * MapSide || surfaceMap.Length != spots.Length || hooks.Bounds is null || hooks.InspectCollisions is null) throw new ArgumentException("일반 배치 미리보기 자료/하위 경계 누락");
        _pool = pool; _types = types; _spots = spots; _surfaceMap = surfaceMap; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>편집기→동일 소유자→활성화한 실제 방향 관계 인덱스 순서로 판단한다.</summary>
    private bool Related(byte surfaceOwner, int owner)
    {
        if (_state.Editor != 0 || surfaceOwner == owner) return true;
        if (_state.UseAlliances == 0) return false;
        long index = (long)surfaceOwner * 9 + owner;
        if (index < 0 || index >= _state.Alliances.Length) throw new ArgumentOutOfRangeException(nameof(owner), "일반 배치 미리보기 관계 표 범위 오류");
        return _state.Alliances[(int)index] != 0;
    }
    /// <summary>CD도 linear spot을 먼저 읽으며 free/void/dead 추가 필터 없이 현재 표면을 조회한다.</summary>
    private bool CellBlocked(int x, int y, int owner)
    {
        long index = (long)y * MapSide + x;
        if (index < 0 || index >= _spots.Length) throw new ArgumentOutOfRangeException(nameof(x), "일반 배치 미리보기 공간 읽기 범위 오류");
        if ((_spots[(int)index] & 0x10) != 0) return true;
        if (x < 0 || x >= MapSide || y < 0 || y >= MapSide) return true;
        int surface = _surfaceMap[(int)index]; if (surface == 0) return true;
        ReadOnlySpan<byte> raw = _pool.Slot(surface); byte type = raw[10];
        if (type >= _types.Count) throw new ArgumentOutOfRangeException(nameof(surface), "일반 배치 미리보기 표면 타입 범위 오류");
        if ((_types[type].Flags2 & Surface) == 0) return false;
        return !Related(raw[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32], owner);
    }
    /// <summary>반복 경계+1과 미리보기 인덱스를 안전하게 계산할 수 있는지 확인한다.</summary>
    private static void CheckBounds(SquidSearchArea area)
    {
        if (area.Right == int.MaxValue || area.Bottom == int.MaxValue || area.Right < int.MinValue + PreviewSide || area.Bottom < int.MinValue + PreviewSide) throw new ArgumentOutOfRangeException(nameof(area), "일반 배치 미리보기 사각형 산술 범위 오류");
    }
    /// <summary>로컬만 배열을 지우고 오른쪽 아래부터 역순 표시하며 PATCH 경계 거부의 부분 배열도 보존한다.</summary>
    public bool Inspect(CanonPlacementQuery query, bool localOwner)
    {
        if (!localOwner) return _hooks.InspectCollisions!(query, localOwner);
        Array.Clear(_state.Blocked); SquidSearchArea area = _hooks.Bounds!(query); CheckBounds(area);
        int left = Math.Max(area.Left, 1), top = Math.Max(area.Top, 1);
        if (area.Right + 1 < left - 1 || area.Bottom + 1 < top - 1) return _hooks.InspectCollisions!(query, localOwner);
        int owner = unchecked((sbyte)(byte)query.Owner);
        // 원문처럼 y를 먼저 감소시키고 각 행의 오른쪽 주변 칸에서 시작한다.
        for (int y = area.Bottom + 1; y >= top - 1; y--)
        {
            // x가 배열 바깥 차원이며 동일 칸을 임의 전치하지 않는다.
            for (int x = area.Right + 1; x >= left - 1; x--)
            {
                if (_pool.Edition == OriginalEdition.Patch1078 && (x < 0 || y < 0 || x >= MapSide || y >= PatchRowEnd)) return false;
                long px = (long)area.Right - x + 1, py = (long)area.Bottom - y + 1;
                if (px < 0 || py < 0 || px >= PreviewSide || py >= PreviewSide) throw new ArgumentOutOfRangeException(nameof(area), "일반 배치 미리보기 12×12 배열 범위 오류");
                if (CellBlocked(x, y, owner)) _state.Blocked[(int)px * PreviewSide + (int)py] = 1;
            }
        }
        return _hooks.InspectCollisions!(query, localOwner);
    }
    /// <summary>접두/기하와 동일한 실제 풀을 확인하는 참조다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>접두의 후반만 실제 미리보기로 연결하고 픽셀 모양 함수 사본은 보존한다.</summary>
    public static CanonPlacementHooks MakeCanonPlacementPreviewHooks(RawSidPool pool, RawCanonPlacementPreview preview, CanonPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(preview); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, preview.Pool)) throw new ArgumentException("일반 배치 미리보기/배치의 SID 풀이 다릅니다");
        CanonPlacementHooks result = hooks.Copy(); result.InspectGeometry = preview.Inspect; return result;
    }
    /// <summary>별도 패턴 인자/방향의 비트를 유지하고 명시 프레임을 끈 실제 기하 범위를 연결한다.</summary>
    public static CanonPlacementPreviewHooks MakeCanonGeometryPreviewHooks(RawSidPool pool, RawCanonPlacementGeometry geometry, CanonPlacementPreviewHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(geometry); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, geometry.Pool)) throw new ArgumentException("일반 모양/미리보기의 SID 풀이 다릅니다");
        CanonPlacementPreviewHooks result = hooks.Copy(); result.Bounds = q => geometry.Bounds(new(q.Type, unchecked((int)q.Argument), unchecked((int)q.Flags), q.X, q.Y)); return result;
    }
}
