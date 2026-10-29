using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 메뉴의 배경·강조·구분선·체크/자식 표시·글자 출력 영역이다.

/// <summary>원본 초기화가 공급하는 현재 메뉴 색/구분선 배경/체크 타입 전역.</summary>
public sealed class MenuGumpDrawState
{
    /// <summary>일반 항목 전경색.</summary>
    public uint Foreground { get; set; }
    /// <summary>일반 비활성 항목 색.</summary>
    public uint Disabled { get; set; }
    /// <summary>구분선의 현재 배경 타입.</summary>
    public uint BackgroundType { get; set; }
    /// <summary>구분선의 현재 배경 프레임.</summary>
    public uint BackgroundFrame { get; set; }
    /// <summary>현재 체크 표시 타입.</summary>
    public uint MarkerType { get; set; }
}
/// <summary>같은 실제 기본/메뉴와 원색 체크 SHP 출력 경계.</summary>
public sealed class MenuGumpDrawHooks
{
    /// <summary>현재 실제 메뉴.</summary>
    public Func<uint, MenuGumpNode?>? Node { get; set; }
    /// <summary>같은 실제 기본 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 체크 타입과 화면 기준점의 원색 출력.</summary>
    public Action<uint, ScreenPoint>? Marker { get; set; }
}
/// <summary>cpppj/src/client/MenuGumpDraw.cpp의 현재 필드 재조회·float 저장·배경/표시/문자 순서와 실제 클립 복구.</summary>
public sealed class MenuGumpDraw
{
    /// <summary>현재 판본/등록 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>같은 실제 논리/VFX 클립.</summary>
    private readonly GumpDrawState _clip;
    /// <summary>현재 색/배경/체크 전역.</summary>
    private readonly MenuGumpDrawState _state;
    /// <summary>같은 실제 위치/경계.</summary>
    private readonly GumpCollection _collection;
    /// <summary>같은 실제 배경 출력.</summary>
    private readonly GumpBackgroundDraw _background;
    /// <summary>같은 실제 글리프 출력.</summary>
    private readonly GumpTextDraw _text;
    /// <summary>같은 실제 소유 조각 출력.</summary>
    private readonly StyleText _style;
    /// <summary>같은 현재 항목 측정.</summary>
    private readonly MenuGumpLayout _layout;
    /// <summary>현재 공유 노드/체크 경계.</summary>
    private readonly MenuGumpDrawHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("메뉴 Draw 경계 필요");
    /// <summary>같은 실제 기본/배경/글꼴/소유 조각/측정 처리기를 빌린다.</summary>
    public MenuGumpDraw(GumpCreateState create, GumpDrawState clip, MenuGumpDrawState state, GumpCollection collection, GumpBackgroundDraw background, GumpTextDraw text, StyleText style, MenuGumpLayout layout, MenuGumpDrawHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(clip); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(background); ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(layout); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _create = create; _clip = clip; _state = state; _collection = collection; _background = background; _text = text; _style = style; _layout = layout; _hooks = hooks; }
    /// <summary>기본/메뉴가 같은 실제 객체여야 한다.</summary>
    private MenuGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("메뉴 Draw 공유 노드 없음"); return n; }
    /// <summary>콜백 뒤 바뀐 개수도 다음 반복에서 읽되 실제40칸 범위를 확인한다.</summary>
    private MenuGumpItem Item(uint token, int index) => index >= 0 && index < MenuGump.ItemCapacity ? Node(token).Items[index] : throw new ArgumentOutOfRangeException(nameof(index));
    /// <summary>명시 float32 로컬 rect에 Position을 더해 CRT 절삭한다.패치만 bottom Y를 float32로 먼저 저장한다.</summary>
    private ScreenRect Rect(uint token, float left, float top, float right, float bottom)
    { var point = _collection.Position(token); double y = _create.Collection.Edition == OriginalEdition.Patch1078 ? (double)(float)point.Y : (double)point.Y; return new(GumpCollection.Ftol((double)point.X + left), GumpCollection.Ftol((double)point.Y + top), GumpCollection.Ftol((double)point.X + right), GumpCollection.Ftol(y + bottom)); }
    /// <summary>compact/세로/가로의 현재 크기·offset을 사용해 임시 클립 안에 실제 선택 배경을 출력한다.</summary>
    private void Highlight(uint token, int index)
    {
        var n = Node(token); ScreenRect rect;
        if ((n.MenuFlags & 0x1000000) != 0) rect = Rect(token, 2, 2, (float)((double)n.Width - 2), (float)((double)n.Height - 2));
        else if ((n.MenuFlags & 0x80000) != 0) rect = Rect(token, 2, (float)((double)n.Offsets[index] + 2), (float)((double)n.Width - 2), (float)((double)n.Heights[index] + n.Offsets[index] - 2));
        else rect = Rect(token, (float)((double)index * n.ItemWidth + 2), 2, (float)((double)unchecked(index + 1) * n.ItemWidth - 2), (float)((double)n.Height - 2));
        using var scope = new GumpClipScope(_clip, rect); _background.Background(rect, 0x2100000, Node(token).Type, Node(token).Frame);
    }
    /// <summary>두1픽셀 명암선을 개별 임시 클립에서 출력하고 매 pass 현재 전역을 읽는다.</summary>
    private void Separator(uint token, int index)
    {
        var n = Node(token); int y = GumpCollection.Ftol(((double)n.Offsets[index] + n.Margins[2]) - ((double)n.TargetHeight - n.Height) + (double)n.Heights[index] * .5 - 1); int right = GumpCollection.Ftol((double)n.Width + 2 - 4); var rect = Rect(token, 2, y, right, unchecked(y + 1));
        // 둘째 pass만 앞 화면 rect의 top/bottom을1씩 증가시킨다.클립은 각 pass마다 복구한다.
        for (int pass = 0; pass < 2; pass++) { rect = new(rect.Left, unchecked(rect.Top + pass), rect.Right, unchecked(rect.Bottom + pass)); using var scope = new GumpClipScope(_clip, rect); _background.Background(rect, pass != 0 ? 0x2080000U : 0x2100000U, _state.BackgroundType, _state.BackgroundFrame); }
    }
    /// <summary>스타일은 저장 문맥 그대로 출력한다.일반은 실제60바이트 항목/NUL과 임시 문맥을 사용한다.</summary>
    private void Text(uint token, int index, ScreenPoint point, uint[] context)
    { var item = Item(token, index); if ((item.Flags & 0x400) != 0) _style.Draw(item.Styled, point); else _text.Draw(MenuGumpLayout.MenuGumpItemBytes(item), point, GumpText.DecodeGumpTextContext(context)); }
    /// <summary>자체 클립→배경→저장 위치→항목별 현재 문맥/강조/표시/글자→클립 복구.표시/삭제 필터는 수집기의 책임이다.</summary>
    public void Draw(uint token)
    {
        var bounds = _collection.Bounds(token); using var scope = new GumpClipScope(_clip, bounds); _background.Draw(token); var point = _collection.Position(token);
        // 매 항목에서 현재 문맥/색을 복사하고 각 출력 콜백 뒤 현재 flags/개수/크기를 다시 읽는다.
        for (int index = 0; index < unchecked((int)Node(token).ItemCount); index++)
        {
            uint flags = Item(token, index).Flags; uint[] context = (uint[])Node(token).TextContext.Clone(); context[1] = 0; context[2] = _state.Foreground; if ((flags & 0x40) != 0) context[3] = 1; if ((flags & 0x200) != 0) { context[2] = _state.Disabled; context[0] = 0; } if ((flags & 8) != 0 && (flags & 0x10) == 0) Highlight(token, index);
            if ((Node(token).MenuFlags & 0x80000) == 0) { int y = GumpCollection.Ftol((double)point.Y + Node(token).Margins[2]); int x = GumpCollection.Ftol((double)index * Node(token).ItemWidth + Node(token).Margins[0] + point.X); Text(token, index, new(x, y), context); continue; }
            if ((Item(token, index).Flags & 0x10) != 0) { Separator(token, index); continue; }
            if ((Item(token, index).Flags & 4) != 0) { int markerX = GumpCollection.Ftol((double)Node(token).Margins[0] * .25 - ((double)Node(token).TargetWidth - Node(token).Width)); int markerY = GumpCollection.Ftol((double)Node(token).Offsets[index] + Node(token).Margins[2] - ((double)Node(token).TargetHeight - Node(token).Height)); Require(_hooks.Marker)(_state.MarkerType, new(unchecked(point.X + markerX + 5), unchecked(point.Y + markerY + 7))); }
            int textY = GumpCollection.Ftol((double)point.Y + Node(token).Offsets[index] + Node(token).Margins[2] - ((double)Node(token).TargetHeight - Node(token).Height)); int textX = GumpCollection.Ftol((double)point.X + Node(token).Margins[0] - ((double)Node(token).TargetWidth - Node(token).Width)); Text(token, index, new(textX, textY), context);
            if ((Item(token, index).Flags & (0x20 | 0x800)) != 0) { var measured = _layout.MeasureItem(token, index, context); int markerX = GumpCollection.Ftol(((double)Node(token).Margins[0] - ((double)Node(token).TargetWidth - Node(token).Width)) + measured.Width); int markerY = GumpCollection.Ftol((double)Node(token).Offsets[index] + Node(token).Margins[2] - ((double)Node(token).TargetHeight - Node(token).Height)); var decoded = GumpText.DecodeGumpTextContext(context); _text.DrawGump(_collection, token, " >\0"u8, new(markerX, markerY), Node(token).TextContext, in decoded); }
        }
    }
    /// <summary>현재 메뉴만 실제 Draw로 분배하고 다른 파생 처리를 보존한다.</summary>
    public static void BindMenuGumpDraw(GumpDrawHooks hooks, Func<uint, MenuGumpDraw?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var previous = hooks.Draw; hooks.Draw = t => { if (resolve(t) is { } menu) menu.Draw(t); else Require(previous)(t); }; }
    /// <summary>현재 실제 타입의 J 첫 번호+2를 같은 장식 원색 캐시에 연결한다.전체 GameAssets의 공급은 호출자가 연결한다.</summary>
    public static void BindMenuGumpDrawAssets(MenuGumpDrawHooks hooks, Func<uint, int> firstJ, GumpDecorationPixelTarget pixels)
    { ArgumentNullException.ThrowIfNull(firstJ); ArgumentNullException.ThrowIfNull(pixels); var shape = Require(pixels.Hooks().Shape); hooks.Marker = (type, point) => { if (type < 70) throw new ArgumentOutOfRangeException(nameof(type)); int first = firstJ(type); shape(type, unchecked((uint)first + 2), point); }; }
}
