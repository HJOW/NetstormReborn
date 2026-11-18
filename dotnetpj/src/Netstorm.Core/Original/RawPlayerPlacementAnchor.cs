// Player 배치 기준점의 현재 작업장/추가 목록·그래프·종속 조건과 거리 선택을 복원한다.
using System.Globalization;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 플레이어 배치 앵커 처리 중 조회·갱신하는 상태 묶음이다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 실제 Player 조회에 쓴다.
/// 근거: cpppj/src/o/RawPlayerPlacementAnchor.h.
/// </summary>
/// <remarks>2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 앵커다.</remarks>
public sealed class PlayerPlacementAnchorState
{
    /// <summary>실제 그래프 활성이다. 0이면 좌표를 읽기 전에 고정 무효 값 254를 돌려준다.</summary>
    public uint GraphReady { get; set; }

    /// <summary>무효 그래프 번호다. 이 값이면 후보 목록을 비워 조기 반환한다.</summary>
    public uint InvalidGraph { get; set; } = 254;

    /// <summary>독립 raw용 종속 조건 우회 fallback이다. 공유 전체 덱이 있으면 그 값을 대신 읽는다.</summary>
    public uint IgnoreRestrictions { get; set; }

    /// <summary>패치 raw graph getter가 surface 이외에도 허용하는 현재 타입 번호다.</summary>
    public uint FenceType { get; set; }

    /// <summary>패치 raw graph getter가 surface 이외에도 허용하는 현재 타입 번호다.</summary>
    public uint WindArcherType { get; set; }

    /// <summary>실제0054db14 owner를 빌리며 상태 값 복사도 같은 주소의 현재 값을 소비한다.</summary>
    public uint[]? SharedFullDeckSource { get; set; }

    /// <summary>00462bc0의 mySid 진단이 복귀하면 원문대로 0을 반환한다.</summary>
    public Action<string, string, uint>? GraphAssertion { get; set; }

    /// <summary>후보별 종속 조회 뒤 DWORD 전체를 정규화 없이 다시 읽는다.</summary>
    /// <returns>공유 전체 덱이 있으면 그 값, 없으면 독립 값이다.</returns>
    public uint IgnoreRestrictionsValue => SharedFullDeckSource is { Length: > 0 } source ? source[0] : IgnoreRestrictions;
}

/// <summary>
/// 플레이어 앵커가 선택한 대상과 배치 기준 정보다.
/// 순서·중복·40개 상한을 유지한 함수 내부 임시 목록의 관찰이다.
/// </summary>
/// <remarks>2026-11-18 0.610에서 추가했다. 근거: cpppj/src/o/RawPlayerPlacementAnchor.h.</remarks>
public sealed class PlayerAnchorSelection
{
    /// <summary>후보가 하나면 그대로, 여러 개면 소유자/거리 선택의 원본 반환이다.</summary>
    public uint Sid { get; set; }

    /// <summary>순서·중복·40개 상한을 유지한 후보 목록이다.</summary>
    public List<uint> Candidates { get; } = new();
}

/// <summary>
/// Player 배치 기준점의 현재 작업장/추가 목록·그래프·종속 조건과 거리 선택이다.
/// R6-3 캐논 배치 요구사항 전용 모듈로, 적재 스코프 농지 MayPlace의 실제 Player 조회에 쓴다.
/// 근거: cpppj/src/o/RawPlayerPlacementAnchor.h/.cpp, 원본 0048fdb0·00462b80·00462bc0·0048fd30.
/// </summary>
/// <remarks>2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다. R6-3 I 영역의 3단위 중 앵커다.</remarks>
public sealed class RawPlayerPlacementAnchor
{
    /// <summary>원본 임시 목록 상한이다.</summary>
    private const int Limit = 40;

    /// <summary>거리 선택의 단정도 초기 최소값이다.</summary>
    private const float InitialDistance = 9999.0f;

    /// <summary>디컴파일에 생략된 fadd DWORD 상수(00500edc / CD 00502418)다.</summary>
    private const float CoordinateBias = 0.9999f;

    /// <summary>공통 raw 타입·좌표·종속 kind/mask의 오프셋이다.</summary>
    private const int TypeOffset = 10;

    /// <summary>원본 X 좌표 DWORD 위치다.</summary>
    private const int XOffset = 14;

    /// <summary>원본 Y 좌표 DWORD 위치다.</summary>
    private const int YOffset = 18;

    /// <summary>종속 kind WORD 위치다.</summary>
    private const int KindOffset = 18;

    /// <summary>종속 mask WORD 위치다.</summary>
    private const int MaskOffset = 20;

    /// <summary>같은 풀의 슬롯을 읽는 SID 풀이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 표면의 배치·소유권 자료다. 같은 지도 WORD 배열을 복사하지 않는다.</summary>
    private readonly IReadOnlyList<ushort> _surfaces;

    /// <summary>객체 종류·소유자별 등록 목록과 개수를 관리하는 장부다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawPostPopState _bookkeeping;

    /// <summary>원본 별도 추가 목록을 공통 factories와 구분한다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawPostPopList _additional;

    /// <summary>객체 내용물의 생성·검색·등록 처리 자료다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly ContainedFinderState _contained;

    /// <summary>처리 중 조회하는 상태다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly PlayerPlacementAnchorState _state;

    /// <summary>
    /// 원본 별도 추가 목록을 공통 factories와 구분한다. 모든 자료는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다. 판본별 개수여야 한다.</param>
    /// <param name="surfaces">현재 표면의 배치·소유권 자료(65,536칸)다.</param>
    /// <param name="bookkeeping">객체 종류·소유자별 등록 목록과 개수다.</param>
    /// <param name="additional">원본 별도 추가 목록이다.</param>
    /// <param name="contained">객체 내용물 처리 자료다.</param>
    /// <param name="state">처리 중 조회하는 상태다.</param>
    public RawPlayerPlacementAnchor(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<ushort> surfaces,
        RawPostPopState bookkeeping,
        RawPostPopList additional,
        ContainedFinderState contained,
        PlayerPlacementAnchorState state)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(surfaces);
        ArgumentNullException.ThrowIfNull(bookkeeping);
        ArgumentNullException.ThrowIfNull(additional);
        ArgumentNullException.ThrowIfNull(contained);
        ArgumentNullException.ThrowIfNull(state);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected || surfaces.Count != 65536)
        {
            throw new ArgumentException("Player 배치 기준점 자료 크기 오류", nameof(pool));
        }

        _pool = pool;
        _types = types;
        _surfaces = surfaces;
        _bookkeeping = bookkeeping;
        _additional = additional;
        _contained = contained;
        _state = state;
    }

    /// <summary>같은 풀의 실제 Player 조회인지 확인한다.</summary>
    /// <returns>이 모듈이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 손상된 DWORD 목록 번호를 임의의 다른 WORD SID로 감추지 않는다.
    /// </summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>슬롯 바이트다.</returns>
    private ReadOnlySpan<byte> Raw(uint sid)
    {
        if (sid > ushort.MaxValue || sid >= (uint)_pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Player 기준점 SID 오류");
        }

        return _pool.Slot((int)sid);
    }

    /// <summary>
    /// CD getter는 타입 assert 없이 raw graph를 읽으며 패치만 현재 타입 조건을 확인한다.
    /// </summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>raw graph 바이트다.</returns>
    private uint RawGraphByte(uint sid)
    {
        ReadOnlySpan<byte> raw = Raw(sid);
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (patch && (raw[TypeOffset] >= _types.Count || ((_types[raw[TypeOffset]].Flags1 & 0x800) == 0 && raw[TypeOffset] != _state.FenceType && raw[TypeOffset] != _state.WindArcherType)))
        {
            throw new ArgumentException("Player 표면 graph getter 타입 오류");
        }

        return raw[patch ? 30 : 28];
    }

    /// <summary>
    /// 비정렬 원본 float 비트값을 DWORD로 읽는다. 낮은 바이트부터 조립하며 NaN·부호 있는 0을 산술 변환하지 않는다.
    /// </summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>비트 그대로의 float 값이다.</returns>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 낮은 바이트부터 조립하는 반복문이다.
        for (int i = 0; i < 4; ++i)
        {
            value |= (uint)raw[offset + i] << (i * 8);
        }

        return BitConverter.UInt32BitsToSingle(value);
    }

    /// <summary>
    /// 종속 조회가 쓰는 비정렬 WORD를 읽는다.
    /// </summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>WORD 값이다.</returns>
    private static ushort Word(ReadOnlySpan<byte> raw, int offset) => (ushort)(raw[offset] | (raw[offset + 1] << 8));

    /// <summary>
    /// x87의 좌표+float 상수는 double에서 계산한 뒤 float으로 좁히지 않고 CRT 절삭한다.
    /// </summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭한 정수 좌표다.</returns>
    private static int Truncate(float value)
    {
        double result = Math.Truncate((double)value + CoordinateBias);
        if (!double.IsFinite(result) || result < int.MinValue || result > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "Player 그래프 좌표 절삭 오류");
        }

        return (int)result;
    }

    /// <summary>
    /// 00462b80 / CD 0045b670: 0.9999를 더해 0쪽 절삭한 surface 지도에서 raw graph 바이트를 읽는다.
    /// </summary>
    /// <param name="x">조회할 월드 X 좌표다.</param>
    /// <param name="y">조회할 월드 Y 좌표다.</param>
    /// <returns>graph 바이트다. 비활성·지도 밖은 무효 값 254다.</returns>
    public uint GraphAt(float x, float y)
    {
        if (_state.GraphReady == 0)
        {
            return 254;
        }

        int px = Truncate(x), py = Truncate(y);
        if (px < 0 || py < 0 || px > 255 || py > 255)
        {
            return 254;
        }

        ushort sid = _surfaces[py * 256 + px];
        return sid != 0 ? RawGraphByte(sid) : 254;
    }

    /// <summary>
    /// 00462bc0 / CD 0045b6f0: SID 0→surface raw 번호 또는 비표면 좌표 조회다.
    /// </summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    /// <returns>graph 값이다.</returns>
    public uint GraphFor(uint sid)
    {
        if (sid == 0)
        {
            if (_pool.Edition == OriginalEdition.Patch1078 && _contained.CheckingDead)
            {
                Action<string, string, uint> assertion = _state.GraphAssertion ?? throw new ArgumentException("Player graph SID 0 실제 진단 미연결");
                assertion("mySid", "\\Ns\\O\\Graph.cpp", 0x93);
            }

            return 0;
        }

        ReadOnlySpan<byte> raw = Raw(sid);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "Player graph 타입 오류");
        }

        return (_types[raw[TypeOffset]].Flags1 & 0x800) != 0 ? RawGraphByte(sid) : GraphAt(Coordinate(raw, XOffset), Coordinate(raw, YOffset));
    }

    /// <summary>
    /// 기존 contained 커서를 사용하여 저장된 다음 링크와 현재 타입 전역을 유지한다.
    /// </summary>
    /// <param name="parent">부모 슬롯 번호다.</param>
    /// <param name="kind">요청 kind다. 0이면 종류를 묻지 않는다.</param>
    /// <returns>조건을 만족하는 내용물이 있으면 참이다.</returns>
    private bool HasContained(uint parent, uint kind)
    {
        _ = Raw(parent);
        var finder = new RawContainedFinder(_pool, _contained);
        // 일치 타입 후보만 요청 kind와 mask로 검사하며 인덱스는 항상 0이다.
        for (int sid = finder.Begin((int)parent); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> raw = _pool.Slot(sid);
            if ((kind == 0 || Word(raw, KindOffset) == kind) && (Word(raw, MaskOffset) & 0x10) != 0)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>
    /// 0048fd30 / CD 00406ef0: owner가 같은 후보의 거리/float 최소값(초기 9999)을 비교한다. CD는 NaN도 채택한다.
    /// </summary>
    /// <param name="candidates">후보 SID 목록이다.</param>
    /// <param name="owner">요청 소유자다.</param>
    /// <param name="x">기준 X 좌표다.</param>
    /// <param name="y">기준 Y 좌표다.</param>
    /// <returns>선택된 SID다. 없으면 0이다.</returns>
    public uint Nearest(IReadOnlyList<uint> candidates, uint owner, float x, float y)
    {
        ArgumentNullException.ThrowIfNull(candidates);
        float best = InitialDistance;
        uint selected = 0;
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        // 현재 raw 소유자와 좌표를 목록 순서대로 읽는 반복문이다. 동률도 float 최소값과 비교한다.
        foreach (uint sid in candidates)
        {
            ReadOnlySpan<byte> raw = Raw(sid);
            if (raw[patch ? 34 : 32] != owner)
            {
                continue;
            }

            double dx = (double)x - Coordinate(raw, XOffset), dy = (double)y - Coordinate(raw, YOffset);
            double distance = Math.Sqrt(dx * dx + dy * dy);
            bool unordered = double.IsNaN(distance) || float.IsNaN(best);
            if (distance < best || (!patch && unordered))
            {
                best = (float)distance;
                selected = sid;
            }
        }

        return selected;
    }

    /// <summary>
    /// type와 owner는 실제로 읽는 시점에 검사하여 무효 그래프의 조기 반환을 보존한다.
    /// </summary>
    /// <param name="owner">요청 소유자다.</param>
    /// <param name="type">요청 타입 번호다.</param>
    /// <param name="x">기준 X 좌표다.</param>
    /// <param name="y">기준 Y 좌표다.</param>
    /// <returns>선택 결과다.</returns>
    public PlayerAnchorSelection Inspect(uint owner, uint type, float x, float y)
    {
        var result = new PlayerAnchorSelection();
        uint graph = GraphAt(x, y);
        if (graph == _state.InvalidGraph)
        {
            return result;
        }

        if (owner >= (uint)_bookkeeping.OwnerFactories.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "Player 기준점 소유자 오류");
        }

        ReadOnlySpan<uint> workshops = _bookkeeping.OwnerFactories[owner].Items();
        bool linked = false;
        // 목록을 매번 다시 읽고 중복·원래 순서·상한 이후의 조회까지 유지하는 반복문이다.
        for (int i = 0; i < workshops.Length; ++i)
        {
            uint sid = workshops[i];
            _ = Raw(sid);
            if (type >= (uint)_types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(type), "Player 기준점 요청 타입 오류");
            }

            bool allowed = (_types[(int)type].Flags2 & 6) != 0;
            if (!allowed)
            {
                allowed = HasContained(sid, type);
                linked = linked || allowed;
                if (!allowed)
                {
                    allowed = _state.IgnoreRestrictionsValue != 0;
                }
            }

            if (allowed && GraphFor(sid) == graph && result.Candidates.Count < Limit)
            {
                result.Candidates.Add(sid);
            }
        }

        if (result.Candidates.Count != 0 || linked)
        {
            // 추가 목록은 현재 raw owner가 같은 것만 받고 그래프 조회 뒤 40개 상한을 적용하는 반복문이다.
            foreach (uint sid in _additional.Items())
            {
                ReadOnlySpan<byte> raw = Raw(sid);
                if (raw[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32] == owner && GraphFor(sid) == graph && result.Candidates.Count < Limit)
                {
                    result.Candidates.Add(sid);
                }
            }
        }

        if (result.Candidates.Count == 1)
        {
            result.Sid = result.Candidates[0];
        }
        else if (result.Candidates.Count != 0)
        {
            result.Sid = Nearest(result.Candidates, owner, x, y);
        }

        return result;
    }

    /// <summary>
    /// 0048fdb0 / CD 00406f80: 그래프→작업장→조건부 추가 목록→하나/거리 선택 순서의 DWORD 반환이다.
    /// </summary>
    /// <param name="owner">요청 소유자다.</param>
    /// <param name="type">요청 타입 번호다.</param>
    /// <param name="x">기준 X 좌표다.</param>
    /// <param name="y">기준 Y 좌표다.</param>
    /// <returns>배치 기준 객체 값이다.</returns>
    public uint Locate(uint owner, uint type, float x, float y) => Inspect(owner, type, x, y).Sid;

    /// <summary>
    /// 같은 풀의 실제 Player 조회를 후보 권한의 필수 경계에 연결한다.
    /// </summary>
    /// <param name="pool">같은 풀의 실제 Player 조회다. 다른 풀의 몸체와 섞으면 거부한다.</param>
    /// <param name="anchor">같은 풀의 앵커 몸체다.</param>
    /// <returns>owner·type·좌표로 배치 기준 객체를 찾는 콜백이다.</returns>
    public static Func<uint, uint, float, float, uint> MakePlayerAnchorQuery(RawSidPool pool, RawPlayerPlacementAnchor anchor)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(anchor);
        if (!ReferenceEquals(pool, anchor.Pool))
        {
            throw new ArgumentException("Player 기준점/권한 SID 풀 불일치", nameof(pool));
        }

        return (owner, type, x, y) => anchor.Locate(owner, type, x, y);
    }
}
