namespace Netstorm.Core.Simulation;

/// <summary>실제 화면 프레임의 튜토리얼 단계 판정과 안내 잠금을 세션 명령/규칙 틱에서 분리한다.</summary>
public sealed partial class BattleSession
{
    /// <summary>화면이 공유하는 초기/후속 안내 잠금. 연결 전 헤드리스 호출은 기존 합성 틱 경로를 사용한다.</summary>
    private MissionDialogFrameGate? _tutorialScreenFrames;

    /// <summary>보정 안내를 즉시 열고 정지 소유권을 갱신하는 화면 콜백. 단계 안내는 프레임 잠금이 나중에 요청한다.</summary>
    private Action<string>? _tutorialScreenTell;

    /// <summary>이번 화면 프레임에서만 단계 판정에 전달할 F4 복귀/회수 사건. 일반 이벤트 Drain과 별도로 보관한다.</summary>
    private readonly List<SessionEvent> _tutorialScreenEvents = [];

    /// <summary>튜토리얼이 실제 화면 프레임 경로에 연결됐는지. 연결되면 명령/따라잡기 틱의 중복 판정을 생략한다.</summary>
    public bool TutorialUsesScreenFrames => _tutorialScreenFrames != null;

    /// <summary>이번 튜토리얼 화면 프레임에 고정한 원본 게임 시각. 월드의 아직 남은 틱 시각과 독립적이다.</summary>
    public double TutorialFrameSeconds { get; private set; }

    /// <summary>
    /// 화면과 같은 안내 잠금/보정 Tell 처리기를 연결한다. 실제 Update 전에 한 번 호출한다.
    /// 다른 잠금으로 실행 중 교체하는 것은 지원하지 않으며 헤드리스의 기존 명시적 진행은 연결하지 않아도 된다.
    /// </summary>
    public void ConnectTutorialScreenFrames(MissionDialogFrameGate frames, Action<string> tell)
    {
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(tell);
        if (_tutorialScreenFrames != null && !ReferenceEquals(_tutorialScreenFrames, frames))
            throw new InvalidOperationException("튜토리얼 화면 잠금은 실행 중 교체할 수 없습니다.");
        _tutorialScreenFrames = frames;
        _tutorialScreenTell = tell;
        TutorialFrameSeconds = FrameSeconds;
    }

    /// <summary>
    /// 안내/재개 잠금과 실제 정지 소유권 판정 뒤 화면 Update당 한 번 호출한다.
    /// 규칙 틱이 0개이거나 여러 개인 화면에서도 단계 판정은 한 번이며, 차단한 프레임의 사건은 재생하지 않는다.
    /// </summary>
    /// <param name="gameSeconds">화면 시작에 고정한 원본 게임 시각. 24Hz 틱으로 반올림하지 않는다.</param>
    public void AdvanceTutorialScreenFrame(double gameSeconds)
    {
        if (!TutorialUsesScreenFrames) throw new InvalidOperationException("튜토리얼 화면 연결이 필요합니다.");
        if (!double.IsFinite(gameSeconds)) throw new ArgumentOutOfRangeException(nameof(gameSeconds));
        TutorialFrameSeconds = gameSeconds;
        try
        {
            // 원본은 잠금 0이면 정지 중에도 단계 함수를 호출한다. 권한/옵션/임시 타이머 쓰기는 유지하고 단계 증가는 거부한다.
            if (Tutorial != null && RunsTutorial && !_tutorialScreenFrames!.StagePending)
                Tutorial.Update(this, _tutorialScreenEvents);
        }
        finally
        {
            _tutorialScreenEvents.Clear();
        }
    }

    /// <summary>화면 경로의 후속 안내를 예약한다. 동기 보정 Tell이 정지를 잡았으면 같은 판정 안에서도 단계 진행을 거부한다.</summary>
    internal bool ScheduleTutorialStage() => _tutorialScreenFrames == null
        || _tutorialScreenFrames.TryScheduleNextStage(TutorialFrameSeconds, TutorialFrameLocked);

    /// <summary>현재 화면에 필요한 명령 사건만 보관한다. 이전 화면의 성공 사건이나 전체 틱 이벤트를 되풀이하지 않는다.</summary>
    private void CollectTutorialScreenEvent(SessionEvent sessionEvent)
    {
        if (TutorialUsesScreenFrames && sessionEvent.Kind is SessionEventKind.ReturnedHome or SessionEventKind.Salvaged)
            _tutorialScreenEvents.Add(sessionEvent);
    }

    /// <summary>화면 경로가 연결됐을 때만 다음 프레임 결과를 바꾸는 잠금/절대 타이머/남은 명령을 검사합에 포함한다.</summary>
    private void HashTutorialScreenState(Fnv1a hash)
    {
        if (_tutorialScreenFrames == null) return;
        hash.Add(BitConverter.DoubleToInt64Bits(TutorialFrameSeconds));
        hash.Add(BitConverter.DoubleToInt64Bits(Tutorial?.TimerSeconds ?? 0));
        hash.Add(_tutorialScreenFrames.RemainingBriefingFrames);
        hash.Add(_tutorialScreenFrames.InitialBriefingRequested ? 1 : 0);
        hash.Add(BitConverter.DoubleToInt64Bits(_tutorialScreenFrames.NextStageSeconds));
        hash.Add(_tutorialScreenFrames.OwnsPause ? 1 : 0);
        hash.Add(_tutorialScreenFrames.ResumeQuietFrames);
        hash.Add(_tutorialScreenEvents.Count);
        // 전달 순서도 다음 단계 판정에 영향을 주므로 보관한 명령 사건을 순서대로 해시한다.
        foreach (SessionEvent sessionEvent in _tutorialScreenEvents)
        {
            hash.Add((int)sessionEvent.Kind);
            hash.Add(sessionEvent.Player);
            hash.Add(sessionEvent.EntityId);
        }
    }
}
