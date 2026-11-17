// 00456460의 내부 표식과 00455b20(mode1)의 화면 포스 필드 생성을 구별한다.
// cpppj/src/o/RawFenceCreate.h·cpp 의 RawFenceMarkerCreate·RawFenceShieldCreate 본체를 이식한다.
// 0.591에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 생성)
namespace Netstorm.Core.Original;

// 펜스 생성 모듈을 담는 네임스페이스다. 0.591에서 추가됐다. (R4-1 펜스 생성 효과)

/// <summary>
/// 같은 실제 월드의 생성/통지/소유자/Pop 소비자다. 선택 시 누락을 거부한다.
/// (원본 FenceCreateHooks, 10.78).
/// </summary>
public sealed class FenceCreateHooks
{
    /// <summary>004af530(type,2)의 실제 새 자산 생성이다. 실패하면 0을 돌려준다. 0.591에서 추가됐다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>표식 QA를 쓴 뒤 004214a0의 현재 표면 통지를 수행한다. 0.591에서 추가됐다.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>생성 자산의 실제 가상+74 소유자 지정이다. 0.591에서 추가됐다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>생성 자산의 실제 가상+90(flags 40) Pop이다. 0.591에서 추가됐다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
}

/// <summary>
/// 내부 fencemark(90)의 단계 2 해시 구간과 끝점 태그를 생성하는 본체다.
/// (원본 RawFenceMarkerCreate, 10.78).
/// </summary>
public sealed class RawFenceMarkerCreate
{
    /// <summary>원본이 x87에서 더한 뒤 절삭하는 float 반올림 상수다. 0.591에서 추가됐다.</summary>
    private const float MarkerRoundBias = 0.5f;

    /// <summary>4칸 버킷 한 축의 상한(63)이다. 0.591에서 추가됐다.</summary>
    private const int CellLimit = 63;

    /// <summary>슬롯 안 소유자 바이트의 위치. 0.591에서 추가됐다.</summary>
    private const int OwnerOffset = 34;

    /// <summary>현재 두 끝점과 생성 표식의 실제 슬롯 풀. 0.591에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>004adea0의 현재 중심을 선택 시 조회한다. 0.591에서 추가됐다.</summary>
    private readonly Func<int, float[]> _center;

    /// <summary>DAT005411c0의 현재 fencemark 타입 번호 상자. 0.591에서 추가됐다.</summary>
    private readonly UIntBox _typeBox;

    /// <summary>실제 생성/표시 하위의 현재 소비자다. 0.591에서 추가됐다.</summary>
    private readonly FenceCreateHooks _hooks;

    /// <summary>
    /// 같은 원문 중심 본문만 빌려 별도 표적 상태 없이 동일 생성을 수행한다.
    /// 참조 대상은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">현재 두 끝점과 생성 표식의 실제 슬롯 풀.</param>
    /// <param name="type">DAT005411c0의 현재 fencemark 타입 번호 상자.</param>
    /// <param name="hooks">실제 생성/표시 하위의 현재 소비자.</param>
    /// <param name="center">같은 풀의 현재 중심 조회 함수.</param>
    public RawFenceMarkerCreate(RawSidPool pool, UIntBox type, FenceCreateHooks hooks, Func<int, float[]> center)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(type);
        ArgumentNullException.ThrowIfNull(hooks);
        ArgumentNullException.ThrowIfNull(center);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("fencemark 생성 판본/중심 미결합", nameof(pool));
        }

        if (hooks.Create is null || hooks.NotifySurface is null || hooks.SetOwner is null || hooks.Pop is null)
        {
            throw new ArgumentException("펜스 생성 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _typeBox = type;
        _hooks = hooks;
        _center = center;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다. 0.591에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>현재 fencemark 타입 번호를 읽는다. 0.591에서 추가됐다.</summary>
    public uint MarkerType => _typeBox.Value;

    /// <summary>
    /// 원본 방향·inclusive 사각형·4칸 구간 변경 뒤의 현재 float 지점을 만든다.
    /// 스냅한 구간은 이동 전 위치에서 구하고 생성 좌표는 한 걸음 이동한 뒤의 float다.
    /// </summary>
    /// <param name="centers">두 끝점 중심 네 좌표.</param>
    public static System.Collections.Generic.List<float[]> Points(float[] centers)
    {
        ArgumentNullException.ThrowIfNull(centers);
        if (centers.Length != 4)
        {
            throw new ArgumentException("펜스 표식 끝점 개수 오류", nameof(centers));
        }

        RequirePoints(centers);
        float[] step = MarkerStep(centers);
        float incomingX = Math.Clamp(centers[2], 1.0f, 255.0f);
        float incomingY = Math.Clamp(centers[3], 1.0f, 255.0f);
        bool valid = centers[0] > 0 && centers[1] > 0 && centers[0] < 256 && centers[1] < 256;
        float left = valid ? Math.Min(centers[0], incomingX) : incomingX;
        float top = valid ? Math.Min(centers[1], incomingY) : incomingY;
        float right = valid ? Math.Max(centers[0], incomingX) : incomingX;
        float bottom = valid ? Math.Max(centers[1], incomingY) : incomingY;
        float[] current = [centers[0], centers[1]];
        int[] previous = [-1, -1];
        // 현재 float/직전 4칸 버킷/생성 위치 순서다.
        var result = new System.Collections.Generic.List<float[]>();
        // 원본은 보드 사각형을 벗어날 때 종료한다. x87 원본은 경계 동일점도 허용한다.
        while (left <= current[0] && current[0] <= right && top <= current[1] && current[1] <= bottom)
        {
            int[] cell = [Math.Min(MarkerInt(current[0] + MarkerRoundBias) / 4, CellLimit), Math.Min(MarkerInt(current[1] + MarkerRoundBias) / 4, CellLimit)];
            current[0] += step[0];
            current[1] += step[1];
            if (!(left <= current[0] && current[0] <= right && top <= current[1] && current[1] <= bottom))
            {
                break;
            }

            if (cell[0] != previous[0] || cell[1] != previous[1])
            {
                previous = cell;
                result.Add([current[0], current[1]]);
            }
        }

        return result;
    }

    /// <summary>
    /// QA/통지/QB/graph/현재 owner/Pop 순서를 보존하며 표식을 만든다.
    /// 부모/대상 좌표의 이후 변경은 이미 정해진 로컬 이동 경로를 바꾸지 않는다.
    /// </summary>
    /// <param name="parent">기준 펜스 번호.</param>
    /// <param name="other">이웃 펜스 번호.</param>
    public void Create(int parent, int other)
    {
        float[] second = _center(other);
        float[] first = _center(parent);
        // 부모/대상 좌표의 이후 변경은 이미 정해진 로컬 이동 경로를 바꾸지 않는다.
        foreach (float[] point in Points([first[0], first[1], second[0], second[1]]))
        {
            int created = Need(_hooks.Create)(_typeBox.Value, 2);
            if (created == 0)
            {
                throw new InvalidOperationException("fencemark 생성의 실제 새 SID 누락");
            }

            Put(_pool.AllocatedBytes(created), 12, (uint)parent, 2);
            Need(_hooks.NotifySurface)(created);
            Put(_pool.AllocatedBytes(created), 26, ((uint)other >> 8) & 255u);
            _pool.AllocatedBytes(created)[30] = (byte)other;
            Need(_hooks.SetOwner)(created, _pool.Slot(parent)[OwnerOffset]);
            Need(_hooks.Pop)(created, point[0], point[1], 0x40);
        }
    }

    /// <summary>원본 정상 좌표의 방향 0/2/4/6을 선택하며 동률은 세로를 우선한다.</summary>
    /// <param name="points">두 끝점 네 좌표.</param>
    private static float[] MarkerStep(float[] points)
    {
        float dx = (float)((double)points[2] - points[0]);
        float dy = (float)((double)points[3] - points[1]);
        if (Math.Abs(dx) <= Math.Abs(dy))
        {
            return [0, dy > 0 ? 1.0f : -1.0f];
        }

        return [dx <= 0 ? -1.0f : 1.0f, 0];
    }

    /// <summary>모든 외부 좌표는 실제 Pop 계약과 같은 유한 입력이어야 한다.</summary>
    /// <param name="points">검사할 네 좌표.</param>
    private static void RequirePoints(float[] points)
    {
        // 원본 배열 밖/비유한 메모리 입력은 명시 거부한다.
        foreach (float coordinate in points)
        {
            if (!float.IsFinite(coordinate))
            {
                throw new ArgumentOutOfRangeException(nameof(points), "펜스 생성의 비유한 좌표");
            }
        }
    }

    /// <summary>원본 지정 폭의 비정렬 필드만 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 폭.</param>
    private static void Put(System.Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 원본 폭만 기록한다.
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>유한 정상 보드 좌표의 원본 CRT signed 절삭이다.</summary>
    /// <param name="value">절삭할 값.</param>
    private static int MarkerInt(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), "펜스 생성 CRT 좌표 범위 오류");
        }

        return (int)value;
    }

    /// <summary>선택한 실제 소비자만 요구하고 앞선 쓰기/생성을 되돌리지 않는다.</summary>
    /// <typeparam name="T">효과 대리자 형식.</typeparam>
    /// <param name="hook">호출할 효과.</param>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("펜스 생성의 실제 하위 미연결");
}

/// <summary>
/// 화면의 포스 필드(170)를 원본 100개 상한과 J/K 구간 첫 프레임으로 생성하는 본체다.
/// (원본 RawFenceShieldCreate, 10.78).
/// </summary>
public sealed class RawFenceShieldCreate
{
    /// <summary>슬롯 안 추가 비트 바이트의 위치. 0.591에서 추가됐다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>생성 직후 조기 종료를 검사하는 비트(0x20)다. 0.591에서 추가됐다.</summary>
    private const uint EarlyExitMask = 0x20u;

    /// <summary>슬롯 안 프레임 DWORD(4바이트)의 위치. 0.591에서 추가됐다.</summary>
    private const int FrameOffset = 36;

    /// <summary>수직 연결선의 방향 코드(J)다. 0.591에서 추가됐다.</summary>
    private const byte VerticalLetter = (byte)'J';

    /// <summary>수평 연결선의 방향 코드(K)다. 0.591에서 추가됐다.</summary>
    private const byte HorizontalLetter = (byte)'K';

    /// <summary>생성 포스 필드의 현재 extra/TYPE/프레임 풀. 0.591에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 TYPE의 J/K 첫 프레임 표. 0.591에서 추가됐다.</summary>
    private readonly System.Collections.Generic.IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>DAT00541300의 현재 fenceShield 타입 번호 상자. 0.591에서 추가됐다.</summary>
    private readonly UIntBox _typeBox;

    /// <summary>실제 생성/소유자/Pop이며 표면 통지는 이 경로에서 호출하지 않는다. 0.591에서 추가됐다.</summary>
    private readonly FenceCreateHooks _hooks;

    /// <summary>
    /// 포스 필드 생성 본체를 만든다. 풀·프레임 표·효과 묶음은 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">생성 포스 필드의 현재 extra/TYPE/프레임 풀.</param>
    /// <param name="frames">현재 TYPE의 J/K 첫 프레임 표.</param>
    /// <param name="type">DAT00541300의 현재 fenceShield 타입 번호 상자.</param>
    /// <param name="hooks">실제 생성/소유자/Pop 소비자.</param>
    public RawFenceShieldCreate(RawSidPool pool, System.Collections.Generic.IReadOnlyList<RiftTypeFrames> frames, UIntBox type, FenceCreateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(type);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || frames.Count != 188)
        {
            throw new ArgumentException("포스 필드 생성 판본/TYPE 불일치", nameof(pool));
        }

        if (hooks.Create is null || hooks.SetOwner is null || hooks.Pop is null)
        {
            throw new ArgumentException("펜스 생성 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _frames = frames;
        _typeBox = type;
        _hooks = hooks;
    }

    /// <summary>실제 단일 풀을 연결 단계에서 확인한다. 0.591에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 원본 mode1의 좌표를 정수로 바꾸지 않고 현재 float를 그대로 가상 Pop에 전달한다.
    /// mode0과 같은 보정이나 정수 절삭 전 float 지점이다.
    /// </summary>
    /// <param name="endpoints">두 끝점 네 좌표.</param>
    public static System.Collections.Generic.List<float[]> Points(float[] endpoints)
    {
        ArgumentNullException.ThrowIfNull(endpoints);
        if (endpoints.Length != 4)
        {
            throw new ArgumentException("펜스 포스 필드 끝점 개수 오류", nameof(endpoints));
        }

        // 원본 배열 밖/비유한 메모리 입력은 명시 거부한다.
        foreach (float coordinate in endpoints)
        {
            if (!float.IsFinite(coordinate))
            {
                throw new ArgumentOutOfRangeException(nameof(endpoints), "펜스 생성의 비유한 좌표");
            }
        }

        // 원본 로컬 float의 현재 끝점/증분이다.
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

        // 원본 100회 상한의 실제 float Pop 지점이다. 끝점은 한 번 포함하고 그 뒤 종료한다.
        var result = new System.Collections.Generic.List<float[]>();
        for (uint remaining = 100; remaining != 0; remaining--)
        {
            result.Add([x, y]);
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
    /// 직접 프레임 쓰기와 생성 직후 bit20의 조기 종료를 유지하며 포스 필드를 만든다.
    /// </summary>
    /// <param name="endpoints">두 끝점 네 좌표.</param>
    /// <param name="firstOwner">첫 끝점의 현재 owner.</param>
    /// <param name="secondOwner">둘째 끝점의 현재 owner.</param>
    /// <param name="firstId">첫 끝점 SID.</param>
    /// <param name="secondId">둘째 끝점 SID.</param>
    public void Create(float[] endpoints, uint firstOwner, uint secondOwner, int firstId, int secondId)
    {
        ArgumentNullException.ThrowIfNull(endpoints);
        if (endpoints.Length != 4)
        {
            throw new ArgumentException("펜스 포스 필드 끝점 개수 오류", nameof(endpoints));
        }

        // 수직 J/수평 K의 실제 TYPE 첫 프레임이다.
        byte letter = endpoints[0] == endpoints[2] ? VerticalLetter : HorizontalLetter;
        // 현재 타입 전역은 각 객체 생성 직전에 다시 읽는다.
        foreach (float[] point in Points(endpoints))
        {
            int created = Need(_hooks.Create)(_typeBox.Value, 2);
            if (created == 0)
            {
                throw new InvalidOperationException("포스 필드 생성의 실제 새 SID 누락");
            }

            ReadOnlySpan<byte> raw = _pool.Slot(created);
            if ((raw[ExtraOffset] & EarlyExitMask) != 0)
            {
                return;
            }

            int type = raw[10];
            if (type < 0 || type >= _frames.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(endpoints), "포스 필드 생성의 현재 TYPE 범위 오류");
            }

            Put(_pool.AllocatedBytes(created), FrameOffset, (uint)_frames[type].Run(letter).First);
            Need(_hooks.SetOwner)(created, firstOwner == secondOwner || secondId < firstId ? firstOwner : secondOwner);
            Need(_hooks.Pop)(created, point[0], point[1], 0x40);
        }
    }

    /// <summary>원본 지정 폭의 비정렬 필드만 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 폭.</param>
    private static void Put(System.Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 원본 폭만 기록한다.
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>선택한 실제 소비자만 요구하고 앞선 쓰기/생성을 되돌리지 않는다.</summary>
    /// <typeparam name="T">효과 대리자 형식.</typeparam>
    /// <param name="hook">호출할 효과.</param>
    private static T Need<T>(T? hook) where T : class =>
        hook ?? throw new InvalidOperationException("펜스 생성의 실제 하위 미연결");
}
