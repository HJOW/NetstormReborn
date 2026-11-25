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
    /// <summary>미션 구분선 노드의 소유자 표시다.</summary>
    private readonly object _hudDividerOwner = new();

    /// <summary>미션 진입에 HUD 구분선 세 개를 만들고 도크 배치로 실제 지도 영역을 정한다.</summary>
    /// <remarks>2026-11-26 1.075에서 R5-4b용으로 추가했다. 근거: cpppj HudDock.cpp의 kMoved 배치와 UberRootGump.Construct의 구분선 기본 배치다. 패널 생성·kHudMode·메뉴 전환은 R5-4c·R5-4d에서 잇는다. 구분선 텍스처 프레임은 0으로 두며 L4a에서 맞춘다.</remarks>
    private void EnsureMissionHudDividers()
    {
        if (_hudVertical != 0) return;
        var menu = _mainMenu ?? throw new InvalidOperationException("HUD 구분선의 실제 공통 트리 없음");
        var tree = menu.Tree;
        var globals = _hudDockGlobals ?? throw new InvalidOperationException("HUD 구분선의 실제 지도 전역 없음");
        uint fortType = menu.BackgroundType;
        var hooks = new HudDockHooks
        {
            Node = tree.Node,
            Divider = token => tree.Node(token) as HudDividerNode,
            ClosePopups = () => { },
            BaseAttach = tree.Tree.Attach,
            Push = tree.Tree.Push,
            Attach = tree.Tree.Attach,
            Move = tree.Tree.Move,
            ResolveSerial = tree.Reference,
            Bubble = (token, code, first, second) => tree.Events.Bubble(token, code, first, second),
            ConstructBackground = (token, width, height, flags, type, frame) => tree.Backgrounds.Construct(token, width, height, flags, 1, type, frame),
            Activate = (token, enabled) => tree.Lifetime.SetOverlay(token, enabled != 0),
            Invalidate = (_, _, _, _, _) => _menuDirty = true,
            FinishResize = () => { NativeRenderer.BackgroundRedraw.Value = 1; _menuDirty = true; },
            ChatVisible = _ => throw new InvalidOperationException("HUD 채팅 표시는 R5-4c에서 잇는다"),
            ChatInput = _ => throw new InvalidOperationException("HUD 채팅 입력은 R5-4c에서 잇는다"),
            BaseHandle = (_, _, _) => throw new InvalidOperationException("HUD 기본 입력은 R5 입력 연결에서 잇는다"),
        };
        var dividers = new HudDividerGump(() => tree.Create.ScreenWidth, () => tree.Create.ScreenHeight, () => (sbyte)55, hooks);
        uint root = tree.Create.Entries[0];
        int height = _driver.Screen.Height;
        int width = _driver.Screen.Width;
        _hudVertical = tree.WithNodeOwner(_hudDividerOwner, () => tree.AllocateNode(new HudDividerNode()));
        dividers.Construct(_hudVertical, 0, 5, height, 0x03400000, fortType, 0);
        tree.Tree.Attach(_hudVertical, 80, 0, root, 0);
        _hudMinimapDivider = tree.WithNodeOwner(_hudDividerOwner, () => tree.AllocateNode(new HudDividerNode()));
        dividers.Construct(_hudMinimapDivider, 0, 80, 5, 0x02a00000, fortType, 0);
        tree.Tree.Attach(_hudMinimapDivider, 0, height - 80, root, 0);
        _hudChatDivider = tree.WithNodeOwner(_hudDividerOwner, () => tree.AllocateNode(new HudDividerNode()));
        dividers.Construct(_hudChatDivider, 18, width, 5, 0x02a00000, fortType, 0);
        tree.Tree.Attach(_hudChatDivider, 85, height - 96, root, 0);
        var dock = new HudDock(part => part switch
        {
            HudDockPart.Vertical => _hudVertical,
            HudDockPart.MinimapDivider => _hudMinimapDivider,
            HudDockPart.ChatDivider => _hudChatDivider,
            _ => 0,
        }, globals, () => (sbyte)55, hooks);
        dock.RegisterNormalLayout();
        GumpCreateNode vertical = tree.Node(_hudVertical) ?? throw new InvalidOperationException("HUD 세로 구분선 없음");
        GumpCreateNode chat = tree.Node(_hudChatDivider) ?? throw new InvalidOperationException("HUD 채팅 구분선 없음");
        dock.Handle(0x0200003a, vertical.Serial, 0);
        dock.Handle(0x0200003a, chat.Serial, 0);
        _missionHudDock = dock;
    }
}
