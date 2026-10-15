namespace Netstorm.Core.Simulation;

/// <summary>
/// 원본 Tutorial 객체의 공통 프레임 필드 14개를 한 곳에 보관한다. 게이트와 튜토리얼 단계 판정이 같은 인스턴스를 읽고 쓴다.
/// 각 필드의 형(부호 있는 정수, 부호 없는 정수, float, double)은 원본 메모리 배치를 따르며 감김·부호 확장도 같다.
/// </summary>
/// <remarks>
/// 원본 10.78 PATCH 의 004c2990 기본값(65/1/0.5/10/0)과 004c34c0 의 읽기·쓰기 순서를 따른다.
/// 근거: docs/exe/cpp-mission-frame-gate-20261015.md, cpppj MissionFrameGateState. 2026-10-15 dotnetpj 에 추가.
/// </remarks>
public sealed class MissionFrameGateState
{
    /// <summary>현재 단계 글자(전역 005ca8e8)를 부호 확장한 값. 증가는 저위 8비트만 올리므로 127 다음은 -128이다.</summary>
    public int Stage { get; set; } = 65;

    /// <summary>초기·현재 프레임 표식(005ca8ec). 시작된 프레임 처리가 처음 쓰는 값은 0이다.</summary>
    public uint StageDirty { get; set; } = 1;

    /// <summary>1~6 튜토리얼 분배 번호(005ca8e4). 캠페인 번호가 아니며 범위 밖이면 분배하지 않는다.</summary>
    public uint TutorialNumber { get; set; }

    /// <summary>+68 데모 여부. 0이 아니면 전체를 데모로 해석하고 튜토리얼을 분배하지 않는다.</summary>
    public uint Demo { get; set; }

    /// <summary>+70 미션 시작 게임 시각(초). tellAt 의 상대 경과 기준이다.</summary>
    public double StartSeconds { get; set; }

    /// <summary>+78 다음 단계 안내 기한(초). 처음 0이면 곧바로 카운터를 판정한다.</summary>
    public double NextStageSeconds { get; set; }

    /// <summary>+80 후속 단계 안내 지연(float 초, 기본 0.5). 더하는 순간에만 double 로 넓힌다.</summary>
    public float DelaySeconds { get; set; } = 0.5f;

    /// <summary>+84 안내 잠금 프레임 수. 다이얼로그가 있으면 10으로 되돌리고 없으면 하나씩 줄인다.</summary>
    public int LockFrames { get; set; } = 10;

    /// <summary>+88 시작 완료 값. 0인 프레임은 어떤 조회도 효과도 실행하지 않는다.</summary>
    public uint Started { get; set; }

    /// <summary>+90 예약된 데모 종료 기한(초). 0은 예약 없음이다.</summary>
    public double DemoEndSeconds { get; set; }

    /// <summary>+98 튜토리얼 임시 타이머(게임 시각). 단계가 바뀌면 0으로 지운다.</summary>
    public double TimerSeconds { get; set; }

    /// <summary>+a8 tellAtList 의 상대 기한(초). 0은 예약 없음이다.</summary>
    public double TellAtSeconds { get; set; }

    /// <summary>+b8 이 미션이 시계 정지를 보유했는지. 실제 시계의 중첩 깊이와 구분한다.</summary>
    public uint ScriptPaused { get; set; }

    /// <summary>+c8 안내가 모두 사라진 연속 화면 수(부호 있는 DWORD). 하나라도 막히면 0으로 되돌린다.</summary>
    public int ClearFrames { get; set; }
}

/// <summary>
/// 공통 프레임이 호출하는 외부 경계의 현재 조회와 효과. 모든 항목을 반드시 채워야 하며 호출 시점마다 현재 상태를 읽는다.
/// 같은 프레임 안에서 앞선 효과(안내 열기 등)가 뒤의 조회 결과를 바꿀 수 있으므로 값을 미리 복사해 넘기지 않는다.
/// </summary>
/// <remarks>실제로 쓰이지 않는 경계(예: 출하 미션에 없는 tellAt/데모)는 빈 성공이 아니라 예외를 던지는 훅을 공급한다. 2026-10-15 추가.</remarks>
public sealed class MissionFrameGateHooks
{
    /// <summary>원본 게임 시각(0055b4d0, double 초). 벽시계나 24Hz 틱으로 바꾼 값을 넘기지 않는다.</summary>
    public Func<double>? GameSeconds { get; init; }

    /// <summary>현재 중첩 시계 정지 여부(00460de0).</summary>
    public Func<bool>? Paused { get; init; }

    /// <summary>미션 승패·사제 사건 판정(004c36c0). 시작된 프레임에서 가장 먼저 실행한다.</summary>
    public Action? CheckOutcome { get; init; }

    /// <summary>다이얼로그가 없으면 참(004c8e90의 비영 반환).</summary>
    public Func<bool>? DialogsClear { get; init; }

    /// <summary>지식 창이 열려 있는지(00445330). tellAt 실행을 막는다.</summary>
    public Func<bool>? KnowledgeOpen { get; init; }

    /// <summary>안내 창이 열려 있는지(004c79a0). 7프레임 재개를 막는다.</summary>
    public Func<bool>? GuideOpen { get; init; }

    /// <summary>루트의 0x4000 자식 창이 있는지(004641d0). 7프레임 재개를 막는다.</summary>
    public Func<bool>? BlockingGumpOpen { get; init; }

    /// <summary>현재 단계/데모 섹션의 안내를 연다(004c2a90). 성공하면 호출자가 <see cref="MissionFrameGate.PauseForDialog"/>를 부른다.</summary>
    public Action? TellStage { get; init; }

    /// <summary>현재 tellAtList 항목을 실행하고 다음 기한을 예약한다(004c3470).</summary>
    public Action? TellAt { get; init; }

    /// <summary>예약된 데모 종료 명령을 요청한다(00452f90).</summary>
    public Action? EndDemo { get; init; }

    /// <summary>튜토리얼 번호(1~6)와 현재 사건으로 해당 단계 판정을 실행하고 DWORD 반환을 돌려준다.</summary>
    public Func<uint, uint, uint>? Tutorial { get; init; }

    /// <summary>실제 시계 정지 깊이를 한 번 올린다(00460df0).</summary>
    public Action? Pause { get; init; }

    /// <summary>실제 시계 정지 깊이를 한 번 내린다(00460e30).</summary>
    public Action? Resume { get; init; }
}

/// <summary>
/// 원본 Tutorial 의 공통 프레임(004c34c0)·단계 증가(004c33f0)·안내 정지(004c2a40)·재지연(004c3170)을 현재 상태와
/// 외부 조회로 실행한다. 초나 24Hz 틱이 아니라 실제 화면 호출마다 한 번 <see cref="RunFrame"/>을 부른다.
/// </summary>
/// <remarks>
/// 처리 순서: 시작 여부 → 승패 → 안내 기한·10프레임 잠금 → tellAt → 7프레임 재개 → 데모 기한 → 튜토리얼 1~6 분배.
/// 각 훅 뒤에는 공유 상태를 다시 읽는다. 원본 실행 파일의 실제 기계어 328개 관찰과 비트 단위로 대조한다
/// (cpppj/tests/fixtures/missionframe-x86.tsv, MissionFrameGateTests). 근거: docs/exe/cpp-mission-frame-gate-20261015.md.
/// 2026-10-15 감사 D02 에 따라 cpppj RawMissionFrameGate 를 옮겨 dotnetpj 화면 경로의 단일 구현으로 추가.
/// </remarks>
public sealed class MissionFrameGate
{
    /// <summary>안내 잠금의 시작·재설정 값(10프레임). 원본 004c2990/004c33f0/004c34c0 의 즉값이다.</summary>
    public const int LockFramesDefault = 10;

    /// <summary>안내가 사라진 연속 화면이 이 수 이상이면 자기 정지를 푼다(7프레임).</summary>
    public const int ResumeFrames = 7;

    /// <summary>분배할 수 있는 튜토리얼 번호의 첫 값(1).</summary>
    private const uint FirstTutorialNumber = 1;

    /// <summary>분배할 수 있는 튜토리얼 번호의 마지막 값(6).</summary>
    private const uint LastTutorialNumber = 6;

    /// <summary>호출마다 현재 값을 읽는 외부 경계. 생성 시 전부 채워졌는지 검사한다.</summary>
    private readonly MissionFrameGateHooks _hooks;

    /// <summary>게이트와 단계 판정기가 공유하는 필드 저장소. 복사하지 않고 같은 참조를 쓴다.</summary>
    public MissionFrameGateState State { get; }

    /// <summary>
    /// 상태 저장소와 외부 경계를 묶는다. 경계가 하나라도 비어 있으면 실행하지 않는 정상 미션처럼 숨기지 않고 즉시 거부한다.
    /// </summary>
    /// <param name="state">원본 필드 저장소(호출자가 수명을 보장한다).</param>
    /// <param name="hooks">현재 시계·UI·승패·튜토리얼 경계.</param>
    public MissionFrameGate(MissionFrameGateState state, MissionFrameGateHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (hooks.GameSeconds == null || hooks.Paused == null || hooks.CheckOutcome == null || hooks.DialogsClear == null
            || hooks.KnowledgeOpen == null || hooks.GuideOpen == null || hooks.BlockingGumpOpen == null || hooks.TellStage == null
            || hooks.TellAt == null || hooks.EndDemo == null || hooks.Tutorial == null || hooks.Pause == null || hooks.Resume == null)
            throw new ArgumentException("미션 공통 프레임의 실제 시계/UI/분배 경계가 누락되었습니다.", nameof(hooks));
        State = state;
        _hooks = hooks;
    }

    /// <summary>
    /// 화면 프레임 하나의 공통 처리를 원본 순서대로 실행한다. 시작되지 않았으면 아무 조회도 효과도 없다.
    /// </summary>
    /// <param name="currentEvent">현재 처리 중인 사건 값. 튜토리얼 분배에 그대로 전달한다.</param>
    /// <returns>튜토리얼 분배가 있었으면 그 반환값, 없으면 0.</returns>
    public uint RunFrame(uint currentEvent = 0)
    {
        if (State.Started == 0) return 0;
        State.StageDirty = 0;
        _hooks.CheckOutcome!();
        // 안내 기한이 지났거나 시계가 멈췄으면 잠금 프레임을 센다. 다이얼로그가 있으면 되돌리고 0이 되는 순간 현재 단계 안내를 연다.
        if ((State.NextStageSeconds <= _hooks.GameSeconds!() || _hooks.Paused!()) && State.LockFrames > 0)
        {
            if (!_hooks.DialogsClear!()) State.LockFrames = LockFramesDefault;
            else State.LockFrames--;
            if (State.LockFrames < 1) _hooks.TellStage!();
        }
        // tellAt 은 경과 시간보다 엄격히 작을 때만 실행한다(같으면 실행하지 않는다).
        if (State.TellAtSeconds != 0.0 && State.TellAtSeconds < _hooks.GameSeconds!() - State.StartSeconds
            && _hooks.DialogsClear!() && !_hooks.KnowledgeOpen!()) _hooks.TellAt!();
        // 자기 정지가 있고 잠금이 정확히 0일 때만, 안내·다이얼로그·팝업이 7프레임 연속 없을 때 정지를 푼다. 잠금 중에는 카운터를 보존한다.
        if (State.ScriptPaused != 0 && State.LockFrames == 0)
        {
            if (!_hooks.GuideOpen!() && _hooks.DialogsClear!() && !_hooks.BlockingGumpOpen!())
            {
                State.ClearFrames = unchecked(State.ClearFrames + 1);
                if (State.ClearFrames >= ResumeFrames)
                {
                    // 이전 훅이 정지 표식을 지웠다면 실제 재개를 요청하지 않는다(원본이 필드를 다시 읽는 위치).
                    if (State.ScriptPaused != 0)
                    {
                        _hooks.Resume!();
                        State.ScriptPaused = 0;
                    }
                    State.ClearFrames = 0;
                }
            }
            else State.ClearFrames = 0;
        }
        // 데모 종료 기한은 현재 시각과 같아도 실행한다.
        if (State.Demo != 0 && State.DemoEndSeconds != 0.0 && State.DemoEndSeconds <= _hooks.GameSeconds!())
        {
            _hooks.EndDemo!();
            State.DemoEndSeconds = 0.0;
        }
        if (State.LockFrames == 0 && State.Demo == 0 && State.TutorialNumber >= FirstTutorialNumber && State.TutorialNumber <= LastTutorialNumber)
            return _hooks.Tutorial!(State.TutorialNumber, currentEvent);
        return 0;
    }

    /// <summary>
    /// 후속 단계 안내를 예약한다. 시계가 멈췄거나 이전 잠금이 남았으면 아무것도 바꾸지 않는다.
    /// 단계 글자는 저위 8비트만 올려 부호 확장하고, 안내 기한은 float 지연을 double 로 넓혀 현재 게임 시각에 더한다
    /// (디컴파일 C 의 현재 시각 float 축소는 따르지 않는다).
    /// </summary>
    /// <returns>예약했으면 true. 호출자가 얻는 편의 값이며 원본 함수의 반환값을 새로 해석한 것이 아니다.</returns>
    public bool Advance()
    {
        if (_hooks.Paused!() || State.LockFrames >= 1) return false;
        byte next = unchecked((byte)((uint)State.Stage + 1U));
        State.Stage = unchecked((sbyte)next);
        State.LockFrames = LockFramesDefault;
        State.NextStageSeconds = (double)State.DelaySeconds + _hooks.GameSeconds!();
        State.TimerSeconds = 0.0;
        return true;
    }

    /// <summary>
    /// 이 미션의 안내 정지를 한 번 잡는다. 다른 보유자 때문에 이미 멈춰 있어도 자기 표식이 0이면 별도 깊이를 잡고,
    /// 자기 표식이 있더라도 실제 시계가 실행 중이면 다시 정지를 요청한다.
    /// </summary>
    public void PauseForDialog()
    {
        if (!_hooks.Paused!() || State.ScriptPaused == 0)
        {
            _hooks.Pause!();
            State.ScriptPaused = 1;
        }
    }

    /// <summary>이미 0이 아닌 다음 단계 기한만 현재 게임 시각 + float 지연으로 다시 예약한다. 처음 0은 유지한다.</summary>
    public void Postpone()
    {
        if (State.NextStageSeconds != 0.0) State.NextStageSeconds = (double)State.DelaySeconds + _hooks.GameSeconds!();
    }
}
