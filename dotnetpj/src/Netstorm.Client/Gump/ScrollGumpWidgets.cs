using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // Slider와 실제 돌 화살표·배경 손잡이의 생성/출력/수명 영역이다.

/// <summary>실제 돌 버튼의 추가 방향 필드.</summary>
public class ScrollGumpArrowNode : StoneButtonGumpNode
{
    /// <summary>0위/1오른쪽/2아래/3왼쪽 삼각형.</summary>
    public uint Direction { get; set; }
}
/// <summary>현재 종류 BYTE와 손잡이 기본 배경.</summary>
public sealed class ScrollGumpWidgetState
{
    /// <summary>Slider 종류.</summary>
    public sbyte SliderAlignment { get; set; } = 39;
    /// <summary>손잡이 종류.</summary>
    public sbyte ThumbAlignment { get; set; } = 51;
    /// <summary>화살표 종류.</summary>
    public sbyte ArrowAlignment { get; set; } = 1;
    /// <summary>실제 자산 초기화 뒤의 기본 배경 타입.</summary>
    public uint DefaultType { get; set; }
    /// <summary>기본 배경 프레임.</summary>
    public uint DefaultFrame { get; set; } = 1;
}
/// <summary>같은 기본/세 파생 저장소와 원본 크기의 자식 할당/부착.</summary>
public sealed class ScrollGumpWidgetHooks
{
    /// <summary>실제 Slider.</summary>
    public Func<uint, ScrollGumpNode?>? Node { get; set; }
    /// <summary>실제 돌 화살표.</summary>
    public Func<uint, ScrollGumpArrowNode?>? Arrow { get; set; }
    /// <summary>실제 배경 손잡이.</summary>
    public Func<uint, ScrollGumpThumbNode?>? Thumb { get; set; }
    /// <summary>같은 기본 노드.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>0x178 화살표/패치0xd8·CD0xd4 손잡이의 실제 new.0은 실패다.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>같은 실제 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
}
/// <summary>cpppj/src/client/ScrollGumpWidgets.cpp의 부분 초기화/생성 인자/자식 순서/실제 Draw/scalar 위임.</summary>
public sealed class ScrollGumpWidgets
{
    /// <summary>현재 판본/등록.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 종류/배경 전역.</summary>
    private readonly ScrollGumpWidgetState _state;
    /// <summary>같은 기본 생성기.</summary>
    private readonly GumpCreate _factory;
    /// <summary>같은 실제 돌 버튼.</summary>
    private readonly StoneButtonGump _stones;
    /// <summary>같은 실제 배경 생성기.</summary>
    private readonly GumpBackgroundCreate _backgrounds;
    /// <summary>같은 실제 Slider 값/부착.</summary>
    private readonly ScrollGump _sliders;
    /// <summary>같은 실제 기본 scalar 수명.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>현재 파생/할당/부착 조회.</summary>
    private readonly ScrollGumpWidgetHooks _hooks;
    /// <summary>필요한 가상 경계만 도달 시 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("Slider 생성/출력 가상 경계 필요");
    /// <summary>signed 종류 BYTE의 DWORD 확장.</summary>
    private static uint Alignment(sbyte value) => unchecked((uint)(int)value);
    /// <summary>현재 같은 실제 파생 객체를 검사한다.</summary>
    private T Shared<T>(uint token, Func<uint, T?> lookup) where T : GumpCreateNode { var n = token != 0 ? lookup(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("Slider 생성 공유 노드 없음"); return n; }
    /// <summary>현재 Slider 필드.</summary>
    private ScrollGumpNode Node(uint token) => Shared(token, _hooks.Node!);
    /// <summary>현재 돌 화살표 필드.</summary>
    private ScrollGumpArrowNode Arrow(uint token) => Shared(token, _hooks.Arrow!);
    /// <summary>현재 배경 손잡이 필드.</summary>
    private ScrollGumpThumbNode Thumb(uint token) => Shared(token, _hooks.Thumb!);
    /// <summary>같은 실제 기본 필드.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("Slider 기본 노드 없음");
    /// <summary>같은 기본/돌 버튼/배경/Slider/수명 처리기를 빌린다.</summary>
    public ScrollGumpWidgets(GumpCreateState create, ScrollGumpWidgetState state, GumpCreate factory, StoneButtonGump stones, GumpBackgroundCreate backgrounds, ScrollGump sliders, GumpLifetime lifetime, ScrollGumpWidgetHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(stones); ArgumentNullException.ThrowIfNull(backgrounds); ArgumentNullException.ThrowIfNull(sliders); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Arrow); Require(hooks.Thumb); Require(hooks.Base); _create = create; _state = state; _factory = factory; _stones = stones; _backgrounds = backgrounds; _sliders = sliders; _lifetime = lifetime; _hooks = hooks; }
    /// <summary>빈 라벨/명시8슬롯 문맥의 실제 돌 버튼을 만든 뒤 파생 표/팝업 유지/종류/방향을 쓴다.</summary>
    public uint ConstructArrow(uint token, int width, int height, uint direction, uint unused, uint color)
    { Arrow(token); _stones.Construct(token, width, height, null, unused, [0, 0, color, 0, 0, 0, 1, 1]); var n = Arrow(token); n.Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x5116a8U : 0x500c28U; n.ButtonFlags |= 0x20; n.Command = Alignment(_state.ArrowAlignment); n.Direction = direction; return token; }
    /// <summary>실제 기본 질감 배경을 만든다.추가 인자 사본과 기본 배경 타입/프레임을 구별하며 이전 좌표는 보존한다.</summary>
    public uint ConstructThumb(uint token, int width, int height, uint type, uint frame)
    {
        Thumb(token); uint defaultType = _state.DefaultType, defaultFrame = _state.DefaultFrame; _backgrounds.Construct(token, 0, 0, 0x03ec0000, 0x201, defaultType, defaultFrame); var n = Thumb(token); n.Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x511668U : 0x500be8U; n.Handle = new(); n.Command = Alignment(_state.ThumbAlignment); n.DetailType = type; n.DetailFrame = frame; n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale); n.Flags |= 0x200; return token;
    }
    /// <summary>성공한 원본0x178 new만 실제 화살표 생성으로 진행한다.</summary>
    private uint NewArrow(int width, int height, uint direction, uint type, uint frame) { uint token = Require(_hooks.Allocate)(0x178); return token != 0 ? ConstructArrow(token, width, height, direction, type, frame) : 0; }
    /// <summary>CD는 이전X 필드만큼 작은 실제 new를 요청한다.</summary>
    private uint NewThumb(int width, int height, uint type, uint frame) { uint token = Require(_hooks.Allocate)(_create.Collection.Edition == OriginalEdition.Patch1078 ? 0xd8 : 0xd4); return token != 0 ? ConstructThumb(token, width, height, type, frame) : 0; }
    /// <summary>생성 직후 자식도 실제 가상 Attach(parent,mode0)로 연결한다.</summary>
    private void Attach(uint token, float x, float y, uint parent) { Base(token); Require(_hooks.Attach)(token, x, y, parent, 0); }
    /// <summary>미작성+bc 보존/세 자식 생성·부착 뒤 range0..100/page10/position50을 실제 몸체로 적용한다.</summary>
    public uint Construct(uint token, int width, int height, uint type, uint frame)
    {
        Node(token); _factory.Construct(token, 0, 0, 0, 1); var n = Node(token); bool patch = _create.Collection.Edition == OriginalEdition.Patch1078; n.Vtable = patch ? 0x5116f8U : 0x500c78U; n.Minimum = n.Maximum = n.Position = n.Page = n.Step = 0; n.ReservedB0 = 0; n.ScrollFlags = 0; n.Reserved.AsSpan().Clear(); n.Decrement = n.Increment = n.Thumb = 0; n.Command = Alignment(_state.SliderAlignment);
        if (width == 0) width = 40; if (height == 0) height = 40; bool horizontal = height < width; n.ScrollFlags = horizontal ? 1U : 0U; n.Flags |= 0x200; n.Step = 1; n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); n.BaseHeight = height; n.Height = (float)((patch ? (double)n.BaseHeight : height) * n.Scale);
        int thickness = GumpCollection.Ftol(horizontal ? n.BaseHeight : patch ? (double)width : n.BaseWidth), twice = unchecked(thickness + thickness);
        if (horizontal)
        {
            uint decrement = NewArrow(thickness, height, 3, type, frame); Node(token).Decrement = decrement; uint increment = NewArrow(thickness, height, 1, type, frame); Node(token).Increment = increment; uint thumb = NewThumb(unchecked(width - twice), height, type, frame); Node(token).Thumb = thumb;
            Attach(Node(token).Decrement, 0, 0, token); Attach(Node(token).Increment, unchecked(width - thickness), 0, token); Attach(Node(token).Thumb, thickness, 0, token);
        }
        else
        {
            uint decrement = NewArrow(width, thickness, 0, type, frame); Node(token).Decrement = decrement; uint increment = NewArrow(width, thickness, 2, type, frame); Node(token).Increment = increment; uint thumb = NewThumb(width, unchecked(height - twice), type, frame); Node(token).Thumb = thumb;
            Attach(Node(token).Decrement, 0, 0, token); Attach(Node(token).Increment, 0, unchecked(height - thickness), token); Attach(Node(token).Thumb, 0, thickness, token);
        }
        Node(token).Position = 0; _sliders.SetRange(token, 0, 100); _sliders.SetPage(token, 10); _sliders.SetPosition(token, 50); return token;
    }
    /// <summary>실제 돌 버튼을 그린 뒤 현재 경계/기본 글자색으로 원본 삼각형의 세 선을 출력한다.</summary>
    public void DrawArrow(uint token, GumpCollection collection, GumpTextDraw text, GumpPixelTarget target)
    {
        Arrow(token); _stones.Draw(token, collection, text, target); ScreenRect b = collection.Bounds(token); uint color = Arrow(token).TextContext[2], direction = Arrow(token).Direction; int left = unchecked(b.Left + 2), top = unchecked(b.Top + 2), right = unchecked(b.Right - 2), bottom = unchecked(b.Bottom - 2), middleX = unchecked(left + right) / 2, middleY = unchecked(top + bottom) / 2;
        GumpLine[] lines = direction switch { 1 => [new(new(left, top), new(left, bottom), color), new(new(left, top), new(right, middleY), color), new(new(left, bottom), new(right, middleY), color)], 2 => [new(new(left, top), new(right, top), color), new(new(left, top), new(middleX, bottom), color), new(new(right, top), new(middleX, bottom), color)], 3 => [new(new(right, top), new(right, bottom), color), new(new(right, top), new(left, middleY), color), new(new(right, bottom), new(left, middleY), color)], _ => [new(new(left, bottom), new(right, bottom), color), new(new(left, bottom), new(middleX, top), color), new(new(right, bottom), new(middleX, top), color)] }; target.Lines(lines);
    }
    /// <summary>실제 배경 Draw 뒤 포함 사각형의 끝점에+1하지 않고 현재 기본 타입/프레임으로 손잡이를 그린다.</summary>
    public void DrawThumb(uint token, GumpCollection collection, GumpBackgroundDraw backgrounds)
    { Thumb(token); backgrounds.Draw(token); ScreenRect b = collection.Bounds(token); var n = Thumb(token); var r = n.Handle; backgrounds.Background(new(unchecked(b.Left + r.Left), unchecked(b.Top + r.Top), unchecked(b.Left + r.Right), unchecked(b.Top + r.Bottom)), 0x03e00000, n.Type, n.Frame); }
    /// <summary>추가 소유 자원 없이 같은 기본 scalar 소멸자로 직접 위임한다.</summary>
    public uint DeleteScalar(uint token, uint flags) { Base(token); return _lifetime.DeleteScalar(token, flags); }
    /// <summary>Slider는 빈 Draw,화살표/손잡이는 실제 픽셀,나머지는 이전 분배로 연결한다.</summary>
    public static void BindScrollGumpWidgetDraw(GumpDrawHooks hooks, GumpCollection collection, GumpTextDraw text, GumpPixelTarget target, GumpBackgroundDraw backgrounds, Func<uint, ScrollGumpWidgets?> slider, Func<uint, ScrollGumpWidgets?> arrow, Func<uint, ScrollGumpWidgets?> thumb)
    { ArgumentNullException.ThrowIfNull(slider); ArgumentNullException.ThrowIfNull(arrow); ArgumentNullException.ThrowIfNull(thumb); var previous = hooks.Draw; hooks.Draw = t => { if (slider(t) is not null) return; if (arrow(t) is { } a) a.DrawArrow(t, collection, text, target); else if (thumb(t) is { } h) h.DrawThumb(t, collection, backgrounds); else Require(previous)(t); }; }
    /// <summary>현재 세 파생의 기본 scalar 위임을 자식 우선 수명 처리기에 연결한다.</summary>
    public static void BindScrollGumpWidgetLifetime(GumpLifetimeHooks hooks, Func<uint, ScrollGumpWidgets?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var previous = hooks.DeletingDestructor; hooks.DeletingDestructor = (t, f) => { if (resolve(t) is { } w) w.DeleteScalar(t, f); else Require(previous)(t, f); }; }
}
