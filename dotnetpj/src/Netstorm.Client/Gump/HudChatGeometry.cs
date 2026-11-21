using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 같은 네 자식의 실제 크기/표시와 원문 가상 호출 순서를 복원한다.
namespace Netstorm.Client.Gump;

/// <summary>원본0x13cBYTE ChatWindow의 같은 기본 Gump와+a0..138 상태다.</summary>
public sealed class HudChatNode : GumpCreateNode
{
    /// <summary>+a0의 같은 실제 Console 자식이다.</summary>
    public uint Console { get; set; }
    /// <summary>+a4의 같은 실제 ScrollingText 자식이다.</summary>
    public uint Text { get; set; }
    /// <summary>+a8의 같은 실제 helper 자식이다.</summary>
    public uint Helper { get; set; }
    /// <summary>+ac의 같은 실제 배경 자식이다.</summary>
    public uint Background { get; set; }
    /// <summary>+b0의 생성자 입력 버퍼 크기128이다.</summary>
    public uint Capacity { get; set; }
    /// <summary>+b4의 매부착 현재 signed 여백이다.</summary>
    public int LeftPadding { get; set; }
    /// <summary>+b8..137의 실제128BYTE 접두다.</summary>
    public byte[] InputPrefix { get; } = new byte[128];
    /// <summary>+138의 helper 높이15를 허용하는 원시BYTE다.</summary>
    public byte HelperVisible { get; set; }
}
/// <summary>같은 실제 노드와 공통 트리/Console/로그의 선택 장치다.</summary>
public sealed class HudChatGeometryHooks
{
    /// <summary>같은 실제 root 파생 노드다.</summary>
    public Func<uint, HudChatNode?>? Chat { get; init; }
    /// <summary>root와 자식이 공유하는 실제 기본 주소다.</summary>
    public Func<uint, GumpCreateNode?>? Node { get; init; }
    /// <summary>00466750의 공통 트리 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; init; }
    /// <summary>현재 자식의 실제 가상+18이다.</summary>
    public Action<uint>? Push { get; init; }
    /// <summary>현재 대상의 실제 가상+20이다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; init; }
    /// <summary>기존 본문 높이에서 현재 로그 위치를 조회한다.</summary>
    public Func<uint, uint>? BottomIndex { get; init; }
    /// <summary>같은 로그의 현재 맨 아래 요청이다.</summary>
    public Action<uint, uint>? RequestBottom { get; init; }
    /// <summary>입력 표시 분기에서만 읽는 현재 메타 모드다.</summary>
    public Func<uint>? Challenge { get; init; }
    /// <summary>Push 뒤에도 다시 읽는00548740 현재 root다.</summary>
    public Func<uint>? Current { get; init; }
}
/// <summary>HudChatGeometry.cpp의 같은 노드/배율/재조회와 선택 경계다.</summary>
public sealed class HudChatGeometry
{
    /// <summary>표시 여부에 사용하는 Console flags 하위bit다.</summary>
    private const uint Visible = 1;
    /// <summary>보호 PE의 실제 helper 높이다.</summary>
    private const double HelperHeight = 15;
    /// <summary>보호 PE의 실제 Console 왼쪽 추가 여백이다.</summary>
    private const double ConsoleLeft = 17;
    /// <summary>binding 때 고정한 실제 공통/자식 장치다.</summary>
    private readonly HudChatGeometryHooks _hooks;
    /// <summary>현재 조회 두장치만 검증하며 자식/트리/텍스트는 실제 선택 위치까지 요구하지 않는다.</summary>
    public HudChatGeometry(HudChatGeometryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks); if (hooks.Chat is null || hooks.Node is null) throw new ArgumentException("ChatWindow 같은 노드 저장소 필요", nameof(hooks)); _hooks = hooks;
    }
    /// <summary>미선택 하위를 선행 요구하지 않는다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("ChatWindow 기하의 실제 선택 장치 미연결");
    /// <summary>현재 root 파생/기본 저장소가 같은 실제 객체인지 확인한다.</summary>
    private HudChatNode Chat(uint token)
    {
        var node = token != 0 ? _hooks.Chat!(token) : null; if (node is null || !ReferenceEquals(node, _hooks.Node!(token))) throw new InvalidOperationException("ChatWindow 공유 root 없음"); return node;
    }
    /// <summary>실제 사용한 자식만 현재 공통 저장소에서 조회한다.</summary>
    private GumpCreateNode Node(uint token) => (token != 0 ? _hooks.Node!(token) : null) ?? throw new InvalidOperationException("ChatWindow 실제 자식 없음");
    /// <summary>단정도 저장 전 중간값으로 배율을 곱해 원문 x87 크기를 유지한다.</summary>
    private void Size(uint token, double width, double height)
    {
        var node = Node(token); node.BaseWidth = (float)width; node.Width = (float)(width * node.Scale); node.BaseHeight = (float)height; node.Height = (float)(height * node.Scale);
    }
    /// <summary>같은 현재 자식의 가상 Push만 실행한다.</summary>
    private void Push(uint token) { Node(token); Need(_hooks.Push)(token); }
    /// <summary>현재 가상 Attach에 원문 parent와 mode0을 전달한다.</summary>
    private void AttachVirtual(uint token, float x, float y, uint parent) { Node(token); Need(_hooks.Attach)(token, x, y, parent, 0); }
    /// <summary>부모 부착 뒤 배경→로그→helper→Console을 현재 필드로 다시 읽어 부착한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        var root = Chat(token); Need(_hooks.BaseAttach)(token, x, y, parent, mode); Push(root.Background);
        double backgroundHeight = (double)root.BaseHeight - ((Node(root.Console).Flags & Visible) != 0 ? Node(root.Console).BaseHeight : 0.0); Size(root.Background, (double)root.BaseWidth - root.LeftPadding, backgroundHeight);
        var helper = Node(root.Helper); bool showHelper = root.HelperVisible != 0 && (Node(root.Console).Flags & Visible) != 0; helper.BaseHeight = showHelper ? (float)HelperHeight : 0; helper.Height = (float)((showHelper ? HelperHeight : 0) * helper.Scale);
        AttachVirtual(root.Background, root.LeftPadding, 0, token); uint bottom = Need(_hooks.BottomIndex)(root.Text); Push(root.Text); Size(root.Text, Node(root.Background).BaseWidth, Node(root.Background).BaseHeight); Need(_hooks.RequestBottom)(root.Text, bottom); AttachVirtual(root.Text, 0, 0, root.Background);
        Push(root.Helper); AttachVirtual(root.Helper, root.LeftPadding, (float)((double)root.BaseHeight - Node(root.Helper).BaseHeight), token);
        Push(root.Console); var console = Node(root.Console); double consoleWidth = Node(root.Text).BaseWidth; console.BaseWidth = (float)consoleWidth; console.Width = (float)(consoleWidth * console.Scale); AttachVirtual(root.Console, (float)((double)root.LeftPadding + ConsoleLeft), (float)((double)root.BaseHeight - Node(root.Console).BaseHeight), token);
    }
    /// <summary>비영 값을BYTE1로만 바꾸며 다음 실제 Attach가 높이를 소비한다.</summary>
    public void SetHelperVisible(uint token, uint value) => Chat(token).HelperVisible = value != 0 ? (byte)1 : (byte)0;
    /// <summary>메타에서는0요청도 표시하고 현재 Console/root를 각 callback 뒤 다시 부착한다.</summary>
    public void SetInputVisible(uint token, uint value)
    {
        if (token == 0) return; var root = Chat(token); Push(token); Push(root.Console); if (value != 0 || Need(_hooks.Challenge)() != 0) Node(root.Console).Flags |= Visible; else Node(root.Console).Flags &= ~Visible;
        uint console = root.Console; AttachVirtual(console, Node(console).X, Node(console).Y, 0); AttachVirtual(token, root.X, root.Y, 0);
    }
    /// <summary>Console만 Push한 뒤 현재 root를 다시 읽고 같은 자식만 표시 반전/부착한다.</summary>
    public void ToggleInputVisible()
    {
        uint before = Need(_hooks.Current)(); if (before == 0) return; Push(Chat(before).Console); var root = Chat(Need(_hooks.Current)()); Node(root.Console).Flags ^= Visible; uint console = root.Console; AttachVirtual(console, Node(console).X, Node(console).Y, 0);
    }
    /// <summary>현재 root0에서는 자식을 조회하지 않고 그 외 Console의 표시bit만 반환한다.</summary>
    public uint InputVisible() { uint current = Need(_hooks.Current)(); return current != 0 ? Node(Chat(current).Console).Flags & Visible : 0; }
    /// <summary>일반 입력의 문자 Enter만 비표시 입력줄 표시→부모 열기 순서로 소비한다.</summary>
    public static bool DispatchChatInputKey(InputEvent input, Func<uint>? visible, Action? show, Action? open)
    {
        if ((input.Code & InputCode.Release) != 0 || (input.Code & 0x03ffffff) != 13) return false; if (Need(visible)() != 0) return false; Action showEffect = Need(show), openEffect = Need(open); showEffect(); openEffect(); return true;
    }
    /// <summary>프레임말 같은 실제 요청을 한 번 소비하고 callback 정상 복귀 뒤0으로 쓴다.</summary>
    public static void ConsumeChatInputToggle(IntBox request, Action? toggle)
    {
        if (request.Value == 0) return; Need(toggle)(); request.Value = 0;
    }
}
