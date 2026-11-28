using Netstorm.Core.Original;

// 실제호스트의Root/장치/State가동일한장기자원을빌린다.
namespace Netstorm.Client.Host;

/// <summary>파일·메뉴·음향과 State 직접쓰기의 나머지 장기소유자를 연결한다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>메뉴/미션/음향장치교체에서재구성하지않는원문자원이다.</summary>
    private readonly ClientStateResources _stateResources = new();
    /// <summary>같은실제소유자만읽으며파일/표/곡을초기화하지않는다.</summary>
    public ClientStateResources StateResources => _stateResources;
    /// <summary>현재 실제 장기물리의 폭별State쓰기다. 시작자료가없는화면호스트에서는미설치다.</summary>
    private StateWorldCleanupGlobals? _stateWorldGlobals;
    /// <summary>전체State부모가빌릴실제전역접근이다. 하위효과나정리를임의실행하지않는다.</summary>
    public StateWorldCleanupGlobals StateWorldGlobals => _stateWorldGlobals ?? throw new InvalidOperationException("실제State장기전역미설치");
    /// <summary>실제 물리 소유자 검증 뒤 현재 Player/battle/Cloud와 같은 State 주소 공간을 연결한다. 장면 감독은 실행하지 않는다.</summary>
    private void BindStateResourceOwners()
    {
        _stateResources.BindCurrentMusicOwners(_gameplay, _surfaceGlobals, _localState, _connections, _startup?.Cloud);
        BindSceneMusicWorld();
        BindNativeThunderOwner();
        if (_startup is not { } startup) return;
        _stateWorldGlobals = ClientStateWorldGlobals.Bind(_stateResources.Context(_surfaceGlobals, _localState, _connections, startup.Cloud, _nsInfoGlobals));
    }

    /// <summary>현재 공통 UI에서 보존 종류를 제외한 노드의 State 지연 삭제를 요청한다.</summary>
    public void KillNativeStateGumps() => (_mainMenu ?? throw new InvalidOperationException("State UI의 실제 메뉴 미설치")).Tree.KillStateGumps();

    /// <summary>현재 지연 삭제를 마치고 해제된 메뉴 별칭을 반납한다.</summary>
    public void CleanupNativeStateGumps()
    {
        var menu = _mainMenu ?? throw new InvalidOperationException("State UI의 실제 메뉴 미설치");
        menu.Tree.Lifetime.Cleanup();
        menu.ForgetReleasedControls();
        _menuDirty = true;
    }

    /// <summary>현재 제품의 정지·공통 UI·HUD·SP·메타 이탈·장면 음향·옵션을 실제 State 정리 위치에 연결한다. 선택된 저장 장치가 없는 갈래는 기존 명시 오류를 유지한다.</summary>
    /// <param name="reason">원문 reason0은 전체 연결을 닫고 reason1은 현재 전환 연결만 반납한다.</param>
    public void CleanupNativeWorld(uint reason) => CleanupNativeWorld(reason, new ClientStateWorldDevices
    {
        ResumeGame = PopNativeGamePause,
        ResumeAudio = () => NativeStateAudio.PopMute(),
        KillGumps = KillNativeStateGumps,
        CleanupGumps = CleanupNativeStateGumps,
        ReplaceUber = ReplaceNativeUberRoot,
        RefreshSp = RefreshNativeStormPower,
        LeaveMetamap = () => NativeMetaDisplayCleanup.LeaveCurrent(_challenge, _surfaceGlobals, _localState),
        Assertion = line => throw new InvalidOperationException($"원문 State 중복 모드 진단:{line}"),
        StartSceneMusic = battle => NativeStateAudio.StartScene(battle),
        StopSoundLoops = () => NativeStateAudio.StopLoops(),
        InterpretOptions = InterpretNativeOptions,
    });

    /// <summary>실제로 선택된 음향 효과 위치에서만 같은 장기 소유자를 요구한다. 조회로 재생기를 만들거나 상태를 초기화하지 않는다.</summary>
    private HostMissionAudio NativeStateAudio => _missionAudio ?? throw new InvalidOperationException("State 정리의 실제 장기 음향 미설치");

    /// <summary>실제같은Root자원으로전체Native State정리를호출한다. 아직설치되지않은UI/음향/저장장치는선택위치에서실패한다.</summary>
    public void CleanupNativeWorld(uint reason, ClientStateWorldDevices devices)
    {
        var startup = _startup ?? throw new InvalidOperationException("전체State정리의실제시작자산미설치");
        ClientStateWorldCleanup.Cleanup(reason, _stateResources.Context(_surfaceGlobals, _localState, _connections, startup.Cloud, _nsInfoGlobals),
            _surfaceGlobals, _spGlobals, devices, SelectNativeCursor, () => CancelNativeCursorForState(), () => ChatChannels.ResetOccupied(),
            RetireNativeMissionRuntimeForState);
    }

    /// <summary>실제 State 자식 정리 뒤 이전 미션의 관측·표시·물리 알림만 끊고 같은 Runtime을 동기 반납한다. 새 HUD와 장기 음향/Renderer는 유지한다.</summary>
    private void RetireNativeMissionRuntimeForState()
    {
        var runtime = _missionRuntime ?? throw new InvalidOperationException("State 반납의 실제 현재 Runtime 없음");
        var session = runtime.Session;
        if (!ReferenceEquals(session, _missionSession)) throw new InvalidOperationException("State 반납의 실제 현재 Session 불일치");
        session.EnsureStateRuntimeIdle();
        if (!session.ProcessBindingsReturned()) throw new InvalidOperationException("State 미션 장치 반납 전 실제 Kernel 반납 필요");
        _sessionDump?.UnbindSession(session);
        _worldSprites = null; _worldSpriteState = null; _worldTileModes = null;
        _worldPrelude = null; _worldCloudQueue = null; _worldShapesDb = null; _worldLogicalPalette = null;
        NativeMinimap.UnbindWorld();
        _missionBurning?.Dispose();
        _missionBurning = null;
        _missionAudio?.UnbindMissionForState(session);
        runtime.RetireForState();
        if (!runtime.Released || _gameplay.Borrowed() || _surfaceGlobals.Storage.Borrowed())
            throw new InvalidOperationException("State 미션 Runtime의 실제 동기 반납 미완료");
        _missionRuntime = null;
        _missionSession = null;
        _missionDemo = 0;
        _menuDirty = true;
    }
}
