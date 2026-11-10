// 로컬 연결의 활성 시각·프레임 표와 0x64 수신 몸체의 사전 구현을 공유한다.
// 근거: cpppj `src/o/RawConnectionActivity.h`·`RawConnectionActivity.cpp`(004b6180).
// 분배기 등록(`RegisterConnectionActivity`)은 `RawPacketDispatch` 이식과 함께 3차 로컬 구현 때 잇는다.
// 2026-11-11 멀티플레이 사전 구현에서 추가했다. 지시대로 검사는 건너뛴다.

namespace Netstorm.Net;

// 로컬 연결 활성 범주를 담는 네임스페이스다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.

// 22칸 활성 표의 같은 저장소 뷰이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionActivityTables
{
    /// <summary>원본 22칸 개수다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public const int EntryCount = 22;

    /// <summary>005c84e8의 프레임 표 22 DWORD다. 0은 그대로 둔다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint[] Frames { get; }

    /// <summary>005c8548의 wall 표 22 DWORD다. signed64 ftol 하위 DWORD이며 CRT clock이 아니다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public uint[] WallSeconds { get; }

    /// <summary>같은 저장소의 두 22칸 표를 빌린다. 세션·호스트와 같은 수명으로 둔다.</summary>
    /// <param name="frames">프레임 표 22칸이다.</param>
    /// <param name="wallSeconds">wall 표 22칸이다. 프레임 표와 다른 배열이어야 한다.</param>
    /// <exception cref="ArgumentException">표 크기·별개 저장소 오류다.</exception>
    public ConnectionActivityTables(uint[] frames, uint[] wallSeconds)
    {
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(wallSeconds);
        if (frames.Length != EntryCount || wallSeconds.Length != EntryCount || ReferenceEquals(frames, wallSeconds))
        {
            throw new ArgumentException("연결 활성의 같은 22칸 표·별개 저장소 필요");
        }

        Frames = frames;
        WallSeconds = wallSeconds;
    }
}

// 활성 수신이 읽는 외부 효과 묶음이다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class ConnectionActivityHooks
{
    /// <summary>00460d70의 wall 시각(초)이다. 분배기 앞단의 now를 바꾸지 않는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<double>? WallSeconds { get; set; }

    /// <summary>0055b4a8의 현재 프레임 DWORD다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<uint>? FrameNumber { get; set; }

    /// <summary>wall 표 선택용 00540bc0 서버 여부다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Func<bool>? Server { get; set; }

    /// <summary>004d3090 (2, sender, 0, 0) 또는 (1, 0, 0, 0) 지우기 어댑터다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    public Action<uint, uint>? EraseStatus { get; set; }
}

// packet payload·연결을 보지 않는 004b6180 Timer 수신 몸체다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
public sealed class RawConnectionActivity
{
    /// <summary>같은 22칸 활성 표다. 호출자가 소유한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly ConnectionActivityTables _tables;

    /// <summary>wall·프레임·서버·지우기 효과다. 설치 때 모두 묶는다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    private readonly ConnectionActivityHooks _hooks;

    /// <summary>같은 표·같은 효과로 활성 수신 몸체를 만든다. 분배기 등록은 3차 로컬 구현 때 잇는다.</summary>
    /// <param name="tables">빌릴 22칸 표다.</param>
    /// <param name="hooks">묶을 외부 효과다. 모두 필수다.</param>
    /// <exception cref="ArgumentException">같은 표·별개 저장소·필수 효과 누락이다.</exception>
    public RawConnectionActivity(ConnectionActivityTables tables, ConnectionActivityHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(tables);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.WallSeconds is null || hooks.FrameNumber is null || hooks.Server is null || hooks.EraseStatus is null)
        {
            throw new ArgumentException("연결 활성의 실제 외부 효과 미연결");
        }

        _tables = tables;
        _hooks = hooks;
    }

    /// <summary>전달보다 wall·frame 캡처를 먼저 쓰는 순서로 한 칸을 적는다. sender 범위 밖은 거부한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="sender">적을 Native 인덱스다. 0..21 범위를 받는다.</param>
    /// <exception cref="ArgumentOutOfRangeException">Native 인덱스 0..21 범위 오류다.</exception>
    /// <exception cref="InvalidOperationException">도달한 실제 외부 효과 미연결이다.</exception>
    public void Receive(int sender)
    {
        if (sender < 0 || sender >= ConnectionActivityTables.EntryCount)
        {
            throw new ArgumentOutOfRangeException(nameof(sender), sender, "연결 활성의 실제 Native 인덱스 0..21 범위 오류");
        }

        // 004b6188/6196/619b의 wall·frame 읽기를 먼저 확정한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.
        uint wall = WallLowDword(Need(_hooks.WallSeconds)());
        uint frame = Need(_hooks.FrameNumber)();
        _tables.WallSeconds[sender] = wall;
        bool server = Need(_hooks.Server)();
        _tables.Frames[sender] = frame;
        Need(_hooks.EraseStatus)(server ? 2u : 1u, server ? unchecked((uint)sender) : 0u);
    }

    /// <summary>masked x87처럼 범위 밖 double은 0으로 두고 signed64 하위 DWORD만 취한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <param name="value">바꿀 wall 시각이다.</param>
    private static uint WallLowDword(double value)
    {
        if (!double.IsFinite(value) || value < -9.223372036854776E18 || value >= 9.223372036854776E18)
        {
            return 0u;
        }

        return unchecked((uint)(long)value);
    }

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-11 멀티플레이 사전 구현에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("연결 활성의 선택된 실제 효과 미연결");
        }

        return callback;
    }
}
