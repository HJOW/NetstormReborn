// PATCH 펜스 사이의 기존 연결선 삭제 00455b20(mode0)을 실제 finder/가상 삭제에 연결한다.
// cpppj/src/o/RawFenceLine.h·cpp 의 RawFenceLine 본체를 이식한다.
// 0.589에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 정리 효과)
namespace Netstorm.Core.Original;

// 펜스 연결선 삭제 모듈을 담는 네임스페이스다. 0.589에서 추가됐다. (R4-1 펜스 정리 효과)

/// <summary>
/// 원본 연결선의 정수 검색점과 현재 fencemark 프레임만 처리하는 펜스 연결선 본체다.
/// (원본 RawFenceLine, 10.78).
/// </summary>
public sealed class RawFenceLine
{
    /// <summary>수직 연결선의 방향 코드(J)다. 0.589에서 추가됐다.</summary>
    private const int VerticalDirection = 0x4a;

    /// <summary>수평 연결선의 방향 코드(K)다. 0.589에서 추가됐다.</summary>
    private const int HorizontalDirection = 0x4b;

    /// <summary>연결선 삭제에 쓰는 가상 flags다. 0.589에서 추가됐다.</summary>
    private const uint DestroyFlags = 0x40u;

    /// <summary>슬롯 안 타입 번호(1바이트)의 위치. 0.589에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 프레임 DWORD(4바이트)의 위치. 0.589에서 추가됐다.</summary>
    private const int FrameOffset = 36;

    /// <summary>현재 mark 기록과 삭제 후 체인을 읽는 실제 풀. 0.589에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>mode0의 flags0 일반 finder가 사용하는 실제 해시. 0.589에서 추가됐다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>현재 finder 발자국 표. 0.589에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>004ad680의 실제 첫 프레임 코드 BYTE 표. 0.589에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>DAT00541300의 현재 연결선 타입 번호 상자. 0.589에서 추가됐다.</summary>
    private readonly UIntBox _markTypeBox;

    /// <summary>원본 가상 삭제 전체 몸체. 선택 위치에서만 호출한다. 0.589에서 추가됐다.</summary>
    private readonly Action<int, uint>? _destroy;

    /// <summary>
    /// 실제 동적 저장소와 삭제 소비자를 빌려 펜스 연결선 본체를 만든다. 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">현재 mark 기록과 삭제 후 체인을 읽는 실제 풀.</param>
    /// <param name="hash">mode0 일반 finder가 사용하는 실제 해시.</param>
    /// <param name="types">현재 finder 발자국 표.</param>
    /// <param name="frames">실제 첫 프레임 코드 BYTE 표.</param>
    /// <param name="markType">DAT00541300의 현재 연결선 타입 번호 상자.</param>
    /// <param name="destroy">원본 가상 삭제 전체 몸체. 비우면 Clear에서 명시 실패한다.</param>
    public RawFenceLine(
        RawSidPool pool,
        RawSquidHash hash,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeFrames> frames,
        UIntBox markType,
        Action<int, uint>? destroy)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(markType);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("펜스 연결선 10.78 풀 계약 오류", nameof(pool));
        }

        if (types.Count != 188 || frames.Count != 188)
        {
            throw new ArgumentException("펜스 연결선 PATCH 표 크기 오류", nameof(types));
        }

        _pool = pool;
        _hash = hash;
        _types = types;
        _frames = frames;
        _markTypeBox = markType;
        _destroy = destroy;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>현재 연결선 타입 번호를 읽는다.</summary>
    public uint MarkType => _markTypeBox.Value;

    /// <summary>
    /// 원문 100회 상한/포함 끝점/CRT 절삭을 보존하는整数 검색점을 만든다. mode0도 동일한 좌표 보정과 100개 검색 상한을 사용한다. 끝점은 한 번 포함한다.
    /// </summary>
    /// <param name="endpoints">두 끝점 네 좌표.</param>
    public static System.Collections.Generic.List<int[]> SearchPoints(float[] endpoints)
    {
        ArgumentNullException.ThrowIfNull(endpoints);
        if (endpoints.Length != 4)
        {
            throw new ArgumentException("펜스 연결선 끝점 개수 오류", nameof(endpoints));
        }

        // 원본 스택에 저장되는 현재 float 지점이다.
        float x = endpoints[0] - 1.0f;
        float y = endpoints[1] - 1.0f;
        float endX = endpoints[2] - 1.0f;
        float endY = endpoints[3] - 1.0f;
        float dx = 0.0f;
        float dy = 0.0f;
        if (x == endX)
        {
            if (endY <= y)
            {
                dy = -1;
                y -= 3;
                if (y < endY)
                {
                    endY = y;
                }
            }
            else
            {
                dy = 1;
                endY -= 3;
                if (endY <= y)
                {
                    y = endY;
                }
            }

            if (x <= endX)
            {
                x = endX;
            }

            if (dy > 0)
            {
                if (endY <= y)
                {
                    y = endY;
                }
            }
            else if (y <= endY)
            {
                y = endY;
            }
        }
        else
        {
            if (endX <= x)
            {
                x += 1;
                dx = -1;
                endX -= 1;
            }
            else
            {
                x -= 1;
                dx = 1;
                endX += 1;
            }

            y -= 1;
            endY -= 1;
            x = dx + dx + x;
            endX -= dx + dx;
            if (dx <= 0)
            {
                if (x <= endX)
                {
                    x = endX;
                }
            }
            else if (endX <= x)
            {
                x = endX;
            }

            if (y <= endY)
            {
                y = endY;
            }
        }

        // 실제 mode0이 원본 finder에 넘긴 정수 좌표 순서다.
        var result = new System.Collections.Generic.List<int[]>();
        // 원본 100회 상한을 유지하고 끝점 처리 뒤 종료한다.
        for (uint remaining = 100; remaining != 0; remaining--)
        {
            result.Add([FenceLineInt(x), FenceLineInt(y)]);
            if (x == endX && y == endY)
            {
                break;
            }

            x += dx;
            y += dy;
        }

        return result;
    }

    /// <summary>
    /// 방향 코드 J/K가 맞는 현재 연결선만 가상+10(flags40)으로 삭제한다. 삭제 효과 뒤에도 동일 finder의 미리 보관한 next와 다음 현재 버킷을 사용한다.
    /// </summary>
    /// <param name="endpoints">두 끝점 네 좌표.</param>
    public void Clear(float[] endpoints)
    {
        ArgumentNullException.ThrowIfNull(endpoints);
        if (endpoints.Length != 4)
        {
            throw new ArgumentException("펜스 연결선 끝점 개수 오류", nameof(endpoints));
        }

        // 실제 첫 코드 J는 수직, K는 수평이다.
        int direction = endpoints[0] == endpoints[2] ? VerticalDirection : HorizontalDirection;
        // 원본 보정 지점을 순서대로 검색한다.
        foreach (int[] point in SearchPoints(endpoints))
        {
            var finder = new RawSquidFinder(_pool, _hash, _types);
            // 표면도 포함하는 실제 일반 탐색이다.
            for (int sid = finder.Begin(new SquidSearchArea(point[0], point[1], point[0], point[1]), 0);
                sid != 0;
                sid = finder.Next())
            {
                ReadOnlySpan<byte> raw = _pool.Slot(sid);
                if (raw[TypeOffset] != MarkType)
                {
                    continue;
                }

                int type = raw[TypeOffset];
                int frame = unchecked((int)ReadUint(raw, FrameOffset));
                IReadOnlyList<RiftFrameCode> codes = _frames[type].Codes;
                if (frame < 0 || frame >= codes.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(endpoints), "펜스 연결선 현재 프레임 범위 오류");
                }

                // MSVC signed char로 부호 확장해 방향을 비교한다.
                if ((sbyte)codes[frame].Side != direction)
                {
                    continue;
                }

                Action<int, uint> destroy = _destroy
                    ?? throw new InvalidOperationException("펜스 연결선 실제 가상 삭제 미연결");
                destroy(sid, DestroyFlags);
            }
        }
    }

    /// <summary>현재 네 바이트를 little endian DWORD로 읽는다.</summary>
    /// <param name="raw">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint ReadUint(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 원본 비정렬 필드의 결합 결과다. 이웃 바이트는 읽지 않는다.
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (8 * index);
        }

        return value;
    }

    /// <summary>유효 CRT 입력에서 signed64 절삭의 낮은 DWORD를 반환한다.</summary>
    /// <param name="value">절삭할 값.</param>
    private static int FenceLineInt(float value)
    {
        double wide = value;
        if (!double.IsFinite(wide) || wide < -9223372036854775808.0 || wide >= 9223372036854775808.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "펜스 연결선 CRT 범위 오류");
        }

        return unchecked((int)unchecked((uint)unchecked((long)wide)));
    }
}
