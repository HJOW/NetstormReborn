// PATCH 썬 바리케이드의 방향별 최근접 검색과 실제 연결 개수를 같은 공간 해시에 연결한다.
// cpppj/src/o/RawFenceNeighbors.h·cpp 의 RawFenceNeighbors 본체를 이식한다.
// 0.588에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 이웃)
namespace Netstorm.Core.Original;

// 펜스 이웃 검색 모듈을 담는 네임스페이스다. 0.588에서 추가됐다. (R4-1 펜스 이웃)

/// <summary>
/// 이웃 검색의 TYPE 사거리와 방향별 9×9 동맹을 현재 원본 저장소에서 빌리는 펜스 이웃 본체다.
/// (원본 RawFenceNeighbors, 10.78).
/// </summary>
public sealed class RawFenceNeighbors
{
    /// <summary>원본 여덟 방향의 x 성분이다. 이웃 검색은 짝수 방향만 사용한다. 0.588에서 추가됐다.</summary>
    private static readonly int[] FenceDx = [0, 1, 1, 1, 0, -1, -1, -1];

    /// <summary>원본 여덟 방향의 y 성분이다. 이웃 검색은 짝수 방향만 사용한다. 0.588에서 추가됐다.</summary>
    private static readonly int[] FenceDy = [-1, -1, 0, 1, 1, 1, 0, -1];

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치. 0.588에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 상태 바이트의 위치. 0.588에서 추가됐다.</summary>
    private const int StateOffset = 11;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치. 0.588에서 추가됐다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치. 0.588에서 추가됐다.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치. 0.588에서 추가됐다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>삭제된 슬롯의 dead 비트다. 후보에서 제외한다. 0.588에서 추가됐다.</summary>
    private const byte DeadBit = 2;

    /// <summary>차단된 슬롯 마스크(abstract·매립·contained)다. 후보에서 제외한다. 0.588에서 추가됐다.</summary>
    private const uint BlockedMask = 9u;

    /// <summary>연결 개수에서 제외하는 표식 비트다. 0.588에서 추가됐다.</summary>
    private const uint MarkerSkipMask = 1u;

    /// <summary>동맹 소유자 범위의 상한(9 미만)이다. 0.588에서 추가됐다.</summary>
    private const uint OwnerLimit = 9u;

    /// <summary>삭제 대상 슬롯을 읽는 풀. 0.588에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>후보 순서를 정하는 같은 4단계 해시. 0.588에서 추가됐다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>현재 발자국/타입 번호 표. 0.588에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>실제 TYPE+44를 타입 번호로 조회하는 독립 사거리 표다. getter 방식에서는 비운다. 0.588에서 추가됐다.</summary>
    private readonly IReadOnlyList<float>? _ranges;

    /// <summary>후보 소유자 행에서 원본 소유자 열을 읽는 독립 81 DWORD 표다. getter 방식에서는 비운다. 0.588에서 추가됐다.</summary>
    private readonly IReadOnlyList<uint>? _alliances;

    /// <summary>장기 본체의 현재 TYPE+44 읽기 함수다. 독립 표 방식에서는 비운다. 0.588에서 추가됐다.</summary>
    private readonly Func<uint, float>? _rangeRead;

    /// <summary>현재 Player의 실제 81 DWORD에서 선택한 행/열만 읽는 함수다. 독립 표 방식에서는 비운다. 0.588에서 추가됐다.</summary>
    private readonly Func<uint, uint, uint>? _allianceRead;

    /// <summary>
    /// 독립 사거리·동맹 표로 펜스 이웃 본체를 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">후보 슬롯을 읽는 단일 raw 풀.</param>
    /// <param name="hash">후보 순서를 정하는 공간 해시.</param>
    /// <param name="types">현재 발자국/타입 번호 표.</param>
    /// <param name="ranges">실제 TYPE+44 188개 표.</param>
    /// <param name="alliances">후보 소유자 행에서 원본 소유자 열을 읽는 81 DWORD 표.</param>
    public RawFenceNeighbors(
        RawSidPool pool,
        RawSquidHash hash,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<float> ranges,
        IReadOnlyList<uint> alliances)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(ranges);
        ArgumentNullException.ThrowIfNull(alliances);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("펜스 이웃 10.78 풀 계약 오류", nameof(pool));
        }

        if (types.Count != 188 || ranges.Count != 188 || alliances.Count != 81)
        {
            throw new ArgumentException("펜스 이웃의 PATCH 타입/동맹 크기 불일치", nameof(types));
        }

        _pool = pool;
        _hash = hash;
        _types = types;
        _ranges = ranges;
        _alliances = alliances;
    }

    /// <summary>
    /// 현재 TYPE/Player getter로 펜스 이웃 본체를 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">후보 슬롯을 읽는 단일 raw 풀.</param>
    /// <param name="hash">후보 순서를 정하는 공간 해시.</param>
    /// <param name="types">현재 발자국/타입 번호 표.</param>
    /// <param name="range">현재 TYPE+44 읽기 함수.</param>
    /// <param name="alliance">현재 Player 실제 81 DWORD 읽기 함수.</param>
    public RawFenceNeighbors(
        RawSidPool pool,
        RawSquidHash hash,
        IReadOnlyList<OriginalTypeRecord> types,
        Func<uint, float> range,
        Func<uint, uint, uint> alliance)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(range);
        ArgumentNullException.ThrowIfNull(alliance);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("펜스 이웃 10.78 풀 계약 오류", nameof(pool));
        }

        if (types.Count != 188)
        {
            throw new ArgumentException("펜스 현재 TYPE 크기 불일치", nameof(types));
        }

        _pool = pool;
        _hash = hash;
        _types = types;
        _rangeRead = range;
        _allianceRead = alliance;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>원본의 예약 슬롯도 범위 안에서만 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private ReadOnlySpan<byte> Slot(int sid)
    {
        // free를 임의 필터링하지 않고 원본처럼 SID1부터 유효한 현재 슬롯을 허용한다.
        if (sid <= 0 || sid >= _pool.Capacity)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "펜스 이웃 SID 범위 오류");
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "펜스 이웃 TYPE 범위 오류");
        }

        return raw;
    }

    /// <summary>실제 Pop 좌표를 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private float[] Position(int sid)
    {
        // 캡처 위치와 현재 타입 중심은 원본의 서로 다른 읽기 위치를 유지한다.
        ReadOnlySpan<byte> raw = Slot(sid);
        return [ReadSingle(raw, XOffset), ReadSingle(raw, YOffset)];
    }

    /// <summary>현재 float 중심을 읽는다. 표식 생성/정리도 같은 본문으로 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    public float[] Center(int sid)
    {
        // 004adea0은 각 성분을 float에 저장한 뒤 거리 helper에 전달한다.
        float[] point = Position(sid);
        OriginalTypeRecord type = _types[Slot(sid)[TypeOffset]];
        return [
            (float)((double)point[0] - type.FootX * 0.5),
            (float)((double)point[1] - type.FootY * 0.5)];
    }

    /// <summary>현재 TYPE 사거리를 매번 읽는다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private float CurrentRange(int sid)
    {
        uint number = Slot(sid)[TypeOffset];
        if (_rangeRead is not null)
        {
            return _rangeRead(number);
        }

        return _ranges![(int)number];
    }

    /// <summary>TYPE+44의 현재 float를 원본 CRT 절삭한다.</summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private int Range(int sid) => FenceTruncate(CurrentRange(sid));

    /// <summary>원본 Rect의 현재 float 경계를 만든다.</summary>
    /// <param name="sid">기준 슬롯 번호.</param>
    /// <param name="direction">원본 여덟 방향 중 하나.</param>
    public float[] SearchBounds(int sid, uint direction)
    {
        if (direction >= 8)
        {
            throw new ArgumentOutOfRangeException(nameof(direction), "펜스 방향 범위 오류");
        }

        // 현재 검색을 만드는 원본 입력이다.
        float[] point = Position(sid);
        OriginalTypeRecord type = _types[Slot(sid)[TypeOffset]];
        int range = Range(sid);
        // signed 곱셈의 낮은 DWORD를 보존한다.
        int dx = unchecked((int)(unchecked((uint)range) * unchecked((uint)FenceDx[direction])));
        int dy = unchecked((int)(unchecked((uint)range) * unchecked((uint)FenceDy[direction])));
        // 원본의 동일 두 끝점으로 시작한다.
        float[] bounds = [point[0], point[1], point[0], point[1]];
        // float Rect 구성은 타입 발자국을 뺀 지점에 원본 정수 사거리의 방향 벡터를 더한다.
        float[] next = [
            (float)((double)point[0] - (float)(type.FootX - 1) + (float)dx),
            (float)((double)point[1] - (float)(type.FootY - 1) + (float)dy)];
        FenceInclude(bounds, next);
        return bounds;
    }

    /// <summary>finder가 반환한 순서대로 엄격한 최근접 한 개를 선택한다.</summary>
    /// <param name="sid">기준 슬롯 번호.</param>
    /// <param name="candidates">finder 순서의 후보 목록.</param>
    /// <param name="allied">참이면 동맹만 남긴다.</param>
    public int Select(int sid, IReadOnlyList<int> candidates, bool allied)
    {
        ArgumentNullException.ThrowIfNull(candidates);
        // 정렬 축 비교는 원본 진입 시 Pop 좌표를 사용한다.
        float[] position = Position(sid);
        int result = 0;
        // 원본 최대 거리는 정수 사거리의 두 배다.
        float best = (float)unchecked((int)(unchecked((uint)Range(sid)) * 2u));
        // finder가 준 순서와 중복을 그대로 소비한다.
        for (int index = 0; index < candidates.Count; index++)
        {
            int candidate = candidates[index];
            ReadOnlySpan<byte> raw = Slot(candidate);
            ReadOnlySpan<byte> source = Slot(sid);
            float[] point = Position(candidate);
            if (raw[TypeOffset] != source[TypeOffset] || candidate == sid || (raw[StateOffset] & DeadBit) != 0
                || (raw[ExtraOffset] & BlockedMask) != 0 || (point[0] != position[0] && point[1] != position[1]))
            {
                continue;
            }

            // 실제 float 중심에서 x87 거리 계산으로 확장한다.
            float[] a = Center(sid);
            float[] b = Center(candidate);
            double dx = (double)b[0] - a[0];
            double dy = (double)b[1] - a[1];
            double distance = Math.Sqrt(dx * dx + dy * dy);
            if (best <= distance)
            {
                continue;
            }

            best = (float)distance;
            result = candidate;
        }

        if (result != 0 && allied)
        {
            // 최근접 선택 뒤 현재 원본 소유자를 읽는다.
            uint owner = Slot(sid)[34];
            uint other = Slot(result)[34];
            if (owner >= OwnerLimit || other >= OwnerLimit)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "펜스 동맹 소유자 범위 오류");
            }

            uint alliedValue = _allianceRead is not null
                ? _allianceRead(other, owner)
                : _alliances![(int)(other * 9 + owner)];
            if (owner != other && alliedValue == 0)
            {
                return 0;
            }
        }

        return result;
    }

    /// <summary>같은 실제 finder로 방향 이웃을 실행한다.</summary>
    /// <param name="sid">기준 슬롯 번호.</param>
    /// <param name="direction">원본 여덟 방향 중 하나.</param>
    /// <param name="allied">참이면 동맹만 남긴다.</param>
    public int Neighbor(int sid, uint direction, bool allied = false)
    {
        float[] bounds = SearchBounds(sid, direction);
        // 표면을 건너뛰고 현재 해시의 순서대로 수집한다.
        var finder = new RawSquidFinder(_pool, _hash, _types);
        var candidates = new System.Collections.Generic.List<int>();
        // 읽기 전용 탐색 중 같은 후보 순서만 보존한다.
        for (int candidate = finder.Begin(new SquidSearchArea(
            FenceTruncate(bounds[0]), FenceTruncate(bounds[1]), FenceTruncate(bounds[2]), FenceTruncate(bounds[3])), 1);
            candidate != 0;
            candidate = finder.Next())
        {
            candidates.Add(candidate);
        }

        return Select(sid, candidates, allied);
    }

    /// <summary>축 거리 절삭과 사거리 포함 판정을 한다.</summary>
    /// <param name="sid">기준 슬롯 번호.</param>
    /// <param name="endpoints">두 끝점 네 좌표.</param>
    public bool WithinRange(int sid, float[] endpoints)
    {
        ArgumentNullException.ThrowIfNull(endpoints);
        if (endpoints.Length != 4)
        {
            throw new ArgumentException("펜스 끝점 개수 오류", nameof(endpoints));
        }

        // 수직이면 Y차, 그 외에는 X차의 절댓값을 절삭한다.
        double distance = endpoints[0] == endpoints[2]
            ? Math.Abs((double)endpoints[1] - endpoints[3])
            : Math.Abs((double)endpoints[0] - endpoints[2]);
        return (float)FenceTruncate(distance) <= CurrentRange(sid);
    }

    /// <summary>0/2/4/6 동맹 이웃 연결 개수를 센다.</summary>
    /// <param name="sid">기준 슬롯 번호.</param>
    public uint Connections(int sid)
    {
        if ((Slot(sid)[ExtraOffset] & BlockedMask) != 0)
        {
            return 0;
        }

        // 실제 원문 네 방향의 누계다.
        float[] position = Position(sid);
        uint result = 0;
        // 원본은 방향 0/2/4/6을 순서대로 검사한다.
        for (uint direction = 0; direction < 8; direction += 2)
        {
            int neighbor = Neighbor(sid, direction, true);
            if (neighbor == 0)
            {
                continue;
            }

            float[] point = Position(neighbor);
            if (WithinRange(sid, [position[0], position[1], point[0], point[1]])
                && (Slot(neighbor)[ExtraOffset] & MarkerSkipMask) == 0)
            {
                result++;
            }
        }

        return result;
    }

    /// <summary>비정렬 float 좌표의 비트를 그대로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static float ReadSingle(ReadOnlySpan<byte> raw, int offset) =>
        BitConverter.UInt32BitsToSingle(ReadUint(raw, offset));

    /// <summary>비정렬 raw DWORD를 낮은 바이트부터 조립한다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 지정한 폭만 읽는다.
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>원본 BoardCoord의 정상 보드 내부 조건이다.</summary>
    /// <param name="point">검사할 좌표.</param>
    private static bool FenceValid(float[] point) =>
        point[0] > 0 && point[0] < 256 && point[1] > 0 && point[1] < 256;

    /// <summary>현재 Rect의 양 끝을 별도로 검사하고 새 점만 1..255에 제한한다.</summary>
    /// <param name="rect">갱신할 사각형.</param>
    /// <param name="point">포함할 점.</param>
    private static void FenceInclude(float[] rect, float[] point)
    {
        float[] clamped = [Math.Clamp(point[0], 1.0f, 255.0f), Math.Clamp(point[1], 1.0f, 255.0f)];
        if (FenceValid([rect[0], rect[1]]))
        {
            rect[0] = Math.Min(rect[0], clamped[0]);
            rect[1] = Math.Min(rect[1], clamped[1]);
        }
        else
        {
            rect[0] = clamped[0];
            rect[1] = clamped[1];
        }

        if (FenceValid([rect[2], rect[3]]))
        {
            rect[2] = Math.Max(rect[2], clamped[0]);
            rect[3] = Math.Max(rect[3], clamped[1]);
        }
        else
        {
            rect[2] = clamped[0];
            rect[3] = clamped[1];
        }
    }

    /// <summary>원본 유효 CRT 입력에서 signed64 절삭의 낮은 32비트를 보존한다.</summary>
    /// <param name="value">절삭할 값.</param>
    private static int FenceTruncate(double value)
    {
        if (!double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "펜스 CRT 절삭 범위 오류");
        }

        return unchecked((int)unchecked((uint)unchecked((long)value)));
    }
}
