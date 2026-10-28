namespace Netstorm.Core.Original;

/// <summary>
/// TYPE+e0의 현재 속도. 경로 요청 동안 2배가 되고 이후 원래 값의 정수 절삭으로 복원한다.
/// 원본 <c>o/RawDudeNotice.h</c>의 <c>DudeNoticeTypeData</c>이며, Dude 통지 본체를 옮길 때 그 파일로 옮긴다.
/// 근거: cpppj/src/o/RawDudeNotice.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class DudeNoticeTypeData
{
    /// <summary>현재 월드의 mutable 이동 속도. 읽기용 타입 표와 별도 공유한다.</summary>
    public float Speed { get; set; }
}

/// <summary>
/// 사제 panic 이동의 외부 동작 연결(원본 <c>PriestPanicHooks</c>).
/// 현재 신전·HP·지면·영역·난수와 경로 시작만 큰 하위로 연결한다.
/// 도달한 실제 하위가 없으면 명시 오류로 멈춘다.
/// 근거: cpppj/src/o/RawPriestPanic.h (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class PriestPanicHooks
{
    /// <summary>plain 좌표의 현재 섬 신전(원본 46f950). null이면 도달할 때 거부한다.</summary>
    public Func<float, float, int>? Temple { get; set; }

    /// <summary>현재 최대 HP(원본 4adcb0). null이면 도달할 때 거부한다.</summary>
    public Func<int, int>? Maximum { get; set; }

    /// <summary>plain 좌표의 현재 isle TYPE 일치(원본 46f6e0). null이면 도달할 때 거부한다.</summary>
    public Func<float, float, bool>? Land { get; set; }

    /// <summary>거의 올림 뒤 현재 지역(원본 46f530). null이면 도달할 때 거부한다.</summary>
    public Func<float, float, uint>? Region { get; set; }

    /// <summary>두 float 끝점과 53비트 중간 결과의 난수(원본 455930). null이면 도달할 때 거부한다.</summary>
    public Func<float, float, double>? Random { get; set; }

    /// <summary>실제 경로 주소(원본 4894e0, actor·신전·0·0). null이면 도달할 때 거부한다.</summary>
    public Func<int, int, ulong>? Target { get; set; }

    /// <summary>실제 경로 주소(원본 489080, actor·x·y·0·0·0·0·0). null이면 도달할 때 거부한다.</summary>
    public Func<int, float, float, ulong>? Point { get; set; }
}

/// <summary>
/// 사제 491db0의 임시 TYPE 속도·신전 복귀와 같은 영역 임의 이동을 복원한다(원본 <c>RawPriestPanic</c>).
/// 첫 실제 임의 경로 시도 뒤에는 실패해도 반환한다. 같은 mutable TYPE 속도를 경로 생성자가 관측하고 같은 원래 레코드에 절삭값을 복원한다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawPriestPanic.cpp (R0-4 신규 이식, 2026-10-28).
/// </summary>
public sealed class RawPriestPanic
{
    /// <summary>191db0 전체 순서를 실행한다. 생성자에서 풀·속도 표·장치를 빌리며 더 오래 살아야 한다.</summary>
    /// <param name="pool">같은 실제 풀.</param>
    /// <param name="speeds">현재 TYPE+0xe0 속도 표 188개.</param>
    /// <param name="hooks">도달한 실제 하위.</param>
    private readonly RawSidPool _pool;

    /// <summary>같은 mutable TYPE 속도 표.</summary>
    private readonly IList<DudeNoticeTypeData> _speeds;

    /// <summary>도달한 실제 하위.</summary>
    private readonly PriestPanicHooks _hooks;

    /// <summary>
    /// 사제 panic 이동기를 만든다. 풀과 속도 표는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">같은 실제 풀. 10.78 풀만 받는다.</param>
    /// <param name="speeds">현재 TYPE 속도 표 188개.</param>
    /// <param name="hooks">도달한 실제 하위.</param>
    public RawPriestPanic(RawSidPool pool, IList<DudeNoticeTypeData> speeds, PriestPanicHooks? hooks = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(speeds);
        if (speeds.Count != OriginalConstructors.TypeCount)
        {
            throw new ArgumentException($"사제 panic 속도 표가 {speeds.Count}개입니다 (10.78 은 {OriginalConstructors.TypeCount}개)", nameof(speeds));
        }
        _pool = pool;
        _speeds = speeds;
        _hooks = hooks ?? new PriestPanicHooks();
    }

    /// <summary>
    /// 491db0 전체 순서를 실행한다. 첫 실제 임의 경로 시도 뒤에는 실패해도 반환한다.
    /// </summary>
    /// <param name="priest">대상 사제 슬롯 번호.</param>
    public void Run(int priest)
    {
        int temple = Need(_hooks.Temple, "사제 panic 실제 하위 미연결")(ReadFloat(priest, 14), ReadFloat(priest, 18));
        if (temple != 0)
        {
            // 최대 HP 조회 앞의 현재 HP 스냅샷이다
            int hp = unchecked((int)Read(priest, 26));
            if (hp < Need(_hooks.Maximum, "사제 panic 실제 하위 미연결")(priest)
                && Read(temple, 34, 1) == Read(priest, 34, 1))
            {
                uint kind = Read(priest, 10, 1);
                if (kind >= (uint)_speeds.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(priest), kind, "사제 panic 현재 TYPE 범위");
                }
                DudeNoticeTypeData type = _speeds[(int)kind];
                int saved = SpeedInteger(type.Speed);
                type.Speed += type.Speed;
                ulong path = Need(_hooks.Target, "사제 panic 실제 하위 미연결")(priest, temple);
                type.Speed = saved;
                if (path != 0)
                {
                    return;
                }
            }
        }
        bool direct = !Need(_hooks.Land, "사제 panic 실제 하위 미연결")(ReadFloat(priest, 14), ReadFloat(priest, 18));
        // 원본은 영역이 맞는 첫 실제 경로 시도까지 임의 위치를 계속 고른다
        while (true)
        {
            // 두 난수 호출 전의 두 좌표를 함께 보존한다
            float x = ReadFloat(priest, 14);
            float y = ReadFloat(priest, 18);
            float candidateX = (float)(Need(_hooks.Random, "사제 panic 실제 하위 미연결")(-20, 20) + x);
            float candidateY = (float)(Need(_hooks.Random, "사제 panic 실제 하위 미연결")(-20, 20) + y);
            bool valid = direct;
            if (!valid && Need(_hooks.Land, "사제 panic 실제 하위 미연결")(candidateX, candidateY))
            {
                // 후보 영역 효과 뒤의 현재 부모 좌표로 두 번째 영역을 읽는다
                uint candidateRegion = Need(_hooks.Region, "사제 panic 실제 하위 미연결")(candidateX, candidateY);
                uint currentRegion = Need(_hooks.Region, "사제 panic 실제 하위 미연결")(ReadFloat(priest, 14), ReadFloat(priest, 18));
                valid = candidateRegion == currentRegion;
            }
            if (valid)
            {
                Need(_hooks.Point, "사제 panic 실제 하위 미연결")(priest, candidateX, candidateY);
                return;
            }
        }
    }

    /// <summary>선택한 실제 하위 누락을 빈 성공으로 숨기지 않는다.</summary>
    /// <typeparam name="T">장치 델리게이트 형태.</typeparam>
    /// <param name="hook">요구할 장치. null이면 거부한다.</param>
    /// <param name="message">거부할 때의 진단 문구.</param>
    private static T Need<T>(T? hook, string message) where T : class
    {
        if (hook is null)
        {
            throw new InvalidOperationException(message);
        }
        return hook;
    }

    /// <summary>원본 CRT의 정상 signed DWORD 절삭 범위를 확인한다.</summary>
    /// <param name="value">확인할 속도.</param>
    private static int SpeedInteger(float value)
    {
        if (!float.IsFinite(value) || (double)value < -2147483648.0 || (double)value >= 2147483648.0)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "사제 panic 속도 CRT 정상범위");
        }
        return (int)value;
    }

    /// <summary>현재 비정렬 raw 필드를 실제 폭으로 조회한다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="at">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private uint Read(int sid, int at, int width = 4)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint result = 0;
        // 실제 바이트 폭만 조합한다
        for (int i = 0; i < width; i++)
        {
            result |= (uint)raw[at + i] << (8 * i);
        }
        return result;
    }

    /// <summary>현재 비정렬 raw float 필드를 읽는다.</summary>
    /// <param name="sid">대상 슬롯 번호.</param>
    /// <param name="at">읽기 시작 위치.</param>
    private float ReadFloat(int sid, int at) => BitConverter.UInt32BitsToSingle(Read(sid, at));
}
