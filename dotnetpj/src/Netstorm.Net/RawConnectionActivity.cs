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

/// <summary>연결 활성 0x64 Timer 수신의 Native 대여 판정이다.
/// 근거: cpppj `src/o/RawConnectionActivity.cpp`의 `RegisterConnectionActivity` 앞부분(실제 Native F가 있으면 같은 값만 대여한다).
/// 분배기 호출(`RegisterCanonical`·`Observe`·`Version`·`Register`)은 `RawPacketDispatch` 이식과 함께 3차 로컬 구현 때 잇는다.
/// 2026-11-11 0.366에서 cpppj HEAD(`eeae45c69`)의 대여 분기를 반영했다.</summary>
public static class ConnectionActivityRegistration
{
    /// <summary>연결 활성 Timer 패킷 타입이다.</summary>
    public const uint ActivityType = 0x64u;

    /// <summary>Timer 수신 몸체 004b6180의 원본 주소 식별자다.</summary>
    public const uint NativeTimerEntry = 0x004b6180u;

    /// <summary>대여가 요구하는 등록 버전이다.</summary>
    public const byte ExpectedVersion = 1;

    /// <summary>대여가 요구하는 불일치 모드다.</summary>
    public const byte ExpectedMismatch = 0;

    /// <summary>대여가 요구하는 선언 길이다. 기존 PE 길이 표의 현재값이다.</summary>
    public const ushort ExpectedLength = 5;

    /// <summary>같은 metadata의 Native 수신을 대여할지 판정한다. 대여할 수 없으면 일반 등록 경로(3차)로 돌려보낸다.</summary>
    /// <param name="table">읽을 패킷 등록 표다.</param>
    /// <param name="borrowed">대여한 표준 수신 한 쌍이다. 대여할 수 없으면 null이다.</param>
    /// <returns>같은 값을 대여하면 참이다.</returns>
    /// <exception cref="InvalidOperationException">Native 수신인데 버전·불일치·길이·관찰기 metadata가 다르면 멈춘다.</exception>
    public static bool TryBorrowTimerReceiver(PacketRegistrationTable table, out CanonicalPacketReceiver? borrowed)
    {
        ArgumentNullException.ThrowIfNull(table);
        PacketRegistration row = table.Registration(ActivityType);
        CanonicalPacketReceiver? receiver = row.CanonicalReceive;
        if (receiver is null || receiver.NativeEntry != NativeTimerEntry)
        {
            borrowed = null;
            return false;
        }

        if (row.Version != ExpectedVersion || row.Mismatch != ExpectedMismatch || row.Length != ExpectedLength
            || row.Observer is null || row.PacketObserver is not null)
        {
            throw new InvalidOperationException("Native 연결 수신의 같은 metadata 불일치");
        }

        borrowed = receiver;
        return true;
    }
}
