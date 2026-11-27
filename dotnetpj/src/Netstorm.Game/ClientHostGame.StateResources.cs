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
    /// <summary>물리소유자가준비된뒤현재battle조회와동일State주소공간을한번설치한다.</summary>
    private void BindStateResourceOwners()
    {
        _stateResources.Music.CurrentBattle = () => _surfaceGlobals.Storage.OwnerMode.Battle ? 1u : 0u;
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

    /// <summary>실제같은Root자원으로전체Native State정리를호출한다. 아직설치되지않은UI/음향/저장장치는선택위치에서실패한다.</summary>
    public void CleanupNativeWorld(uint reason, ClientStateWorldDevices devices)
    {
        var startup = _startup ?? throw new InvalidOperationException("전체State정리의실제시작자산미설치");
        ClientStateWorldCleanup.Cleanup(reason, _stateResources.Context(_surfaceGlobals, _localState, _connections, startup.Cloud, _nsInfoGlobals),
            _surfaceGlobals, _spGlobals, devices, SelectNativeCursor, () => CancelNativeCursorForState(), () => ChatChannels.ResetOccupied());
    }
}
