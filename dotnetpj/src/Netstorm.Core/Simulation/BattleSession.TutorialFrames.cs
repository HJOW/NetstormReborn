namespace Netstorm.Core.Simulation;

/// <summary>실제 화면 프레임의 튜토리얼 단계 판정과 안내 잠금을 세션 명령/규칙 틱에서 분리한다.</summary>
public sealed partial class BattleSession
{
    /// <summary>화면이 공유하는 공통 프레임 게이트(원본 004c34c0). 연결 전 헤드리스 호출은 기존 합성 틱 경로를 사용한다.</summary>
    private MissionFrameGate? _tutorialGate;

    /// <summary>보정 안내를 즉시 열고 정지 소유권을 갱신하는 화면 콜백. 단계 안내는 프레임 잠금이 나중에 요청한다.</summary>
    private Action<string>? _tutorialScreenTell;

    /// <summary>이번 화면 프레임에서만 단계 판정에 전달할 F4 복귀/회수 사건. 일반 이벤트 Drain과 별도로 보관한다.</summary>
    private readonly List<SessionEvent> _tutorialScreenEvents = [];

    /// <summary>튜토리얼이 실제 화면 프레임 경로에 연결됐는지. 연결되면 명령/따라잡기 틱의 중복 판정을 생략한다.</summary>
    public bool TutorialUsesScreenFrames => _tutorialGate != null;

    /// <summary>이번 튜토리얼 화면 프레임에 고정한 원본 게임 시각. 월드의 아직 남은 틱 시각과 독립적이다.</summary>
    public double TutorialFrameSeconds { get; private set; }

    /// <summary>
    /// 화면의 공통 프레임 게이트와 보정 Tell 처리기를 연결한다. 실제 Update 전에 한 번 호출한다.
    /// 튜토리얼 단계 글자·임시 타이머는 이후 게이트 상태가 하나로 소유한다.
    /// 다른 게이트로 실행 중 교체하는 것은 지원하지 않으며 헤드리스의 기존 명시적 진행은 연결하지 않아도 된다.
    /// </summary>
    /// <param name="gate">화면이 프레임마다 <see cref="MissionFrameGate.RunFrame"/>을 부르는 게이트.</param>
    /// <param name="tell">보정 안내 섹션을 즉시 열고 정지를 잡는 콜백.</param>
    /// <remarks>2026-10-15 감사 D02: 안내 잠금·재개·튜토리얼 분배를 원본 공통 프레임 하나로 통합.</remarks>
    public void ConnectTutorialScreenFrames(MissionFrameGate gate, Action<string> tell)
    {
        ArgumentNullException.ThrowIfNull(gate);
        ArgumentNullException.ThrowIfNull(tell);
        if (_tutorialGate != null && !ReferenceEquals(_tutorialGate, gate))
            throw new InvalidOperationException("튜토리얼 화면 잠금은 실행 중 교체할 수 없습니다.");
        _tutorialGate = gate;
        _tutorialScreenTell = tell;
        // 튜토리얼이 없는 미션은 1~6 분배를 하지 않도록 번호를 0으로 둔다.
        if (Tutorial != null) Tutorial.AttachGateState(gate.State);
        else gate.State.TutorialNumber = 0;
        TutorialFrameSeconds = FrameSeconds;
    }

    /// <summary>
    /// 공통 프레임의 튜토리얼 분배(안내 잠금 0·데모 아님·번호 1~6)에서 한 번 호출해 현재 단계를 판정한다.
    /// 규칙 틱이 0개이거나 여러 개인 화면에서도 단계 판정은 한 번이다.
    /// </summary>
    /// <param name="gameSeconds">화면 시작에 고정한 원본 게임 시각. 24Hz 틱으로 반올림하지 않는다.</param>
    /// <remarks>
    /// 원본은 잠금 0이면 정지 중에도 단계 함수를 호출한다. 권한/옵션/임시 타이머 쓰기는 유지하고 단계 증가는 게이트의 Advance가 거부한다.
    /// 사건 목록은 호출자가 <see cref="EndTutorialScreenFrame"/>으로 프레임마다 비운다.
    /// </remarks>
    public void AdvanceTutorialScreenFrame(double gameSeconds)
    {
        if (!TutorialUsesScreenFrames) throw new InvalidOperationException("튜토리얼 화면 연결이 필요합니다.");
        if (!double.IsFinite(gameSeconds)) throw new ArgumentOutOfRangeException(nameof(gameSeconds));
        TutorialFrameSeconds = gameSeconds;
        if (Tutorial != null && RunsTutorial) Tutorial.Update(this, _tutorialScreenEvents);
    }

    /// <summary>
    /// 화면 프레임 하나가 끝날 때 호출한다. 이번 화면에 보관한 F4 복귀/회수 사건을 비운다.
    /// 안내 잠금 등으로 분배가 없던 프레임의 사건도 다음 프레임에 재생하지 않는다(원본은 현재 사건 값만 전달한다).
    /// </summary>
    public void EndTutorialScreenFrame() => _tutorialScreenEvents.Clear();

    /// <summary>
    /// 튜토리얼 단계를 한 글자 올릴 수 있는지 확인한다. 화면 경로는 게이트의 004c33f0 구현이 현재 정지/잠금을 읽어
    /// 글자 증가·10프레임 잠금·안내 기한·타이머 0까지 한 번에 처리하고, 헤드리스 경로는 호출자가 정한 잠금만 본다.
    /// </summary>
    /// <returns>올렸거나 올려도 되면 true. 정지/잠금으로 거부됐으면 false.</returns>
    internal bool TryAdvanceTutorialStage() => _tutorialGate != null ? _tutorialGate.Advance() : !TutorialFrameLocked;

    /// <summary>현재 화면에 필요한 명령 사건만 보관한다. 이전 화면의 성공 사건이나 전체 틱 이벤트를 되풀이하지 않는다.</summary>
    private void CollectTutorialScreenEvent(SessionEvent sessionEvent)
    {
        if (TutorialUsesScreenFrames && sessionEvent.Kind is SessionEventKind.ReturnedHome or SessionEventKind.Salvaged)
            _tutorialScreenEvents.Add(sessionEvent);
    }

    /// <summary>화면 경로가 연결됐을 때만 다음 프레임 결과를 바꾸는 게이트 필드·절대 타이머·남은 명령을 검사합에 포함한다.</summary>
    private void HashTutorialScreenState(Fnv1a hash)
    {
        if (_tutorialGate == null) return;
        MissionFrameGateState state = _tutorialGate.State;
        hash.Add(BitConverter.DoubleToInt64Bits(TutorialFrameSeconds));
        hash.Add(state.Stage);
        hash.Add(BitConverter.DoubleToInt64Bits(state.TimerSeconds));
        hash.Add(state.LockFrames);
        hash.Add(BitConverter.DoubleToInt64Bits(state.NextStageSeconds));
        hash.Add((int)state.ScriptPaused);
        hash.Add(state.ClearFrames);
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
