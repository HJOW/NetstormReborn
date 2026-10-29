namespace Netstorm.Client.Gump; // 원본 HTML 창의 현재 본문/버튼/메뉴와 화면 제한을 배치한다.

/// <summary>측정/트리/현재 메뉴 스택과 실제 화면 전역의 선택 경계.</summary>
public sealed class HtmlWindowFitHooks
{
    /// <summary>현재 같은 HTML 부모.</summary>
    public Func<uint, HtmlWindowGumpNode?>? Node { get; set; }
    /// <summary>현재 같은 기본/자식 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>실제 돌 버튼 글자 맞춤.</summary>
    public Action<uint>? FitButton { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>실제 다중 줄의 현재 폭에 맞춘 높이 측정.</summary>
    public Action<uint>? FitHeight { get; set; }
    /// <summary>실제 다중 줄의 자연 내용 치수 측정.</summary>
    public Action<uint, int, int>? FitContent { get; set; }
    /// <summary>실제 Slider 상태 변경.</summary>
    public Action<uint, uint>? Slider { get; set; }
    /// <summary>실제 스크롤 허용 상태 변경.</summary>
    public Action<uint, uint>? Scrollable { get; set; }
    /// <summary>실제 절대 스크롤 요청.</summary>
    public Action<uint, int>? RequestScroll { get; set; }
    /// <summary>자식의 실제 가상 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>부모 자신의 실제 기본 부착.</summary>
    public Action<uint, float, float, uint, uint>? AttachBase { get; set; }
    /// <summary>실제 메뉴 스택의 현재 상단 토큰.</summary>
    public Func<uint>? CurrentMenu { get; set; }
    /// <summary>현재 메뉴를 부착하고 스택에서 꺼낸다.</summary>
    public Action<int, int, uint>? AttachMenu { get; set; }
    /// <summary>현재 보조창 serial의 등록표 조회.</summary>
    public Func<uint, uint>? Reference { get; set; }
    /// <summary>현재 원본 주소의 화면/뷰 DWORD.</summary>
    public Func<uint, uint>? Read { get; set; }
    /// <summary>공통 트리의 실제 깊이 덧셈.</summary>
    public Action<uint, uint>? AddDepth { get; set; }
}
/// <summary>cpppj/src/client/HtmlWindowFit.h/.cpp의004c7d40 전체 가상 부착. FST32 저장 위치/현재 재조회 순서를 보존한다.</summary>
public sealed class HtmlWindowFit
{
    /// <summary>호출보다 오래 유지되는 같은 실제 장치.</summary>
    private readonly HtmlWindowFitHooks _hooks;
    /// <summary>도달한 실제 경계만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("HTML 전체 배치 실제 장치 미연결");
    /// <summary>같은 현재 부모/자식 조회만 미리 확인한다.</summary>
    public HtmlWindowFit(HtmlWindowFitHooks hooks) { ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _hooks = hooks; }
    /// <summary>기본과 같은 현재 파생 주소를 요구한다.</summary>
    private HtmlWindowGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("HTML 배치 실제 부모 없음"); return n; }
    /// <summary>현재 본문/버튼/메뉴의 같은 기본 객체.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("HTML 배치 실제 자식 없음");
    /// <summary>현재 주소의 DWORD를 signed로 읽는다.</summary>
    private int Read(uint address) => unchecked((int)Require(_hooks.Read)(address));
    /// <summary>유효20행 밖 원문 물리 입력만 거부한다.</summary>
    private static int Slot(uint index) => index < 20 ? (int)index : throw new IndexOutOfRangeException("HTML 배치 실제20행 밖 접근");
    /// <summary>원문_ftol의64비트 절삭 하위 DWORD.</summary>
    private static int Ftol(double value) => GumpCollection.Ftol(value);
    /// <summary>입력 좌표를 현재 화면/뷰 중앙으로 바꾸되 부모/모드를 보존하고 마지막 깊이를-80으로 맞춘다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        Node(token); float maximum = 0;
        // 각 버튼 측정 뒤 현재 행/개수/폭을 다시 읽고 NaN도 원문 대입 경로로 보낸다.
        for (uint i = 0; unchecked((int)i) < unchecked((int)Node(token).ControlCount); i++) { int slot = Slot(i); Require(_hooks.FitButton)(Node(token).Controls[slot].Child); float width = Base(Node(token).Controls[slot].Child).BaseWidth; if (!(maximum > width)) maximum = width; }
        int count = unchecked((int)Node(token).ControlCount); float span = (float)((double)count * maximum); if (count > 1) span = (float)((double)unchecked(count * 15 - 15) + span);
        Require(_hooks.Push)(Node(token).Body); int requested = unchecked((int)Node(token).RequestedWidth - 40); float target = span; if (!((double)span > requested)) target = requested; Require(_hooks.FitContent)(Node(token).Body, 0, 0); var measured = Base(Node(token).Body);
        if (!(measured.BaseWidth <= target)) {
            int previousHeight = 0, previousWidth = 0;
            // 폭을 float32로20씩 늘리며 높이 정체/뷰 한도/종횡비를 원문 순서로 판정한다.
            for (;;) {
                measured.BaseWidth = target; measured.Width = (float)((double)target * measured.Scale); Require(_hooks.FitHeight)(Node(token).Body); int available = Read(0x5ca9b4);
                if ((double)unchecked(available - 40) < (double)target + 40) { var body = Base(Node(token).Body); int width = unchecked(available - 80); body.BaseWidth = width; body.Width = (float)((double)width * body.Scale); Require(_hooks.FitHeight)(Node(token).Body); break; }
                if (Node(token).RequestedHeight != 0) { var body = Base(Node(token).Body); int height = unchecked((int)Node(token).RequestedHeight - 65); if ((double)height > body.BaseHeight) { body.BaseHeight = height; body.Height = (float)((double)height * body.Scale); } }
                measured = Base(Node(token).Body); if ((double)measured.BaseHeight != previousHeight) { previousHeight = Ftol(measured.BaseHeight); previousWidth = Ftol(target); }
                if ((double)target - previousWidth > 50) { var first = Base(Node(token).Body); first.BaseWidth = previousWidth; first.Width = (float)((double)previousWidth * first.Scale); var second = Base(Node(token).Body); second.BaseHeight = previousHeight; second.Height = (float)((double)previousHeight * second.Scale); break; }
                target = (float)((double)target + 20); if (!((double)measured.BaseWidth * 2 < (double)measured.BaseHeight * 4)) break;
            }
            if ((double)Read(0x531860) < (double)target + 40 || (double)Read(0x531864) < (double)Base(Node(token).Body).BaseHeight + 65) {
                int limit = unchecked(Read(0x531860) - 40); if ((double)target > limit) target = limit; var body = Base(Node(token).Body); body.BaseWidth = target; body.Width = (float)((double)target * body.Scale); var second = Base(Node(token).Body); double height = (double)target * .5; second.BaseHeight = (float)height; second.Height = (float)(height * second.Scale);
                if (Node(token).RequestedHeight != 0) { var current = Base(Node(token).Body); int requestedHeight = unchecked((int)Node(token).RequestedHeight - 65); if ((double)requestedHeight > current.BaseHeight) { current.BaseHeight = requestedHeight; current.Height = (float)((double)requestedHeight * current.Scale); } }
                Require(_hooks.Slider)(Node(token).Body, 1); Require(_hooks.Scrollable)(Node(token).Body, 1); Require(_hooks.RequestScroll)(Node(token).Body, 0);
            }
        } else Require(_hooks.FitHeight)(Node(token).Body);
        var root = Node(token); var bodyNode = Base(root.Body); double fullWidth = (double)target + 40; root.BaseWidth = (float)fullWidth; root.Width = (float)(fullWidth * root.Scale); double fullHeight = (double)bodyNode.BaseHeight + 65; root.BaseHeight = (float)fullHeight; root.Height = (float)(fullHeight * root.Scale); int halfWidth = Ftol(fullWidth * .5), buttonY = Ftol((double)bodyNode.BaseHeight + 30);
        if (Node(token).MenuColumns != 0) {
            uint firstToken = Require(_hooks.CurrentMenu)(); var first = Base(firstToken); int menuHalf = Ftol((double)first.BaseWidth * .5), menuHeight = Ftol(first.BaseHeight), menuX = unchecked(halfWidth - menuHalf); if (menuX < 20) menuX = 20;
            if (unchecked((int)Node(token).MenuColumns) > 1) { menuX = unchecked(menuX - menuHalf / 2); Require(_hooks.AttachMenu)(menuX, buttonY, token); float oldHeight = menuHeight; uint currentMenu = Require(_hooks.CurrentMenu)(); if (oldHeight > Base(currentMenu).BaseHeight) menuHeight = Ftol(oldHeight); else menuHeight = Ftol(Base(Require(_hooks.CurrentMenu)()).BaseHeight); menuX = unchecked(menuX + menuHalf + menuHalf); }
            Require(_hooks.AttachMenu)(menuX, buttonY, token); var current = Node(token); double height = (double)menuHeight + current.BaseHeight + 10; current.BaseHeight = (float)height; current.Height = (float)(height * current.Scale); int width = unchecked((int)(Node(token).MenuColumns * (uint)menuHalf * 2 + 40)); if ((double)width > Node(token).BaseWidth) { var node = Node(token); node.BaseWidth = width; node.Width = (float)((double)width * node.Scale); } buttonY = Ftol((double)buttonY + first.BaseHeight + 10); Node(token).MenuColumns = 0;
        }
        int buttonX = Ftol(((double)Node(token).BaseWidth - span) * .5); float yPosition = buttonY;
        // Push/Attach 효과 뒤 현재 행/개수를 재조회하면서 동일 최대 폭을 적용한다.
        for (uint i = 0; unchecked((int)i) < unchecked((int)Node(token).ControlCount); i++) { int slot = Slot(i); Require(_hooks.Push)(Node(token).Controls[slot].Child); var button = Base(Node(token).Controls[slot].Child); button.BaseWidth = maximum; button.Width = (float)((double)maximum * button.Scale); float xPosition = buttonX; Require(_hooks.Attach)(Node(token).Controls[slot].Child, xPosition, yPosition, token, 0); buttonX = Ftol((double)xPosition + maximum + 15); }
        bodyNode = Base(Node(token).Body); int bodyX = Ftol((double)Node(token).BaseWidth - bodyNode.BaseWidth) / 2; Require(_hooks.Attach)(Node(token).Body, bodyX, 20, token, 0);
        if (Require(_hooks.Reference)(Node(token).AuxiliarySerial) != 0) { x = (float)(((double)Read(0x5c78f0) - Node(token).BaseWidth) * .5); y = (float)(((double)Read(0x5c78f4) - Node(token).BaseHeight) * .5); }
        else { int left = Read(0x5ca9cc), right = Read(0x5ca9d4), bottom = Read(0x5ca9d8), top = Read(0x5ca9d0); x = (float)(((double)unchecked(right - left) - Node(token).BaseWidth) * .5 + left); y = (float)(((double)unchecked(bottom - top) - Node(token).BaseHeight) * .5 + Read(0x5ca9d0)); }
        Require(_hooks.AttachBase)(token, x, y, parent, mode); Require(_hooks.AddDepth)(token, unchecked(0xffffffb0 - Node(token).Depth));
    }
    /// <summary>같은 실제 본문/돌 버튼/기본 트리 장치를 연결한다. 현재 메뉴/전역/가상 부착은 상위가 공급한다.</summary>
    public static void BindHtmlWindowFitDevices(HtmlWindowFitHooks hooks, StoneButtonGump buttons, GumpTree tree, ScrollingTextGump text, ScrollingTextGumpWidgets widgets) { hooks.FitButton = buttons.FitToText; hooks.Push = tree.Push; hooks.FitHeight = text.FitHeight; hooks.FitContent = text.FitContent; hooks.Slider = widgets.SetSlider; hooks.Scrollable = widgets.SetScrollable; hooks.RequestScroll = text.RequestScroll; hooks.AttachBase = tree.Attach; hooks.AddDepth = tree.AddDepth; }
    /// <summary>실제 controlled 메뉴 스택의 현재 조회/부착·pop을 연결한다.</summary>
    public static void BindHtmlWindowFitMenus(HtmlWindowFitHooks hooks, MenuGump menu) { hooks.CurrentMenu = menu.Current; hooks.AttachMenu = menu.AttachCurrent; }
    /// <summary>현재 HTML 창만 전체 가상 부착으로 분배하고 다른 파생은 보존한다.</summary>
    public static void BindHtmlWindowFit(GumpTreeHooks hooks, Func<uint, HtmlWindowFit?> resolve) { var previous = hooks.Attach; hooks.Attach = (t, x, y, parent, mode) => { if (resolve(t) is { } fit) fit.Attach(t, x, y, parent, mode); else Require(previous)(t, x, y, parent, mode); }; }
}
