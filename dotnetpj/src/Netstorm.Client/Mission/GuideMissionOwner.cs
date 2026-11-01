using Netstorm.Client.Input;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Client.Mission;

/// <summary>Guide와 실제 Client가 공유하는 미션 모드 및 월드 수명 토큰이다.</summary>
public sealed class GuideMissionGlobals
{
    /// <summary>원본의 현재 미션 모드. 수명 함수가 같은 필드를 직접 변경한다.</summary>
    public bool MissionMode;
    /// <summary>현재 월드가 빌린 미션 수명 토큰이다.</summary>
    public uint WorldMissionToken { get; set; }
}

/// <summary>현재 Guide가 소유하는 미션 필드다. 실제 월드의 수명은 포함하지 않는다.</summary>
public sealed class GuideMissionRuntime
{
    /// <summary>이 미션 수명을 식별하는 토큰.</summary>
    public uint SaveToken { get; }
    /// <summary>튜토리얼 가상 단계를 사용하는지.</summary>
    public bool Guided { get; }
    /// <summary>재시작에 보존하는 Mission+64 필드.</summary>
    public uint Mode { get; set; }
    /// <summary>현재 프레임과 데모 상태.</summary>
    public MissionFrameGateState Gate { get; } = new();
    /// <summary>공통 Start와 가상 단계가 공유하는 상태.</summary>
    public MissionStartState State { get; } = new();

    /// <summary>스크립트 판정과 초기 모드로 현재 미션 상태를 만든다.</summary>
    /// <param name="guided">10.78 튜토리얼 가상 단계를 쓰는지.</param><param name="demo">초기 데모 값.</param>
    /// <param name="mode">초기 복귀 모드.</param><param name="token">새 수명 토큰.</param>
    internal GuideMissionRuntime(bool guided, uint demo, uint mode, uint token)
    { Guided = guided; Gate.Demo = demo; Mode = mode; SaveToken = token; }
}

/// <summary>Guide가 실제 호스트에 요청하는 화면과 지연 요청 수명 효과다.</summary>
public sealed class GuideMissionOwnerHooks
{
    /// <summary>현재 Guide에 속한 지연 Fort 요청을 반납한다.</summary>
    public Action? ReleaseFortRequest { get; init; }
    /// <summary>현재 기술 제한 표의 전체 허용 값을 변경한다.</summary>
    public Action<uint>? SetAllowAll { get; init; }
    /// <summary>새 미션이 준비되면 실제 로딩 화면과 월드 요청을 연결한다.</summary>
    public Action<GuideMissionRuntime>? Created { get; init; }
    /// <summary>미션 중단 확인 Tell을 연다.</summary>
    public Action<string>? Tell { get; init; }
    /// <summary>현재 단계 안내를 표시한다.</summary>
    public Action? ShowStage { get; init; }
}

/// <summary>Guide의 실제 스크립트·Runtime을 소유하고 같은 입력 큐와 문자열 고리의 미션 명령을 소비한다.
/// 근거: cpppj/src/client/UberGump.cpp 및 CampaignGuideRelease.cpp. 월드 생성과 프레임 효과는 호스트가 연결한다.</summary>
public sealed class GuideMissionOwner
{
    /// <summary>현재 Client의 공유 설정.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>같은 Client의 입력 사건 큐.</summary>
    private readonly InputQueue _input;
    /// <summary>스크립트 명령의 문자열 수명 고리.</summary>
    private readonly ScriptStringRing _strings;
    /// <summary>실제 Player 및 AI 반납 소유자.</summary>
    private readonly PlayerParticipation _players;
    /// <summary>현재 제한 설정.</summary>
    private readonly MissionStartGlobals _startGlobals;
    /// <summary>현재 미션 번호.</summary>
    private readonly TutorialMissionReleaseGlobals _tutorialGlobals;
    /// <summary>Client와 공유하는 모드와 월드 토큰.</summary>
    private readonly GuideMissionGlobals _globals;
    /// <summary>실제 호스트 효과.</summary>
    private readonly GuideMissionOwnerHooks _hooks;
    /// <summary>현재 미션 설정 객체. 공장 실패 뒤에도 정리가 필요할 수 있다.</summary>
    private MissionScript? _mission;
    /// <summary>마지막으로 발급한 미션 수명 토큰.</summary>
    private uint _nextToken;

    /// <summary>현재 스크립트. 반납 후에는 null이다.</summary>
    public MissionScript? Mission => _mission;
    /// <summary>현재 Guide 상태. 생성이 끝나기 전에는 null이다.</summary>
    public GuideMissionRuntime? Runtime { get; private set; }

    /// <summary>실제 공유 저장소와 호스트 효과를 빌린다.</summary>
    /// <param name="configuration">Client 설정.</param><param name="input">같은 입력 큐.</param>
    /// <param name="strings">같은 스크립트 문자열 고리.</param><param name="players">같은 Player 소유자.</param>
    /// <param name="startGlobals">공유 제한 값.</param><param name="tutorialGlobals">공유 미션 번호.</param>
    /// <param name="globals">공유 모드와 월드 토큰.</param><param name="hooks">실제 호스트 효과.</param>
    public GuideMissionOwner(ConfigInterface configuration, InputQueue input, ScriptStringRing strings, PlayerParticipation players,
        MissionStartGlobals startGlobals, TutorialMissionReleaseGlobals tutorialGlobals, GuideMissionGlobals globals, GuideMissionOwnerHooks hooks)
    {
        _configuration = configuration ?? throw new ArgumentNullException(nameof(configuration));
        _input = input ?? throw new ArgumentNullException(nameof(input));
        _strings = strings ?? throw new ArgumentNullException(nameof(strings));
        _players = players ?? throw new ArgumentNullException(nameof(players));
        _startGlobals = startGlobals ?? throw new ArgumentNullException(nameof(startGlobals));
        _tutorialGlobals = tutorialGlobals ?? throw new ArgumentNullException(nameof(tutorialGlobals));
        _globals = globals ?? throw new ArgumentNullException(nameof(globals));
        _hooks = hooks ?? throw new ArgumentNullException(nameof(hooks));
    }

    /// <summary>선택된 실제 효과가 없으면 그 지점에서 오류를 낸다.</summary>
    /// <param name="effect">호출할 실제 효과.</param>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Guide의 실제 호스트 효과 미연결");
    /// <summary>현재 Runtime이 없으면 원본 선행 조건 위반을 알린다.</summary>
    private GuideMissionRuntime Current() => Runtime ?? throw new InvalidOperationException("현재 Guide 미션이 없다");

    /// <summary>현재 명시 source를 반납 전에 보유하고 새 미션을 시작한다.</summary>
    /// <param name="name">미션 이름.</param><param name="demo">초기 데모 값.</param><param name="mode">초기 복귀 모드.</param>
    /// <param name="source">첫 미션의 명시 source. 생략하면 현재 미션 또는 기본 source를 쓴다.</param>
    public void Begin(string name, uint demo = 0, uint mode = 0, Func<string, byte[]?>? source = null)
    {
        var selected = source ?? _mission?.FileSource();
        var hooks = Effects(selected, demo, mode);
        GuideMissionTransitions.Begin(name, hooks);
    }

    /// <summary>선택한 source를 유지한 채 현재 미션 설정과 Runtime을 만든다.</summary>
    private void Create(string name, uint demo, uint mode, Func<string, byte[]?>? source)
    {
        if (_mission != null || Runtime != null) throw new InvalidOperationException("새 Guide 미션 생성 전에 기존 미션이 남아 있다");
        _mission = GuideMissionScriptLifetime.Create(_configuration, name, ref _globals.MissionMode, source);
        if (!_mission.Loaded) throw new InvalidOperationException("미션 파일이 없다: " + name);
        string type = _mission.MissionType();
        bool guided = string.Equals(type, "tutorial", StringComparison.OrdinalIgnoreCase);
        if (!guided && !string.Equals(type, "battle", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("미구현 미션 형식: " + type);
        if (_nextToken == uint.MaxValue) throw new OverflowException("미션 수명 토큰 소진");
        Runtime = new GuideMissionRuntime(guided, demo, mode, ++_nextToken);
        Need(_hooks.Created)(Runtime);
    }

    /// <summary>현재 미션만 반납하고 World와 입력 큐는 그대로 둔다. 생성 실패한 스크립트도 정리할 수 있다.</summary>
    public void Release()
    {
        Need(_hooks.ReleaseFortRequest)();
        void ReleaseBase()
        {
            Runtime = null;
            _globals.WorldMissionToken = 0;
            GuideMissionScriptLifetime.Release(ref _mission, ref _globals.MissionMode, _configuration);
        }
        if (Runtime is { Guided: true } runtime)
            RawTutorialMissionRelease.Release(_startGlobals, runtime.Gate, _tutorialGlobals, _players, _hooks.SetAllowAll, ReleaseBase);
        else ReleaseBase();
    }

    /// <summary>같은 현재 객체에 Guide 교체 효과를 연결한다. source는 해제 전에 보유한다.</summary>
    private GuideMissionTransitionHooks Effects(Func<string, byte[]?>? source, uint demo = 0, uint mode = 0) => new()
    {
        Current = () => Runtime != null,
        Finish = () => { var current = Current(); if (current.Guided) MissionLifecycle.RunTutorialStage(current.State, 6, new()); },
        Release = Release,
        Create = name => Create(name, demo, mode, source),
        Get = key => (_mission ?? throw new InvalidOperationException("현재 Guide 설정이 없다")).Get(key) ?? "",
        Mode = value => Current().Mode = value,
        Demo = value => Current().Gate.Demo = value,
    };

    /// <summary>현재 미션 명령을 소비한다. 그 외 입력은 false로 남겨 상위 UI에 넘긴다.</summary>
    /// <param name="input">같은 큐에서 꺼낸 입력 사건.</param>
    public bool Dispatch(InputEvent input)
    {
        var hooks = new GuideMissionCommandHooks
        {
            Begin = token => Begin(_strings.Resolve(token)),
            Restart = () =>
            {
                var runtime = Current();
                var request = GuideMissionTransitions.CaptureReload((_mission ?? throw new InvalidOperationException("현재 미션 설정이 없다")).Name, runtime.Mode, runtime.Gate.Demo);
                GuideMissionTransitions.Restart(request, Effects(_mission.FileSource()));
            },
            Fork = value => GuideMissionTransitions.Fork(value, Effects(_mission?.FileSource())),
            End = () => GuideMissionTransitions.End(Effects(_mission?.FileSource())),
            InvalidBegin = _ => throw new InvalidOperationException("Guide Begin의 문자열 토큰 관문 위반"),
        };
        if (GuideMissionTransitions.Dispatch(input, hooks)) return true;
        if (MissionGuideInput.DispatchAbort(input, _input, _hooks.Tell)) return true;
        if (Runtime is { Guided: true } runtime)
            return MissionGuideInput.DispatchFirst(input, () => MissionGuideInput.MissionFirst(runtime.State, _hooks.ShowStage));
        return Runtime == null && MissionGuideInput.DispatchFirst(input, null);
    }
}
