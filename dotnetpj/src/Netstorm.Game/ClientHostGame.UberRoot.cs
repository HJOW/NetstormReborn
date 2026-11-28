using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 전체 State가 빌릴 HUD 부모를 같은 장기 메뉴·지도·입력 장치에 설치한다.
namespace Netstorm.Client.Host;

/// <summary>전체 UberRoot의 실제 네 공장과 분배 대여다. State 확정이나 월드 적재를 대신하지 않는다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>같은 실제 트리의 전체 부모·구분선 분배 대여다. 생성은 Replace 호출까지 미룬다.</summary>
    private RawUberRootControls? _nativeUberRoot;

    /// <summary>동일 메뉴·지도·입력으로 설치한 전체 HUD 장치다.</summary>
    public RawUberRootControls NativeUberRoot => _nativeUberRoot ?? throw new InvalidOperationException("실제 전체 HUD 분배 미설치");

    /// <summary>전체 State가 만든 현재 HUD를 재사용한다. 일반 진입의 임시 루트와 구별하고 실제 참가자 창 소유자도 확인한다.</summary>
    private UberRootNode? CurrentNativeMissionHud()
    {
        var tree = (_mainMenu ?? throw new InvalidOperationException("현재 HUD의 실제 공통 트리 없음")).Tree;
        var root = HostUberRoot.CurrentRoot(tree, _nativeUberRoot);
        if (root is null) return null;
        var factory = _rootFactories ?? throw new InvalidOperationException("현재 HUD의 실제 NSInfo 공장 없음");
        if (HostUberRoot.RequireChild(tree, root, root.Knowledge, root.Knowledge, factory.Primary) is not HudNsInfoNode)
            throw new InvalidOperationException("현재 HUD의 실제 NSInfo 자식 종류 불일치");
        return root;
    }

    /// <summary>현재 HTML 파서가 실제로 작성한 같은80BYTE 배경 이름을 읽는다.</summary>
    private string NativeHudBackgroundName() => HtmlTextParser.HtmlBackgroundName(
        (_htmlWindows?.Parser ?? throw new InvalidOperationException("전체 HUD의 실제 HTML 배경 소유자 없음")).Globals);

    /// <summary>최초 NSInfo 생성 또는 정리된 첫 창의 실제 재생성을 선택한다.</summary>
    private uint CreateNativeKnowledgePanel()
    {
        if (_rootFactories is null)
        {
            _ = EnsureRootFactories(_surfaceGlobals.Storage);
            return _nsInfoGlobals.Current.Value;
        }
        return _rootFactories.RecreatePrimary();
    }

    /// <summary>현재 실제 Client 전역만 사용 위치에서 조회한다.</summary>
    private uint ReadNativeUberRoot(uint address)
    {
        var menu = _mainMenu ?? throw new InvalidOperationException("전체 HUD의 실제 메뉴 없음");
        var tree = menu.Tree;
        return address switch
        {
            0x540451 => 43,
            0x5411c8 => menu.BackgroundType,
            0x542664 => 1,
            0x5c78f0 or 0x531860 => unchecked((uint)tree.Create.ScreenWidth),
            0x5c78f4 or 0x531864 => unchecked((uint)tree.Create.ScreenHeight),
            0x5949d0 => _minimapGlobals.Current,
            0x5ca920 => _productionGlobals.BackgroundType,
            0x5ca924 => _productionGlobals.BackgroundFrame,
            0x5ca928 => _productionGlobals.PanelFrame,
            0x565e70 => (_htmlWindows?.Parser ?? throw new InvalidOperationException("전체 HUD의 실제 배경 없음")).Globals.Background[0],
            0x540cec => 0x503c8b,
            0x540cf8 => 0x508ab4,
            0x542a04 => 0x020000c4,
            _ => throw new ArgumentOutOfRangeException(nameof(address), "전체 HUD의 실제 현재 전역 미연결"),
        };
    }

    /// <summary>전체 생성자가 작성하는 세 생산 전역을 같은 장기 소유자에 반영한다.</summary>
    private void WriteNativeUberRoot(uint address, uint value)
    {
        switch (address)
        {
            case 0x5ca920: _productionGlobals.BackgroundType = value; return;
            case 0x5ca924: _productionGlobals.BackgroundFrame = value; return;
            case 0x5ca928: _productionGlobals.PanelFrame = value; return;
            default: throw new ArgumentOutOfRangeException(nameof(address), "전체 HUD의 실제 쓰기 전역 미연결");
        }
    }

    /// <summary>현재 TYPE의 icon.B/C 개수를 매번 다시 조회한다.</summary>
    private int NativeRankIconCount(byte kind)
    {
        return AssetTables.IconFrameCount(kind);
    }

    /// <summary>원문 순서대로 B 개수와 C 개수를 읽고32비트 곱의 랭크 상한을 구한다.</summary>
    private int NativeMaximumRank()
    {
        int minor = NativeRankIconCount((byte)'B');
        int major = NativeRankIconCount((byte)'C');
        return unchecked(major * minor - 1);
    }

    /// <summary>같은 현재 Core·manager·TYPE로 원문 기술 표시 갱신을 실행한다.</summary>
    private void RefreshNativeChatRankPrefix() => ClientChatRuntime.RefreshClientChatRankPrefix(_chatManagerGlobals,
        _subscriberGlobals, _gameplay.CoreData.ActiveBox,
        () => _gameplay.CoreData.Read(_gameplay.Player, _surfaceGlobals.Storage.OwnerMode.Challenge ? 1u : 0u,
            new() { Assertion = line => ErrorLog.RecordAssertion("Core 원문 진단", ".\\Player.cpp", line) }),
        () => NativeRankIconCount((byte)'B'),
        NativeMaximumRank,
        line => ErrorLog.RecordAssertion("Chat 원문 진단", ".\\Chat.cpp", line));

    /// <summary>실제 네 공장·생산 전역·HTML 배경·도킹을 장기 트리에 설치한다. 노드 생성과 State 정리는 실행하지 않는다.</summary>
    private void InitializeNativeUberRoot()
    {
        if (_nativeUberRoot is not null) throw new InvalidOperationException("전체 HUD 분배 중복 설치");
        var menu = _mainMenu ?? throw new InvalidOperationException("전체 HUD의 실제 메뉴 없음");
        var menus = _htmlMenus ?? throw new InvalidOperationException("전체 HUD의 실제 메뉴 열 없음");
        var baseHandle = menu.Tree.EventHooks.Handle ?? throw new InvalidOperationException("전체 HUD의 실제 기본 입력 없음");
        _nativeUberRoot = HostUberRoot.Assemble(menu.Tree, menus.State, NativeHudDockGlobals, new()
        {
            CreateMinimap = () => NativeMinimap.Construct(),
            CreateProduction = CreateNativeProductionPanel,
            CreateKnowledge = CreateNativeKnowledgePanel,
            CreateChat = () => NativeChatWindows.Create(),
        }, new()
        {
            Read = ReadNativeUberRoot,
            Write = WriteNativeUberRoot,
            Type = type => type < (uint)_surfaceGlobals.Storage.Types.Count ? type : throw new ArgumentOutOfRangeException(nameof(type), "전체 HUD 실제 TYPE 범위"),
            Translate = text => _chatWindowGlobals.Store(TranslateNativeChatText(text)),
            Help = (token, text) => menu.Tree.SetHelpText(token, _chatWindowGlobals.Strings[text]),
            HelpWord = menu.Tree.SetHelpWord,
            RefreshTechnology = RefreshNativeChatRankPrefix,
            SavedMessage = () => RawUberMessageProbe.SavedMessage,
            EqualText = (token, text) => token == RawUberMessageProbe.SavedMessage && string.Equals(NativeHudBackgroundName(), text, StringComparison.OrdinalIgnoreCase),
            // 실제 GIF 후단은 아직 연결되지 않았으므로 선택된 위치에서 명시 오류를 유지한다.
            AllocateMessage = () => throw new InvalidOperationException("전체 HUD의 실제 배경 GIF 공장 미연결"),
            Poll = Poll,
            Cursor = SelectNativeCursor,
            BaseHandle = baseHandle,
            ClosePopups = () => menus.RequestClosePopups(() => _menuDirty = true),
            Replaced = _ => menu.ForgetReleasedControls(),
            Changed = () => _menuDirty = true,
            DockInvalidate = (_, _, _, _, _) => _menuDirty = true,
            DockFinishResize = () => { NativeRenderer.BackgroundRedraw.Value = 1; _menuDirty = true; },
            DockChatVisible = visible => NativeChatGeometry.SetHelperVisible(NativeChatWindows.CurrentRoot, visible ? 1u : 0u),
            DockChatInput = visible => NativeChatGeometry.SetInputVisible(NativeChatWindows.CurrentRoot, visible ? 1u : 0u),
        });
    }

    /// <summary>원문 전체 공장으로 기존 직계 자식을 옮기고 현재 등록0을 교체한다.</summary>
    public void ReplaceNativeUberRoot()
    {
        NativeUberRoot.Replace();
        _missionHudRouting?.Dispose();
        _missionHudRouting = null;
    }

    /// <summary>전체 부모가 소유한 노드와 자식 우선 공통 수명을 정리하고 파생 분배 대여를 반납한다.</summary>
    private void ReleaseNativeUberRoot()
    {
        _nativeUberRoot?.Dispose();
        _nativeUberRoot = null;
    }
}
