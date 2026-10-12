using Netstorm.Core.Display;

namespace Netstorm.Core.Simulation;

/// <summary>현재 세션이 소유한 정지 사유. 같은 사유를 여러 프레임 보고해도 한 번만 정지한다.</summary>
[Flags]
public enum SessionPauseReason
{
    /// <summary>이 세션이 소유한 정지 요청이 없다.</summary>
    None = 0,
    /// <summary>사용자의 Pause/Shift-F9 토글.</summary>
    Manual = 1,
    /// <summary>미션 스크립트의 안내/브리핑. 그 위의 지식창·도움말에도 이 소유권이 유지된다.</summary>
    ScriptDialog = 2,
    /// <summary>미션·배치·다리·전투 시험이 모두 꺼진 개발 뷰어의 시간 정지.</summary>
    ViewerIdle = 4,
}

/// <summary>
/// 원본 숫자 게임 시계를 실제 세션의 절대 프레임 시각에 연결하고 정지 요청의 소유권을 관리한다.
/// 같은 단조 밀리초 공급과 OriginalGameClock을 화면 루프와 공유하며, 정지 중에는 명령만 처리한다.
/// </summary>
/// <remarks>
/// 원본 00460cd0/00460df0/00460e30 및 docs/dotnet-clock-kernel-20261012.md를 따른다.
/// 2026-10-12 추가: 화면 제한에만 연결됐던 시계의 세션 연결. 남은 규칙의 24Hz 타이머는 별도 이행 대상이다.
/// </remarks>
public sealed class BattleSessionFrameClock
{
    /// <summary>유효한 세션 정지 사유의 비트 집합. 알 수 없는 비트는 상태 변경 전에 거부한다.</summary>
    private const SessionPauseReason KnownReasons = SessionPauseReason.Manual | SessionPauseReason.ScriptDialog | SessionPauseReason.ViewerIdle;

    /// <summary>게임 DWORD 한 주기의 초. 숫자 함수는 그대로 두고 세션 프레임 축만 감김 이후 이어준다.</summary>
    private const double DwordPeriodSeconds = 4294967.296;

    /// <summary>안정된 소유권 처리 순서. 새 소유자를 모두 등록한 뒤 종료된 소유자를 해제한다.</summary>
    private static readonly SessionPauseReason[] ReasonOrder =
        [SessionPauseReason.Manual, SessionPauseReason.ScriptDialog, SessionPauseReason.ViewerIdle];

    /// <summary>프레임·명령을 실제로 처리할 세션.</summary>
    private readonly BattleSession _session;

    /// <summary>앱 전체가 공유하는 원본 게임 시계. 이 연결기가 소유하지 않은 정지도 보존한다.</summary>
    private readonly OriginalGameClock _clock;

    /// <summary>마지막 관찰한 펼친 단조 밀리초. DWORD 값의 대소로 감김을 역행으로 오인하지 않는다.</summary>
    private long _lastMilliseconds;

    /// <summary>직전 실행 프레임에서 읽은 원본 게임 시각. 정지 중 조회는 이 기준을 덮어쓰지 않는다.</summary>
    private double _previousRawGameSeconds;

    /// <summary>원본 시계와 세션의 시작 축 차이·명시적 합성 앞당기기를 보존하는 오프셋.</summary>
    private double _sessionOffsetSeconds;

    /// <summary>DWORD 감김으로 접힌 게임 시각에 더하는 세션 전용 누계.</summary>
    private double _unwrappedSeconds;

    /// <summary>이 연결기가 마지막으로 세션에 전달한 시각. RunTicks 등의 외부 앞당기기를 알아낸다.</summary>
    private double _publishedFrameSeconds;

    /// <summary>현재 세션이 소유한 정지 사유. 다른 세션/상위 계층의 사유는 여기에 포함하지 않는다.</summary>
    public SessionPauseReason PauseReasons { get; private set; }

    /// <summary>공유 원본 시계의 전체 중첩 깊이. 자기 사유를 닫아도 다른 소유자의 정지는 유지된다.</summary>
    public int PauseDepth => _clock.PauseDepth;

    /// <summary>현재 게임 시계가 실제로 정지했는지. 프레임 제한과 표시 연출도 같은 상태를 읽는다.</summary>
    public bool IsPaused => _clock.IsPaused;

    /// <summary>마지막 화면 밀리초에서 원본 계산으로 읽은 게임 시각. 세션의 합성/감김 보정은 포함하지 않는다.</summary>
    public double RawGameSeconds => _clock.GameSeconds(unchecked((uint)_lastMilliseconds));

    /// <summary>
    /// 기존 세션을 공유 숫자 시계에 연결한다. 원본 절대 시각으로 만든 세션은 오프셋이 0이다.
    /// 상대 시각으로 만든 기존 호출자는 그 시작 축을 유지하며 이후 간격은 원본 시계에서 얻는다.
    /// </summary>
    public BattleSessionFrameClock(BattleSession session, OriginalGameClock clock, long milliseconds)
    {
        ArgumentNullException.ThrowIfNull(session);
        ArgumentNullException.ThrowIfNull(clock);
        if (milliseconds < 0) throw new ArgumentOutOfRangeException(nameof(milliseconds));
        _session = session;
        _clock = clock;
        _lastMilliseconds = milliseconds;
        _previousRawGameSeconds = RawGameSeconds;
        _publishedFrameSeconds = session.FrameSeconds;
        _sessionOffsetSeconds = session.FrameSeconds - _previousRawGameSeconds;
    }

    /// <summary>
    /// 현재 세션의 전체 정지 사유를 동기화한다. 같은 비트는 중복 Pause하지 않는다.
    /// 사유 교체에서는 새 Pause를 먼저 처리해 깊이가 잠깐 0으로 내려가며 재개되는 일을 막는다.
    /// </summary>
    /// <param name="reasons">수동 정지·스크립트 안내·개발 뷰어 정지의 현재 비트 집합.</param>
    /// <param name="milliseconds">현재 화면에서 한 번 고정한 펼친 단조 밀리초.</param>
    public void SetPauseReasons(SessionPauseReason reasons, long milliseconds)
    {
        if ((reasons & ~KnownReasons) != 0) throw new ArgumentOutOfRangeException(nameof(reasons));
        Observe(milliseconds);
        uint now = unchecked((uint)milliseconds);
        // 새로 생긴 소유권마다 한 번만 원본 중첩 Pause를 호출한다.
        foreach (SessionPauseReason reason in ReasonOrder)
        {
            if ((reasons & reason) != 0 && (PauseReasons & reason) == 0)
            {
                _clock.Pause(now);
                PauseReasons |= reason;
            }
        }
        // 다른 소유권이 먼저 추가된 상태에서 종료된 자기 소유권만 해제한다.
        foreach (SessionPauseReason reason in ReasonOrder)
        {
            if ((reasons & reason) == 0 && (PauseReasons & reason) != 0)
            {
                _clock.Resume(now);
                PauseReasons &= ~reason;
            }
        }
    }

    /// <summary>
    /// 실행 중이면 원본 게임 시각을 고정해 세션 프레임을 한 번 진행한다. 정지 중에는 명령만 실행한다.
    /// DWORD 감김과 원본 double의 작은 역행은 호스트 프레임 축에서 처리하며 숫자 시계 상태는 바꾸지 않는다.
    /// </summary>
    /// <returns>아직 남아 있는 규칙 로직에서 이번 프레임에 실행한 틱 수.</returns>
    public int AdvanceFrame(long milliseconds)
    {
        Observe(milliseconds);
        // CLI RunTicks/다리 앞당기기가 이미 쓴 세션 시각을 다음 실시간 프레임에서 되돌리지 않는다.
        if (_session.FrameSeconds != _publishedFrameSeconds)
        {
            _sessionOffsetSeconds += _session.FrameSeconds - _publishedFrameSeconds;
            _publishedFrameSeconds = _session.FrameSeconds;
        }
        if (IsPaused)
        {
            _session.ProcessCommands();
            return 0;
        }
        double raw = RawGameSeconds;
        if (raw - _previousRawGameSeconds < -DwordPeriodSeconds / 2)
            _unwrappedSeconds += DwordPeriodSeconds;
        _previousRawGameSeconds = raw;
        double now = Math.Max(_session.FrameSeconds, raw + _sessionOffsetSeconds + _unwrappedSeconds);
        int ticks = _session.AdvanceTo(now);
        _publishedFrameSeconds = _session.FrameSeconds;
        return ticks;
    }

    /// <summary>미션 전환/종료에서 자기 정지 사유만 해제한다. 다른 소유자의 Pause는 유지하고 반복 해제는 무효다.</summary>
    public void ReleasePauseReasons(long milliseconds) => SetPauseReasons(SessionPauseReason.None, milliseconds);

    /// <summary>시계 역행을 상태 변경 전에 거부한다. 같은 화면 시각의 여러 소유권 변경은 허용한다.</summary>
    private void Observe(long milliseconds)
    {
        if (milliseconds < _lastMilliseconds) throw new ArgumentOutOfRangeException(nameof(milliseconds));
        _lastMilliseconds = milliseconds;
    }
}
