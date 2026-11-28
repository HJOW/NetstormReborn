// 같은 실제 트리/메뉴에 전체 HUD 부모 공장을 묶는다.
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>Uber 루트 생성에 쓰는 네 자식 공장이다.</summary>
public sealed class HostUberRootFactories
{
    /// <summary>실제 미니맵 전체 공장이다.</summary>
    public Func<uint>? CreateMinimap { get; set; }
    /// <summary>실제 생산 패널 전체 공장이다.</summary>
    public Func<uint>? CreateProduction { get; set; }
    /// <summary>실제 지식 패널 전체 공장이다.</summary>
    public Func<uint>? CreateKnowledge { get; set; }
    /// <summary>실제 채팅 창 전체 공장이다.</summary>
    public Func<uint>? CreateChat { get; set; }
}

/// <summary>Uber 루트 생성에 쓰는 실제 외부 장치다. 트리에서 빌리는 파생·부착·수명은 제외한다.</summary>
public sealed class HostUberRootDevices
{
    /// <summary>현재 실제 마우스 폴링이다.</summary>
    public Func<uint, InputEvent>? Poll { get; set; }
    /// <summary>실제 커서 장치다.</summary>
    public Action<uint>? Cursor { get; set; }
    /// <summary>같은 메뉴 처리기의 기본 Help다.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>같은 메뉴 처리기의 실제 popup 정리다.</summary>
    public Action? ClosePopups { get; set; }
    /// <summary>같은 실제 UI dirty 소유자 통지다.</summary>
    public Action? Changed { get; set; }
    /// <summary>공통 등록0 교체 뒤 호스트의 현재 루트 별칭 반납이다.</summary>
    public Action<uint>? Replaced { get; set; }
    /// <summary>기존 실제 배경 GIF 공장의 Allocate. 메시지 선택 때만 필요하다.</summary>
    public Func<uint>? AllocateMessage { get; set; }
    /// <summary>종류 BYTE/화면/TYPE/프레임/메시지/명령을 실제 소비 위치에서 읽는다.</summary>
    public Func<uint, uint>? Read { get; set; }
    /// <summary>배경/프레임 전역 세 곳의 실제 쓰기다.</summary>
    public Action<uint, uint>? Write { get; set; }
    /// <summary>0049a840의 현재 TYPE 조회다.</summary>
    public Func<uint, uint>? Type { get; set; }
    /// <summary>Chat Window 번역 포인터다.</summary>
    public Func<string, uint>? Translate { get; set; }
    /// <summary>00464ed0의 같은 번역 포인터 보관이다.</summary>
    public Action<uint, uint>? Help { get; set; }
    /// <summary>00464f80의 chatView 도움말 키 보관이다.</summary>
    public Action<uint, string>? HelpWord { get; set; }
    /// <summary>00565e70 현재 C문자열의 같은 원본 포인터 토큰이다.</summary>
    public Func<uint>? SavedMessage { get; set; }
    /// <summary>현재 C문자열의 대소문자 무시 비교다.</summary>
    public Func<uint, string, bool>? EqualText { get; set; }
    /// <summary>0041a030의 현재 이름/배경 GIF 전체 창 생성이다.</summary>
    public Action<uint, uint>? ConstructMessage { get; set; }
    /// <summary>459c60의 원문9DWORD 인자로 실제 조건 표현식을 작성한다.</summary>
    public Func<uint[], uint>? FormatMessage { get; set; }
    /// <summary>41ab10의 완성 표현식 검사다.</summary>
    public Func<uint, uint>? TestMessage { get; set; }
    /// <summary>4b6100의 실제 현재 메시지 방문 기록이다.</summary>
    public Action<uint>? VisitMessage { get; set; }
    /// <summary>419e10의 실제 GIF 사건/인자다.</summary>
    public Action<uint, uint, uint, uint>? MessageEvent { get; set; }
    /// <summary>4437f0의 실제1BYTE RET다. 선택적 호출 관측만 제공한다.</summary>
    public Action? EmptyObserver { get; set; }
    /// <summary>42f540의 실제 기술 표시 갱신이다.</summary>
    public Action? RefreshTechnology { get; set; }
    /// <summary>실제 dirty 4좌표·플래그다.</summary>
    public Action<int, int, int, int, uint>? DockInvalidate { get; set; }
    /// <summary>실제 화면 후처리다.</summary>
    public Action? DockFinishResize { get; set; }
    /// <summary>실제 채팅 표시 설정이다.</summary>
    public Action<bool>? DockChatVisible { get; set; }
    /// <summary>실제 채팅 입력 설정이다.</summary>
    public Action<bool>? DockChatInput { get; set; }
    /// <summary>도킹 기본 입력이다.</summary>
    public Func<uint, uint, uint, uint>? DockBaseHandle { get; set; }
}

/// <summary>같은 실제 트리/메뉴/지도 owner에 전체 HUD 부모 공장을 조립한다.</summary>
/// <remarks>cpppj UberHud.inc의 EnsureHudFactories가 묶는 네 공장·메시지·전역 결선의 DN 진입점이다. 생성·교체·배치는 호출자가 `RawUberRootControls`로 수행한다. 2026-11-26 1.074에서 R5-4a용으로 추가했다.</remarks>
public static class HostUberRoot
{
    /// <summary>현재 등록0의 실제 전체 HUD를 검증한다. 일반 메뉴 루트는 null이며, 손상되거나 다른 소유자의 Native 루트는 재생성으로 숨기지 않는다.</summary>
    public static UberRootNode? CurrentRoot(GumpButtonTree tree, RawUberRootControls? controls)
    {
        ArgumentNullException.ThrowIfNull(tree);
        uint token = tree.Create.Entries[0];
        var current = tree.Node(token) ?? throw new InvalidOperationException("HUD 재사용의 현재 공통 루트 없음");
        if (current is not UberRootNode root) return null;
        if (controls is null || !ReferenceEquals(controls.Node(token), root) || !controls.Current())
            throw new InvalidOperationException("HUD 재사용의 실제 전체 부모/자식 수명 불일치");
        // 같은 전체 공장이 소유한 세 구분선만 허용하며 다른 생성기의 같은 종류로 대신하지 않는다.
        foreach (uint child in new[] { root.Vertical, root.MinimapDivider, root.ChatDivider })
        {
            var node = RequireChild(tree, root, child, child, controls);
            if (node is not HudDividerNode || !ReferenceEquals(controls.Node(child), node))
                throw new InvalidOperationException("HUD 재사용의 실제 구분선 소유자 불일치");
        }
        return root;
    }

    /// <summary>현재 부모의 자식 토큰과 실제 공장 현재값/소유자/부착/생존을 함께 확인하고 같은 노드를 빌린다.</summary>
    public static GumpCreateNode RequireChild(GumpButtonTree tree, UberRootNode root, uint token, uint current, object owner)
    {
        ArgumentNullException.ThrowIfNull(tree);
        ArgumentNullException.ThrowIfNull(root);
        ArgumentNullException.ThrowIfNull(owner);
        uint parent = tree.Create.Entries[0];
        var node = tree.Node(token);
        if (!ReferenceEquals(tree.Node(parent), root) || token == 0 || token != current || node is null
            || !ReferenceEquals(tree.NodeOwner(token), owner) || node.Parent != parent || (node.Flags & 0x10000) != 0)
            throw new InvalidOperationException("HUD 재사용의 현재 자식/공장/부모 불일치");
        return node;
    }

    /// <summary>같은 실제 트리/메뉴/지도 owner와 외부 장치로 전체 부모 조립기를 만든다.</summary>
    /// <param name="tree">같은 실제 노드/자산 소유자다.</param>
    /// <param name="menus">현재 메뉴 인스턴스 owner다.</param>
    /// <param name="globals">같은 실제 화면/지도 전역이다.</param>
    /// <param name="factories">네 자식 공장이다.</param>
    /// <param name="devices">실제 외부 장치다.</param>
    /// <returns>생성·교체·배치를 수행하는 조립기다.</returns>
    public static RawUberRootControls Assemble(GumpButtonTree tree, MenuGumpState menus, HudDockGlobals globals, HostUberRootFactories factories, HostUberRootDevices devices)
    {
        ArgumentNullException.ThrowIfNull(tree);
        ArgumentNullException.ThrowIfNull(menus);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(factories);
        ArgumentNullException.ThrowIfNull(devices);
        var root = new UberRootHooks
        {
            Read = devices.Read,
            Write = devices.Write,
            Type = devices.Type,
            Translate = devices.Translate,
            Help = devices.Help,
            HelpWord = devices.HelpWord,
            SavedMessage = devices.SavedMessage,
            EqualText = devices.EqualText,
            ConstructMessage = devices.ConstructMessage,
            FormatMessage = devices.FormatMessage,
            TestMessage = devices.TestMessage,
            VisitMessage = devices.VisitMessage,
            MessageEvent = devices.MessageEvent,
            EmptyObserver = devices.EmptyObserver,
            RefreshTechnology = devices.RefreshTechnology,
            CreateMinimap = factories.CreateMinimap,
            CreateProduction = factories.CreateProduction,
            CreateKnowledge = factories.CreateKnowledge,
            CreateChat = factories.CreateChat,
        };
        var dock = new HudDockHooks
        {
            ClosePopups = devices.ClosePopups,
            Invalidate = devices.DockInvalidate,
            FinishResize = devices.DockFinishResize,
            ChatVisible = devices.DockChatVisible,
            ChatInput = devices.DockChatInput,
            BaseHandle = devices.DockBaseHandle,
        };
        var context = new RawUberRootContext
        {
            Globals = globals,
            Root = root,
            Dock = dock,
            Poll = devices.Poll ?? throw new ArgumentException("Uber 루트 실제 폴링 없음", nameof(devices)),
            Cursor = devices.Cursor ?? throw new ArgumentException("Uber 루트 실제 커서 없음", nameof(devices)),
            BaseHandle = devices.BaseHandle ?? throw new ArgumentException("Uber 루트 기본 입력 없음", nameof(devices)),
            ClosePopups = devices.ClosePopups ?? throw new ArgumentException("Uber 루트 팝업 정리 없음", nameof(devices)),
            Replaced = devices.Replaced,
            Changed = devices.Changed ?? throw new ArgumentException("Uber 루트 변경 통지 없음", nameof(devices)),
            AllocateMessage = devices.AllocateMessage,
        };
        return new RawUberRootControls(tree, menus, context);
    }
}
