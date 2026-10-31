using Netstorm.Assets;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>원본 Tutorial 미션 시작의 프레임 게이트 외 필드다.</summary>
public sealed class MissionStartState
{
    /// <summary>상태 비트와 가상 단계 순번이다.</summary>
    public uint Flags { get; set; }
    /// <summary>현재 Guide 단계 순번이다.</summary>
    public uint Sequence { get; set; }
    /// <summary>현재 tellAtList의 항목 인덱스다.</summary>
    public uint TellAtIndex { get; set; }
    /// <summary>Start에서 0으로 쓰는 원본 +c0 필드다.</summary>
    public uint FieldC0 { get; set; }
    /// <summary>Start에서 0으로 쓰는 원본 +c4 필드다.</summary>
    public uint FieldC4 { get; set; }
}

/// <summary>월드가 공유할 수 있는 사제 포획과 일반 간헐천 설정 저장소다.</summary>
public sealed class MissionStartAux
{
    /// <summary>모든 포획 허용의 원본 signed DWORD다.</summary>
    public int AllowAnyCapture { get; set; }
    /// <summary>일반 간헐천 생성 여부의 원본 DWORD다.</summary>
    public uint OrdinaryGeysers { get; set; }
}

/// <summary>미션 시작이 읽거나 쓰는 현재 전역 및 공유 보조 상태다.</summary>
public sealed class MissionStartGlobals
{
    /// <summary>현재 입력 getter 미연결 시 로컬 소유자다.</summary>
    public uint LocalOwner { get; set; } = 1;
    /// <summary>현재 입력 getter 미연결 시 PlayerKnown 값이다.</summary>
    public uint PlayerKnown { get; set; }
    /// <summary>현재 입력 getter 미연결 시 편집 모드다.</summary>
    public uint Editor { get; set; }
    /// <summary>회수 금지 설정의 원본 signed DWORD다.</summary>
    public int DenySalvage { get; set; }
    /// <summary>승천 금지 설정의 원본 signed DWORD다.</summary>
    public int DenyAscend { get; set; }
    /// <summary>실제 월드와 공유할 포획·간헐천 저장소다.</summary>
    public MissionStartAux Aux { get; init; } = new();
}

/// <summary>시작 도중 실제 세션 전역을 원본 읽기 위치에서 조회한다.</summary>
public sealed class MissionStartReads
{
    /// <summary>현재 전투 소유자 조회다.</summary>
    public Func<uint>? LocalOwner { get; init; }
    /// <summary>현재 PlayerKnown 조회다.</summary>
    public Func<uint>? PlayerKnown { get; init; }
    /// <summary>현재 편집 모드 조회다.</summary>
    public Func<uint>? Editor { get; init; }
}

/// <summary>미션 시작이 설정·세계·UI에 수행하는 실제 효과다. 선택된 경로의 누락은 실패한다.</summary>
public sealed class MissionStartHooks
{
    /// <summary>현재 미션 설정 값을 조회한다.</summary>
    public Func<string, string>? Get { get; init; }
    /// <summary>지정 접두부의 섹션 이름을 순서대로 구한다.</summary>
    public Func<string, string>? SectionNames { get; init; }
    /// <summary>현재 미션 설정에 값을 쓴다.</summary>
    public Action<string, string>? Set { get; init; }
    /// <summary>현재 게임 시각을 double 초로 조회한다.</summary>
    public Func<double>? GameSeconds { get; init; }
    /// <summary>소유자의 분산 SP를 직접 지정한다.</summary>
    public Action<uint, float>? SetSp { get; init; }
    /// <summary>SP 화면을 갱신한다.</summary>
    public Action? RefreshSp { get; init; }
    /// <summary>소유자의 기술 목록을 지운다.</summary>
    public Action<uint>? ClearTechnology { get; init; }
    /// <summary>소유자에게 미션 기술 목록을 추가한다.</summary>
    public Action<uint, string>? AddTechnology { get; init; }
    /// <summary>소유자에게 사제가 있는지 조회한다.</summary>
    public Func<uint, bool>? HasPriest { get; init; }
    /// <summary>해당 소유자의 AI를 생성한다.</summary>
    public Action<uint>? CreateAi { get; init; }
    /// <summary>명령 Gump를 갱신한다.</summary>
    public Action? RefreshCommandGump { get; init; }
    /// <summary>랜덤 간헐천을 만든다.</summary>
    public Action? RandomGeysers { get; init; }
    /// <summary>카메라를 초기 위치로 돌린다.</summary>
    public Action? ResetCamera { get; init; }
}

/// <summary>10.78 미션의 공통 시작 순서를 실제 설정·세계 효과에 연결한다.
/// 근거: cpppj/src/o/RawMissionStart.h/.cpp, 원본 004c2b20·004c3100.</summary>
public sealed class RawMissionStart
{
    /// <summary>공통 프레임과 공유하는 게이트다.</summary>
    private readonly MissionFrameGateState _gate;
    /// <summary>현재 미션의 시작 상태다.</summary>
    private readonly MissionStartState _state;
    /// <summary>실제 전역과 공유할 설정이다.</summary>
    private readonly MissionStartGlobals _globals;
    /// <summary>현재 설정과 실제 세계 효과다.</summary>
    private readonly MissionStartHooks _hooks;
    /// <summary>연결된 현재 전역 조회기다. 없으면 globals 값을 사용한다.</summary>
    private MissionStartReads? _reads;

    /// <summary>게이트와 현재 미션 및 효과의 수명을 빌린다. 10.78과 설정 조회가 필요하다.</summary>
    /// <param name="edition">기준 판본.</param><param name="gate">공통 프레임 게이트.</param><param name="state">미션 상태.</param>
    /// <param name="globals">현재 전역.</param><param name="hooks">실제 설정과 세계 효과.</param>
    public RawMissionStart(OriginalEdition edition, MissionFrameGateState gate, MissionStartState state, MissionStartGlobals globals, MissionStartHooks hooks)
    {
        if (edition != OriginalEdition.Patch1078 || hooks.Get == null) throw new ArgumentException("MissionStart는 10.78과 현재 설정 조회가 필요하다");
        _gate = gate; _state = state; _globals = globals; _hooks = hooks;
    }

    /// <summary>실제 현재 입력 조회기 세 개를 함께 연결한다.</summary>
    /// <param name="reads">소유자·PlayerKnown·편집 모드 조회기.</param>
    public void BindCurrentInputs(MissionStartReads reads)
    {
        if (reads.LocalOwner == null || reads.PlayerKnown == null || reads.Editor == null) throw new ArgumentException("MissionStart 현재 입력 조회 누락");
        _reads = reads;
    }

    /// <summary>도달한 효과가 연결되지 않았을 때 성공으로 처리하지 않는다.</summary>
    /// <param name="effect">필요한 효과.</param>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("MissionStart 실제 효과 연결 누락");
    /// <summary>현재 소유자를 원본 읽기 시점마다 조회한다.</summary>
    private uint LocalOwner() => _reads?.LocalOwner?.Invoke() ?? _globals.LocalOwner;
    /// <summary>현재 PlayerKnown을 조회한다.</summary>
    private uint PlayerKnown() => _reads?.PlayerKnown?.Invoke() ?? _globals.PlayerKnown;
    /// <summary>현재 편집 모드를 조회한다.</summary>
    private uint Editor() => _reads?.Editor?.Invoke() ?? _globals.Editor;
    /// <summary>현재 미션의 설정 문자열을 읽는다.</summary>
    /// <param name="key">미션 설정 키.</param>
    private string Get(string key) => _hooks.Get!(key);

    /// <summary>기존 인덱스의 tellAtList 항목 첫 글자 뒤를 원본 atol로 읽어 기한을 갱신한다.</summary>
    public void RefreshTellAt()
    {
        string value = ConfigNumbers.ListItem(Get("tellAtList"), unchecked((int)_state.TellAtIndex));
        _gate.TellAtSeconds = value.Length == 0 ? 0.0 : ConfigNumbers.ParseLong(value[1..]);
    }

    /// <summary>원본 순서로 설정·SP·기술·AI·UI·시계를 초기화하고 마지막에 시작 완료를 쓴다.</summary>
    public void Start()
    {
        _state.FieldC0 = _state.FieldC4 = 0;
        _globals.DenySalvage = ConfigNumbers.ParseLong(Get("denySalvage"));
        _globals.DenyAscend = ConfigNumbers.ParseLong(Get("denyAscend"));
        _globals.Aux.AllowAnyCapture = ConfigNumbers.ParseLong(Get("allowAnyCapture"));
        _globals.Aux.OrdinaryGeysers = ConfigNumbers.ParseLong(Get("moreGeysers")) == 0 ? 1u : 0u;
        int demoTimer = ConfigNumbers.ParseLong(Get("demoRestartTimer"));
        if (demoTimer != 0) _gate.DemoEndSeconds = (double)demoTimer + Need(_hooks.GameSeconds)();
        string names = Need(_hooks.SectionNames)("@");
        Need(_hooks.Set)("tellAtList", names);
        RefreshTellAt();
        _state.Sequence = unchecked(_state.Sequence + 1); _state.Flags &= ~1u;
        float money = ConfigNumbers.ParseLong(Get("myStartMoney"));
        if (Editor() != 0) money = 194162.0f;
        if (PlayerKnown() != 0) Need(_hooks.SetSp)(LocalOwner(), money);
        Need(_hooks.RefreshSp)();
        Need(_hooks.ClearTechnology)(LocalOwner());
        uint techOwner = LocalOwner();
        string tech = Get("myTech");
        Need(_hooks.AddTechnology)(techOwner, tech);
        // 원본 소유자 1~8을 순회하며 매번 현재 로컬 소유자와 실제 사제 존재를 확인한다.
        for (uint owner = 1; owner < 9; owner++)
            if ((owner != LocalOwner() || _gate.Demo != 0) && Need(_hooks.HasPriest)(owner)) Need(_hooks.CreateAi)(owner);
        Need(_hooks.RefreshCommandGump)();
        if (ConfigNumbers.ParseLong(Get("randGeysers")) != 0) Need(_hooks.RandomGeysers)();
        Need(_hooks.ResetCamera)();
        _gate.StartSeconds = Need(_hooks.GameSeconds)();
        _gate.Started = 1;
    }
}
