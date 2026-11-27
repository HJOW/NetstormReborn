using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host;

// 미션 HUD 구분선 세 개를 만들고 도크 배치로 실제 지도 영역을 정한다.
public sealed partial class ClientHostGame
{
    /// <summary>미션 세로 구분선 토큰이다. 0이면 미설치다.</summary>
    private uint _hudVertical;
    /// <summary>미션 미니맵 구분선 토큰이다. 0이면 미설치다.</summary>
    private uint _hudMinimapDivider;
    /// <summary>미션 채팅 구분선 토큰이다. 0이면 미설치다.</summary>
    private uint _hudChatDivider;
    /// <summary>미션 도크 배치 실행기다. 구분선 토큰을 조회한다.</summary>
    private HudDock? _missionHudDock;
    /// <summary>임시 루트의 채팅·구분선 사건을 실제 도킹 본체로 전달한다.</summary>
    private HostMissionHudRouting? _missionHudRouting;
    /// <summary>미션 구분선 노드의 소유자 표시다.</summary>
    private readonly object _hudDividerOwner = new();

    /// <summary>미션 진입에 HUD 구분선 세 개를 만들고 도크 배치로 실제 지도 영역을 정한다.</summary>
    /// <remarks>2026-11-26 1.075에서 R5-4b용으로 추가했다. 근거: cpppj HudDock.cpp의 kMoved 배치와 UberRootGump.Construct의 구분선 기본 배치다. 패널 생성·kHudMode·메뉴 전환은 R5-4c·R5-4d에서 잇는다. 세로·미니맵 구분선은 원본00542664의 프레임1을 쓰며 채팅 구분선은 배경 프레임0을 쓴다.</remarks>
    private void EnsureMissionHudDividers()
    {
        if (_hudVertical != 0) return;
        var menu = _mainMenu ?? throw new InvalidOperationException("HUD 구분선의 실제 공통 트리 없음");
        var tree = menu.Tree;
        var globals = _hudDockGlobals ?? throw new InvalidOperationException("HUD 구분선의 실제 지도 전역 없음");
        uint fortType = menu.BackgroundType;
        // 원본 00542664의 정적 초기값이며 세로 및 미니맵 구분선에 적용한다.
        const uint dividerFrame = 1;
        HudDock? dock = null;
        var hooks = new HudDockHooks
        {
            Node = tree.Node,
            Divider = token => tree.Node(token) as HudDividerNode,
            ClosePopups = () => _htmlMenus?.RequestClosePopups(() => _menuDirty = true),
            BaseAttach = tree.Tree.Attach,
            Push = tree.Tree.Push,
            Attach = (token, x, y, parent, mode) =>
            {
                if (token == _hudVertical || token == _hudMinimapDivider || token == _hudChatDivider)
                    (dock ?? throw new InvalidOperationException("HUD 실제 구분선 처리기 없음")).Attach(token, x, y, parent, mode);
                else
                    (tree.TreeHooks.Attach ?? throw new InvalidOperationException("HUD 자식의 실제 부착 장치 없음"))(token, x, y, parent, mode);
            },
            Move = tree.Tree.Move,
            ResolveSerial = tree.Reference,
            Bubble = (token, code, first, second) => tree.Events.Bubble(token, code, first, second),
            ConstructBackground = (token, width, height, flags, type, frame) => tree.Backgrounds.Construct(token, width, height, flags, 1, type, frame),
            Activate = (token, enabled) => tree.Lifetime.SetOverlay(token, enabled != 0),
            Invalidate = (_, _, _, _, _) => _menuDirty = true,
            FinishResize = () => { NativeRenderer.BackgroundRedraw.Value = 1; _menuDirty = true; },
            ChatVisible = visible => NativeChatWindows.Geometry.SetHelperVisible(NativeChatWindows.CurrentRoot, visible ? 1U : 0U),
            ChatInput = visible => NativeChatWindows.Geometry.SetInputVisible(NativeChatWindows.CurrentRoot, visible ? 1U : 0U),
            BaseHandle = (code, first, second) => (_missionHudRouting ?? throw new InvalidOperationException("HUD 실제 기본 입력 장치 없음")).HandleBase(code, first, second),
        };
        var dividers = new HudDividerGump(() => tree.Create.ScreenWidth, () => tree.Create.ScreenHeight, () => (sbyte)55, hooks);
        uint root = tree.Create.Entries[0];
        int height = _driver.Screen.Height;
        int width = _driver.Screen.Width;
        _hudVertical = tree.WithNodeOwner(_hudDividerOwner, () => tree.AllocateNode(new HudDividerNode()));
        dividers.Construct(_hudVertical, 0, 5, height, 0x03400000, fortType, dividerFrame);
        tree.Tree.Attach(_hudVertical, 80, 0, root, 0);
        _hudMinimapDivider = tree.WithNodeOwner(_hudDividerOwner, () => tree.AllocateNode(new HudDividerNode()));
        dividers.Construct(_hudMinimapDivider, 0, 80, 5, 0x02a00000, fortType, dividerFrame);
        tree.Tree.Attach(_hudMinimapDivider, 0, height - 80, root, 0);
        _hudChatDivider = tree.WithNodeOwner(_hudDividerOwner, () => tree.AllocateNode(new HudDividerNode()));
        dividers.Construct(_hudChatDivider, 18, width, 5, 0x02a00000, fortType, 0);
        tree.Tree.Attach(_hudChatDivider, 85, height - 96, root, 0);
        dock = new HudDock(part => part switch
        {
            HudDockPart.Root => tree.Create.Entries[0],
            HudDockPart.Vertical => _hudVertical,
            HudDockPart.MinimapDivider => _hudMinimapDivider,
            HudDockPart.ChatDivider => _hudChatDivider,
            HudDockPart.Chat => _nativeChatWindows?.CurrentRoot ?? 0,
            _ => 0,
        }, globals, () => (sbyte)55, hooks);
        _missionHudRouting = new HostMissionHudRouting(tree, dock);
        dock.RegisterNormalLayout();
        GumpCreateNode vertical = tree.Node(_hudVertical) ?? throw new InvalidOperationException("HUD 세로 구분선 없음");
        GumpCreateNode chat = tree.Node(_hudChatDivider) ?? throw new InvalidOperationException("HUD 채팅 구분선 없음");
        dock.Handle(0x0200003a, vertical.Serial, 0);
        dock.Handle(0x0200003a, chat.Serial, 0);
        _missionHudDock = dock;
    }
}
