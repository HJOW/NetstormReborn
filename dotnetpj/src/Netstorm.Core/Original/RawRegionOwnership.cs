// 지역 소유권 투표와 패치 도장 요청 순서를 복원한다.
namespace Netstorm.Core.Original;

// 지역 소유권을 담는 네임스페이스다. 2026-11-18 0.618에서 R4-2 지역 칠하기·소유자 결선용으로 추가했다.

/// <summary>
/// 소유권 변경의 영향을 받아 갱신할 지역 정보다.
/// 근거: cpppj <c>o/RawRegionOwnership.h</c>의 <c>OwnershipAffectedRegion</c>.
/// </summary>
/// <param name="X">영향을 받은 가로 좌표다. 투표에는 쓰지 않는다.</param>
/// <param name="Y">영향을 받은 세로 좌표다. 투표에는 쓰지 않는다.</param>
/// <param name="Region">영향을 받은 지역 번호다.</param>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 소유권 투표용으로 추가했다.</remarks>
public readonly record struct OwnershipAffectedRegion(float X, float Y, uint Region);

/// <summary>
/// 지역 소유권 처리 중 조회·갱신하는 상태 묶음이다.
/// 근거: cpppj <c>o/RawRegionOwnership.h</c>의 <c>RegionOwnershipState</c>.
/// </summary>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 소유권 투표용으로 추가했다. 장기 주인은 같은 원본 DWORD를 빌린다.</remarks>
public sealed class RegionOwnershipState
{
    /// <summary>소유권 변경의 영향을 받아 갱신할 지역 목록이다.</summary>
    /// <remarks>투표 뒤 재투표가 목록 전체를 교체하므로 읽기·쓰기가 모두 가능하다.</remarks>
    public List<OwnershipAffectedRegion> Affected { get; set; } = [];

    /// <summary>독립 입력의 패치 0059a8b0 증가값이다.</summary>
    public uint Revision { get; set; }

    /// <summary>독립 입력의 005949d4 쓰기값이다.</summary>
    public uint Dirty { get; set; }

    /// <summary>패치 0059a8b0 증가를 빌리는 같은 실제 DWORD 소유자다. 없으면 독립 값을 쓴다.</summary>
    public UIntBox? SharedWorldRedraw { get; set; }

    /// <summary>005949d4 쓰기를 빌리는 같은 실제 DWORD 소유자다. 없으면 독립 값을 쓴다.</summary>
    public UIntBox? SharedMapRevision { get; set; }

    /// <summary>투표 효과 뒤 현재 월드 다시 그리기 DWORD를 직접 읽는다.</summary>
    /// <returns>공유 주인이 있으면 그 값, 없으면 독립 값이다.</returns>
    public uint WorldRedraw => SharedWorldRedraw?.Value ?? Revision;

    /// <summary>투표 효과 뒤 현재 월드 다시 그리기 DWORD에 쓴다.</summary>
    /// <param name="value">쓸 값이다.</param>
    public void SetWorldRedraw(uint value)
    {
        // 공유 주인이 있으면 그곳에 쓰고 독립 값은 그대로 둔다.
        if (SharedWorldRedraw is not null)
        {
            SharedWorldRedraw.Value = value;
        }
        else
        {
            Revision = value;
        }
    }

    /// <summary>현재 미니맵 DWORD를 읽는다.</summary>
    /// <returns>공유 주인이 있으면 그 값, 없으면 독립 값이다.</returns>
    public uint MapRevision => SharedMapRevision?.Value ?? Dirty;

    /// <summary>현재 미니맵 DWORD에 쓴다.</summary>
    /// <param name="value">쓸 값이다.</param>
    public void SetMapRevision(uint value)
    {
        // 공유 주인이 있으면 그곳에 쓰고 독립 값은 그대로 둔다.
        if (SharedMapRevision is not null)
        {
            SharedMapRevision.Value = value;
        }
        else
        {
            Dirty = value;
        }
    }
}

/// <summary>
/// 지역 소유권 표시에 적용할 소유자·테마 정보다.
/// 근거: cpppj <c>o/RawRegionOwnership.h</c>의 <c>RegionOwnershipPaint</c>.
/// </summary>
/// <param name="X">원래 query float를 편향 없이 절삭한 가로 좌표다.</param>
/// <param name="Y">원래 query float를 편향 없이 절삭한 세로 좌표다.</param>
/// <param name="Owner">객체나 지역을 소유한 플레이어 번호다.</param>
/// <param name="Theme">테마다.</param>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 소유권 투표용으로 추가했다.</remarks>
public readonly record struct RegionOwnershipPaint(int X, int Y, uint Owner, uint Theme);

/// <summary>
/// 지역 소유권 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
/// 근거: cpppj <c>o/RawRegionOwnership.h</c>의 <c>RegionOwnershipHooks</c>.
/// </summary>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 소유권 투표용으로 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class RegionOwnershipHooks
{
    /// <summary>실제 좌표 snap·지역 getter다.</summary>
    public Func<float, float, uint>? RegionAt { get; set; }

    /// <summary>00470900·CD 004bd2d0의 flood·지형 도장 필수 외부 경계다.</summary>
    public Action<RegionOwnershipPaint>? Paint { get; set; }

    /// <summary>패치의 theammode 정수 조회다. 같은 소유자의 신전마다 새로 조회한다.</summary>
    public Func<uint>? ThemeMode { get; set; }

    /// <summary>Native는 선택한 TYPE의 현재 +98 DWORD를 읽고 Session 테마 사본을 사용하지 않는다.</summary>
    public Func<uint, uint>? TypeTheme { get; set; }
}

/// <summary>
/// outpost·신전 지역 투표와 판본별 도장 요청 순서를 복원한다.
/// 근거: cpppj <c>o/RawRegionOwnership.h/.cpp</c>, 원본 00455380·CD 0047b3c0.
/// </summary>
/// <remarks>2026-11-18 0.618에서 R4-2 지역 소유권 투표용으로 추가했다. 실제 농지 호출·사용자 건설 통합은 R4-6·R6에서 검증한다.</remarks>
public sealed class RawRegionOwnership
{
    /// <summary>비정렬 raw 좌표와 두 판본에 같은 float 편향 snap의 정확한 비트다.</summary>
    private const uint SnapBiasBits = 0x3f7fff58u;

    /// <summary>같은 풀의 슬롯을 읽는 SID 풀이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>추가 목록이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawPostPopList _additional;

    /// <summary>신전 목록이다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RawPostPopList _temples;

    /// <summary>테마다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly IReadOnlyList<uint> _themes;

    /// <summary>처리 중 조회·갱신하는 상태다. 이 모듈보다 오래 살아야 한다.</summary>
    private readonly RegionOwnershipState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 콜백이다.</summary>
    private readonly RegionOwnershipHooks _hooks;

    /// <summary>
    /// outpost·신전 지역 투표와 판본별 도장 요청 순서를 복원한다.
    /// </summary>
    /// <param name="pool">같은 풀의 슬롯을 읽는다.</param>
    /// <param name="additional">추가 목록이다.</param>
    /// <param name="temples">신전 목록이다.</param>
    /// <param name="themes">테마 표다.</param>
    /// <param name="state">처리 중 조회·갱신하는 상태다.</param>
    /// <param name="hooks">외부 저장소 조회와 가상 동작을 연결하는 콜백이다.</param>
    /// <exception cref="ArgumentException">지역 소유 목록/테마/필수 효과 누락이다.</exception>
    public RawRegionOwnership(
        RawSidPool pool,
        RawPostPopList additional,
        RawPostPopList temples,
        IReadOnlyList<uint> themes,
        RegionOwnershipState state,
        RegionOwnershipHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(additional);
        ArgumentNullException.ThrowIfNull(temples);
        ArgumentNullException.ThrowIfNull(themes);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (ReferenceEquals(additional, temples) || hooks.RegionAt is null || hooks.Paint is null
            || (pool.Edition == OriginalEdition.Patch1078 && (hooks.ThemeMode is null || (hooks.TypeTheme is null && themes.Count < 188u))))
        {
            throw new ArgumentException("지역 소유 목록/테마/필수 효과 누락");
        }

        _pool = pool;
        _additional = additional;
        _temples = temples;
        _themes = themes;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>같은 풀의 실제 지역 조회인지 확인한다.</summary>
    /// <returns>이 모듈이 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>실제 TYPE 장치 또는 기존 독립 입력표에서 선택한 원소만 읽는다.</summary>
    /// <param name="type">조회할 타입 번호다.</param>
    /// <returns>해당 타입의 테마 종류값이다.</returns>
    private uint Theme(uint type)
    {
        if (_hooks.TypeTheme is { } theme)
        {
            return theme(type);
        }

        if (type >= (uint)_themes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "지역 테마 타입 오류");
        }

        return _themes[(int)type];
    }

    /// <summary>원본 좌표를 0 방향으로 절삭한다.</summary>
    /// <param name="value">절삭할 값이다.</param>
    /// <returns>절삭한 정수 좌표다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">지역 소유 좌표 절삭 범위 오류다.</exception>
    private static int Truncate(double value)
    {
        double result = Math.Truncate(value);
        if (!double.IsFinite(result) || result < int.MinValue || result > int.MaxValue)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "지역 소유 좌표 절삭 범위 오류");
        }

        return (int)result;
    }

    /// <summary>비정렬 원본 float 비트값을 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>비트 그대로의 float 값이다.</returns>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 낮은 바이트부터 조립하는 반복문이다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }

        return BitConverter.UInt32BitsToSingle(value);
    }

    /// <summary>현재 목록 순서로 지역과 소유자를 읽고 같은 표 배열에 누적한다.</summary>
    /// <param name="list">조회할 목록이다.</param>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="votes">누적할 9칸 투표 배열이다.</param>
    /// <param name="affectedIndex">영향 목록 위치다. 없으면 현재 좌표를 쓴다.</param>
    private void Vote(RawPostPopList list, float x, float y, int[] votes, int? affectedIndex)
    {
        // 동적 count·목록 변경을 반영하고 중복 SID도 독립적인 표로 센다.
        for (int i = 0; i < list.Count; ++i)
        {
            uint number = list.Items()[i];
            if (number > 65535u)
            {
                throw new ArgumentOutOfRangeException(nameof(list), "지역 투표 SID 범위 오류");
            }

            ReadOnlySpan<byte> raw = _pool.Slot((int)number);
            uint region = Need(_hooks.RegionAt, nameof(RegionOwnershipHooks.RegionAt))(Coordinate(raw, 14), Coordinate(raw, 18));
            uint target = affectedIndex.HasValue
                ? _state.Affected[affectedIndex.Value].Region
                : Need(_hooks.RegionAt, nameof(RegionOwnershipHooks.RegionAt))(x, y);
            if (region != target)
            {
                continue;
            }

            uint owner = raw[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
            if (owner >= (uint)votes.Length || votes[owner] == int.MaxValue)
            {
                throw new ArgumentOutOfRangeException(nameof(list), "지역 투표 소유자/표 개수 오류");
            }

            ++votes[owner];
        }
    }

    /// <summary>CD는 한 번 도장하며 패치는 초기 도장·최종 도장·필요한 누적 재투표를 수행한다.</summary>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="mode">원본 모드다.</param>
    public void Update(float x, float y, uint mode = 0)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        bool special = patch && mode == 10u;
        int paintX = Truncate(x);
        int paintY = Truncate(y);
        int[] votes = new int[9];
        if (special)
        {
            // 지역마다 표를 초기화하지 않으며 중복 지역도 다시 누적한다.
            for (int index = 0; index < _state.Affected.Count; ++index)
            {
                Vote(_additional, x, y, votes, index);
                Vote(_temples, x, y, votes, index);
            }

            mode = 0;
        }
        else
        {
            if (patch)
            {
                _state.Affected.Clear();
            }

            Vote(_additional, x, y, votes, null);
            Vote(_temples, x, y, votes, null);
        }

        uint owner = 0;
        int best = 0;
        bool tied = false;
        // 소유자 0도 정상 투표 대상이며 더 큰 최대값이 나오면 앞선 동률을 해제한다.
        for (uint i = 0; i < (uint)votes.Length; ++i)
        {
            if (votes[i] > best)
            {
                best = votes[i];
                owner = i;
                tied = false;
            }
            else if (votes[i] == best)
            {
                tied = true;
            }
        }

        if (!patch)
        {
            Need(_hooks.Paint, nameof(RegionOwnershipHooks.Paint))(new RegionOwnershipPaint(paintX, paintY, tied ? 0u : owner, 0u));
            return;
        }

        uint theme = 0;
        if (tied || owner == 0u)
        {
            Need(_hooks.Paint, nameof(RegionOwnershipHooks.Paint))(new RegionOwnershipPaint(paintX, paintY, 1u, 0u));
            _state.Affected.Clear();
            owner = 0;
        }
        else
        {
            if (mode == 0u)
            {
                // 지역과 무관하게 같은 소유자의 마지막 신전 테마가 남는다.
                for (int i = 0; i < _temples.Count; ++i)
                {
                    uint number = _temples.Items()[i];
                    if (number > 65535u)
                    {
                        throw new ArgumentOutOfRangeException(nameof(owner), "지역 테마 SID 범위 오류");
                    }

                    ReadOnlySpan<byte> raw = _pool.Slot((int)number);
                    if (raw[34] != owner)
                    {
                        continue;
                    }

                    uint kind = Theme(raw[10]);
                    uint enabled = Need(_hooks.ThemeMode, nameof(RegionOwnershipHooks.ThemeMode))();
                    theme = enabled == 0u ? 0u : kind == 0u ? 2u : kind == 1u ? 3u : kind == 2u ? 1u : 0u;
                }
            }

            Need(_hooks.Paint, nameof(RegionOwnershipHooks.Paint))(new RegionOwnershipPaint(paintX, paintY, 0u, 0u));
        }

        Need(_hooks.Paint, nameof(RegionOwnershipHooks.Paint))(new RegionOwnershipPaint(paintX, paintY, owner, theme));
        _state.SetWorldRedraw(unchecked(_state.WorldRedraw + 1u));
        _state.SetMapRevision(1u);
        if (!special && _state.Affected.Count > 1)
        {
            Update(x, y, 10u);
        }
    }

    /// <summary>원본 signed DWORD count가 0이면 배열을 읽지 않고 각 지역 조회 뒤 현재 owner를 표에 더한다.</summary>
    /// <param name="list">조회할 목록이다.</param>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="votes">누적할 9칸 투표 배열이다.</param>
    /// <param name="affectedIndex">영향 목록 위치다. 없으면 현재 좌표를 쓴다.</param>
    private void VoteNative(RawPostPopList list, float x, float y, int[] votes, int? affectedIndex)
    {
        // 원문 각 반복의 현재 count·현재 포인터를 사용한다.
        for (int index = 0; index < unchecked((int)list.Count); ++index)
        {
            Span<uint> storage = list.Storage;
            if (index >= storage.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(list), "지역 Native 투표의 현재 DWORD 물리 부족");
            }

            uint number = storage[checked((int)index)];
            if (number > 65535u)
            {
                throw new ArgumentOutOfRangeException(nameof(list), "지역 Native 투표의 실제 DWORD SID 범위 오류");
            }

            ReadOnlySpan<byte> raw = _pool.Slot((int)number);
            uint region = Need(_hooks.RegionAt, nameof(RegionOwnershipHooks.RegionAt))(Coordinate(raw, 14), Coordinate(raw, 18));
            uint target = affectedIndex.HasValue
                ? _state.Affected[affectedIndex.Value].Region
                : Need(_hooks.RegionAt, nameof(RegionOwnershipHooks.RegionAt))(x, y);
            if (region != target)
            {
                continue;
            }

            uint owner = raw[34];
            if (owner >= (uint)votes.Length)
            {
                throw new ArgumentOutOfRangeException(nameof(list), "지역 Native 투표의 현재 owner 범위 오류");
            }

            votes[owner] = unchecked((int)(unchecked((uint)votes[owner]) + 1u));
        }
    }

    /// <summary>같은 455380의 PATCH 도장·재투표이며 typed Update의 unsigned 전체 목록 선검사와 분리한다.</summary>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="mode">원본 모드다.</param>
    public void UpdateNative(float x, float y, uint mode = 0)
    {
        bool special = mode == 10u;
        int paintX = Truncate(x);
        int paintY = Truncate(y);
        int[] votes = new int[9];
        Action<uint, uint> paint = (owner, theme) => Need(_hooks.Paint, nameof(RegionOwnershipHooks.Paint))(new RegionOwnershipPaint(paintX, paintY, owner, theme));
        if (special)
        {
            // 지역별 표를 초기화하지 않으며 현재 영향 목록을 순회한다.
            for (int index = 0; index < _state.Affected.Count; ++index)
            {
                VoteNative(_additional, x, y, votes, index);
                VoteNative(_temples, x, y, votes, index);
            }

            mode = 0;
        }
        else
        {
            _state.Affected.Clear();
            VoteNative(_additional, x, y, votes, null);
            VoteNative(_temples, x, y, votes, null);
        }

        uint owner = 0;
        int best = 0;
        bool tied = false;
        // 현재 최대와 동률을 원문 순서로 선택한다.
        for (uint index = 0; index < (uint)votes.Length; ++index)
        {
            if (votes[index] > best)
            {
                best = votes[index];
                owner = index;
                tied = false;
            }
            else if (votes[index] == best)
            {
                tied = true;
            }
        }

        uint theme = 0;
        if (tied || owner == 0u)
        {
            paint(1u, 0u);
            _state.Affected.Clear();
            owner = 0;
        }
        else
        {
            if (mode == 0u)
            {
                // 같은 소유자의 마지막 신전 테마가 남는다.
                for (int index = 0; index < unchecked((int)_temples.Count); ++index)
                {
                    Span<uint> storage = _temples.Storage;
                    if (index >= storage.Length)
                    {
                        throw new ArgumentOutOfRangeException(nameof(owner), "지역 Native 테마의 실제 DWORD 물리 부족");
                    }

                    uint number = storage[checked((int)index)];
                    if (number > 65535u)
                    {
                        throw new ArgumentOutOfRangeException(nameof(owner), "지역 Native 테마의 실제 DWORD SID 범위 오류");
                    }

                    ReadOnlySpan<byte> raw = _pool.Slot((int)number);
                    if (raw[34] != owner)
                    {
                        continue;
                    }

                    uint kind = Theme(raw[10]);
                    uint enabled = Need(_hooks.ThemeMode, nameof(RegionOwnershipHooks.ThemeMode))();
                    theme = enabled == 0u ? 0u : kind == 0u ? 2u : kind == 1u ? 3u : kind == 2u ? 1u : 0u;
                }
            }

            paint(0u, 0u);
        }

        paint(owner, theme);
        _state.SetWorldRedraw(unchecked(_state.WorldRedraw + 1u));
        _state.SetMapRevision(1u);
        if (!special && _state.Affected.Count > 1)
        {
            UpdateNative(x, y, 10u);
        }
    }

    /// <summary>double 정밀도로 편향을 더하고 float에 저장한 정수값을 실제 지역 getter에 넘긴다.</summary>
    /// <param name="pool">같은 풀의 실제 지역 조회다. 다른 풀의 몸체와 섞으면 거부한다.</param>
    /// <param name="terrain">같은 풀의 지형 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다. 지역 getter만 실제 구현으로 바꾼다.</param>
    /// <returns>지역 getter가 연결된 훅 묶음이다.</returns>
    /// <exception cref="ArgumentException">지역 소유/지형 SID 풀 불일치.</exception>
    /// <remarks>2026-11-18 0.618에서 R4-2 지역 소유권 투표용으로 추가했다. 근거: cpppj `RawRegionOwnership.cpp` 139행의 `MakeTerrainRegionOwnershipHooks`다.</remarks>
    public static RegionOwnershipHooks MakeTerrainRegionOwnershipHooks(RawSidPool pool, RawCanonPlacementTerrain terrain, RegionOwnershipHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(terrain);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, terrain.Pool))
        {
            throw new ArgumentException("지역 소유/지형 SID 풀 불일치", nameof(pool));
        }

        RegionOwnershipHooks result = new()
        {
            RegionAt = hooks.RegionAt,
            Paint = hooks.Paint,
            ThemeMode = hooks.ThemeMode,
            TypeTheme = hooks.TypeTheme,
        };
        result.RegionAt = (x, y) =>
        {
            // 원문 0x3f7fff58 편향을 double 합산 뒤 float에 저장하고 0 방향으로 절삭한다.
            float sx = (float)(Truncate((double)x + BitConverter.UInt32BitsToSingle(SnapBiasBits)));
            float sy = (float)(Truncate((double)y + BitConverter.UInt32BitsToSingle(SnapBiasBits)));
            return terrain.RegionAt(Truncate(sx), Truncate(sy));
        };
        return result;
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <param name="name">장치 이름이다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">지역 소유의 실제 장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? function, string name) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("지역 소유 실제 장치 누락: " + name);
        }

        return function;
    }
}
