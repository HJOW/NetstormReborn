using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host;

// RU-4b 같은 지도 owner의 실제 HUD 작성자로 지도 저장소를 작성한다. 0.993에서 추가됐다.
public sealed partial class ClientHostGame
{
    /// <summary>같은 실제 메뉴 트리/화면과 FullDeck/MapRevision을 빌리는 HUD 전역이다. 미션보다 오래 산다.</summary>
    private HudDockGlobals? _hudDockGlobals;
    /// <summary>같은 실제 메뉴 트리에 묶은 HUD 도킹 본체다. 토큰은 실제 전체 부모에서 읽는다.</summary>
    private HudDock? _hudDock;

    /// <summary>같은 실제 공통 트리/현재 Deck/MapRevision owner를 빌리는 HUD 전역이다.</summary>
    public HudDockGlobals NativeHudDockGlobals => _hudDockGlobals ?? throw new InvalidOperationException("실제 HUD 도킹 전역 미설치");
    /// <summary>같은 지도 저장소를 작성하는 실제 HUD 도킹 본체다.</summary>
    public HudDock NativeHudDock => _hudDock ?? throw new InvalidOperationException("실제 HUD 도킹 작성자 미설치");

    /// <summary>같은 실제 메뉴/트리 저장소로 HUD 전역과 도킹 본체를 만들고 현재 지도 getter와 같은 owner를 공유한다.</summary>
    /// <remarks>2026-11-25 0.993에서 RU-4b용으로 추가했다. 근거: cpppj HudDock.cpp의 같은 실제 부모 필드 재조회와 UberHud.inc 20행의 같은 MapViewRect/MapRevision owner 공유. 구분선/전체 부모 생성과 선택 수명은 RU-3 나머지·RU-4c에서 잇는다.</remarks>
    private void InitializeNativeHudDock()
    {
        if (_hudDockGlobals is not null || _hudDock is not null) throw new InvalidOperationException("HUD 도킹 작성자 중복 설치");
        var menu = _mainMenu ?? throw new InvalidOperationException("HUD 도킹의 실제 공통 트리 없음");
        var globals = new HudDockGlobals(menu.Tree.Create, menu.Tree.TreeState, _gameplay.FullDeckBox, _gameplay.MapRevision);
        // FullDeckBox와 MapRevision은 생성자에 같은 owner를 직접 넘기므로 사본이나 기본 viewport를 만들지 않는다. 동일 화면/지도 owner 검사는 RawUberRootControls의 BorrowsView와 같은 조건이며 여기서는 같은 참조를 직접 넘긴다.
        var hooks = new HudDockHooks
        {
            Node = menu.Tree.Node,
            Divider = token => menu.Tree.Node(token) as HudDividerNode,
            BaseAttach = menu.Tree.Tree.Attach,
            Push = menu.Tree.Tree.Push,
            Attach = (token, x, y, parent, mode) => (menu.Tree.TreeHooks.Attach ?? throw new InvalidOperationException("HUD 실제 파생 부착 미설치"))(token, x, y, parent, mode),
            Move = menu.Tree.Tree.Move,
            ResolveSerial = menu.Tree.Reference,
            Bubble = (token, code, first, second) => menu.Tree.Events.Bubble(token, code, first, second),
            ClosePopups = () => { },
            Invalidate = (_, _, _, _, _) => _menuDirty = true,
            FinishResize = () => throw new InvalidOperationException("HUD 화면 후처리는 R5 표시 연결에서 잇는다"),
            ChatVisible = _ => throw new InvalidOperationException("HUD 채팅 표시는 RU-5 Chat 명령 효과에서 잇는다"),
            ChatInput = _ => throw new InvalidOperationException("HUD 채팅 입력는 RU-5 Chat 명령 효과에서 잇는다"),
            BaseHandle = (_, _, _) => throw new InvalidOperationException("HUD 기본 입력은 R5 입력 연결에서 잇는다"),
        };
        Func<HudDockPart, uint> tokens = part =>
        {
            uint root = menu.Tree.Create.Entries[0];
            if (menu.Tree.Node(root) is not UberRootNode node) throw new InvalidOperationException("실제 HUD 부모 미설치(RU-3 나머지에서 잇는다)");
            if (part == HudDockPart.Root) return root;
            return part switch
            {
                HudDockPart.Vertical => node.Vertical,
                HudDockPart.MinimapDivider => node.MinimapDivider,
                HudDockPart.ChatDivider => node.ChatDivider,
                HudDockPart.Minimap => node.Minimap,
                HudDockPart.Production => node.Production,
                HudDockPart.Chat => node.Chat,
                HudDockPart.Knowledge => node.Knowledge,
                _ => throw new ArgumentOutOfRangeException(nameof(part), "Uber 현재 HUD 필드 종류 오류"),
            };
        };
        _hudDockGlobals = globals;
        _hudDock = new HudDock(tokens, globals, () => 55, hooks);
    }
}
