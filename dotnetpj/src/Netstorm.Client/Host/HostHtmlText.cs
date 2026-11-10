using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;

namespace Netstorm.Client.Host; // 실제 HTML 본문의 다중 줄·슬라이더와 글꼴 소유 영역이다.

/// <summary>공통 Gump 트리와 실제 글꼴에 HTML 다중 줄 본문·스크롤 자식을 조립한다.
/// 그림 TYPE과 커서·이름 소리는 별도 장치이며 선택 시 제공받는다.</summary>
public sealed class HostHtmlText
{
    /// <summary>같은 공통 등록·입력·수명.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>실제 글리프를 그리는 같은 논리 화면.</summary>
    private readonly Screen _screen;
    /// <summary>본문과 글리프 장치가 공유하는 현재 클립.</summary>
    private readonly GumpDrawState _clip = new();
    /// <summary>공통 출력기와 본문이 공유하는 현재 클립.</summary>
    internal GumpDrawState Clip => _clip;
    /// <summary>Draw 호출 동안에만 활성인 실제 글리프 출력기.</summary>
    private GumpTextDraw? _activeText;
    /// <summary>부분 생성 자식까지 보관하는 같은 HTML 창 소유자.</summary>
    private readonly HostHtmlWindows _windows;
    /// <summary>원본 버퍼는 Gump 등록과 별도 주소 수명을 가진다.</summary>
    private uint _nextBuffer = 0x60000000;
    /// <summary>살아 있는 실제 버퍼 주소.</summary>
    private readonly HashSet<uint> _bufferTokens = [];
    /// <summary>같은 글자·그림 조각 저장소.</summary>
    public StyleTextState Fragments { get; } = new();
    /// <summary>본문과 메뉴 항목이 같은 조각 저장소를 쓰는 스타일 처리기.</summary>
    public StyleText Style { get; }
    /// <summary>현재 실제 본문 버퍼 소유자.</summary>
    public StyleTextBufferStore Buffers { get; }
    /// <summary>실제 표시·배치와 스크롤 처리기.</summary>
    public ScrollingTextGump Text { get; }
    /// <summary>같은 실제 본문 생성·입력·수명 처리기.</summary>
    public ScrollingTextGumpWidgets Widgets { get; }
    /// <summary>공통 출력기가 화살표·손잡이에 사용하는 실제 슬라이더 처리기.</summary>
    internal ScrollGumpWidgets SliderWidgets { get; }
    /// <summary>문자 입력창도 같은 Slider 처리기를 사용한다.</summary>
    internal ScrollGump Slider { get; }
    /// <summary>같은 실제 다중 줄 스타일 명령 파서.</summary>
    public ScrollingTextGumpMarkup Parser { get; }
    /// <summary>본문에 사용하는 파일 팔레트의 기본 흰색.</summary>
    private readonly uint _bodyColor;
    /// <summary>ScrollingText 슬라이더 화살표에 전달하는 현재 팔레트의 갈색 색 번호.</summary>
    public uint SliderArrowType { get; }
    /// <summary>ScrollingText 슬라이더 화살표에 전달하는 현재 팔레트의 검정 색 번호.</summary>
    public uint SliderArrowFrame { get; }
    /// <summary>같은 본문 위젯이 슬라이더 생성에 쓰는 실제 상태(화살표 색 포함).</summary>
    internal ScrollingTextGumpWidgetState WidgetState { get; }

    /// <summary>같은 메인 메뉴·화면·물리 폴링과 현재 호스트 효과를 사용한다.</summary>
    public HostHtmlText(HostMainMenu menu, HostHtmlWindows windows, Screen screen, Func<uint, InputEvent> poll,
        Action<uint, bool> redraw, Action<uint> cursor, Action<string> sound)
    {
        ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(windows); ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(poll);
        ArgumentNullException.ThrowIfNull(redraw); ArgumentNullException.ThrowIfNull(cursor); ArgumentNullException.ThrowIfNull(sound);
        _tree = menu.Tree; _windows = windows; _screen = screen; _bodyColor = screen.Colors().Basic[7];
        var font = new GumpText(_tree.Fonts); var styleHooks = new StyleTextHooks();
        StyleText.BindStyleTextFonts(styleHooks, font); StyleText.BindStyleTextPalette(styleHooks, screen.Colors());
        styleHooks.TextDraw = (bytes, position, context) =>
            (_activeText ?? throw new InvalidOperationException("HTML 본문 실제 출력 문맥 없음"))
                .Draw(bytes, position, GumpText.DecodeGumpTextContext(context));
        var pictures = new Markup(Fragments, new(), new()); Markup.BindMarkup(styleHooks, pictures);
        var markupHooks = new StyleTextMarkupHooks { Sound = sound };
        StyleTextMarkup.BindStyleTextMarkupPalette(markupHooks, screen.Colors());
        var commands = new StyleTextMarkup(Fragments, pictures, new(), markupHooks); commands.Hooks(styleHooks);
        var style = Style = new StyleText(Fragments, styleHooks); var bufferHooks = new StyleTextBufferHooks();
        StyleTextBuffer.BindStyleTextBufferFonts(bufferHooks, font); StyleTextBuffer.BindStyleTextBufferMarkup(bufferHooks, pictures);
        Buffers = new StyleTextBufferStore(Fragments, style, bufferHooks, token =>
        {
            if (!_bufferTokens.Remove(token)) throw new InvalidOperationException("HTML 본문 버퍼 반납 불일치");
        });
        var sliderState = new ScrollGumpState { ClickEvent = 0x0200002c, RepeatEvent = 0x0200002f,
            StepEvent = 0x0200002e, ChangeEvent = 0x02000036 };
        var slider = Slider = new ScrollGump(_tree.Create, sliderState, _tree.Tree, _tree.Collection, _tree.Events, _tree.Input,
            new() { Node = token => _tree.Node(token) as ScrollGumpNode, ThumbNode = token => _tree.Node(token) as ScrollGumpThumbNode,
                BaseNode = _tree.Node, Push = token => _tree.TreeHooks.Push!(token),
                Attach = (token, x, y, parent, mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode), Redraw = redraw, Poll = poll });
        ScrollGump.BindScrollGump(_tree.TreeHooks, _tree.EventHooks, _tree.InputHooks, _tree.FrameHooks,
            token => _tree.Node(token) is ScrollGumpNode ? slider : null,
            token => _tree.Node(token) is ScrollGumpThumbNode ? slider : null);
        var sliderWidgets = SliderWidgets = new ScrollGumpWidgets(_tree.Create, new() { DefaultType = menu.BackgroundType, DefaultFrame = 1 },
            _tree.Factory, _tree.Stones, _tree.Backgrounds, slider, _tree.Lifetime,
            new() { Node = token => _tree.Node(token) as ScrollGumpNode, Arrow = token => _tree.Node(token) as ScrollGumpArrowNode,
                Thumb = token => _tree.Node(token) as ScrollGumpThumbNode, Base = _tree.Node, Allocate = AllocateSliderChild,
                Attach = (token, x, y, parent, mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode) });
        ScrollGumpWidgets.BindScrollGumpWidgetLifetime(_tree.LifetimeHooks,
            token => _tree.Node(token) is ScrollGumpNode or ScrollGumpArrowNode or ScrollGumpThumbNode ? sliderWidgets : null);
        var textHooks = new ScrollingTextGumpHooks { Node = token => _tree.Node(token) as ScrollingTextGumpNode,
            BaseNode = _tree.Node, Redraw = redraw };
        StyleTextBufferStore.BindScrollingTextBuffers(textHooks, Buffers); ScrollGump.BindScrollingTextSlider(textHooks, slider);
        GumpDraw.SetGumpDrawClip(_clip, new(0, 0, screen.Width, screen.Height));
        Text = new ScrollingTextGump(_tree.Create, _tree.Collection, _clip, Fragments, style, textHooks);
        // 원본 ScrollingText 생성(004a64b0 상당)은 슬라이더 너비 9·높이 100과 함께 현재 팔레트의 갈색(named 9)·검정(basic 0) 색 번호를 넘긴다.
        // fortGump TYPE·프레임 1을 넘기면 화살표가 팔레트 1번(붉은 갈색)으로 그려지므로 원본과 같이 색 번호를 쓴다.
        // 근거: cpppj/src/client/RawMenuControls.cpp의 RawMenuControls::SetPalette(2026-11-09 c91d9b84 수정 반영).
        var palette = screen.Colors();
        SliderArrowType = palette.Named[9].Index; SliderArrowFrame = palette.Basic[0];
        var state = new ScrollingTextGumpWidgetState { SliderType = SliderArrowType, SliderFrame = SliderArrowFrame,
            ChangeEvent = sliderState.ChangeEvent, StepEvent = sliderState.StepEvent, RefreshEvent = 0x0200001b,
            CursorEvent = 0x0200003b, LeftTagEvent = 0x02000025, RightTagEvent = 0x02000026 };
        var widgetHooks = new ScrollingTextGumpWidgetHooks { Node = token => _tree.Node(token) as ScrollingTextGumpNode,
            Slider = token => _tree.Node(token) as ScrollGumpNode, Base = _tree.Node, Allocate = AllocateTextChild,
            Push = token => _tree.TreeHooks.Push!(token), Attach = (token, x, y, parent, mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode),
            Redraw = redraw, Cursor = cursor };
        WidgetState = state;
        Widgets = new ScrollingTextGumpWidgets(_tree.Create, state, _tree.Factory, _tree.Tree, _tree.Lifetime,
            _tree.Events, _tree.Input, Fragments, Buffers, Text, sliderWidgets, widgetHooks);
        ScrollingTextGumpWidgets.BindScrollingTextGumpWidgets(_tree.TreeHooks, _tree.EventHooks, _tree.InputHooks,
            _tree.FrameHooks, _tree.LifetimeHooks, token => _tree.Node(token) is ScrollingTextGumpNode ? Widgets : null);
        Parser = new ScrollingTextGumpMarkup(Text, style, pictures, commands,
            new() { Node = token => _tree.Node(token) as ScrollingTextGumpNode, Base = _tree.Node });
    }

    /// <summary>슬라이더의 실제 화살표 또는 손잡이 파생 객체를 할당한다.</summary>
    private uint AllocateSliderChild(int size)
    {
        uint token = _tree.AllocateNode(size switch
        {
            0x178 => new ScrollGumpArrowNode(), 0xd8 => new ScrollGumpThumbNode(),
            _ => throw new ArgumentOutOfRangeException(nameof(size), "HTML 슬라이더 자식 크기 미지원")
        });
        _windows.TrackChild(token); return token;
    }

    /// <summary>본문 버퍼는 별도 주소, 슬라이더는 공통 Gump 객체로 할당한다.</summary>
    private uint AllocateTextChild(int size)
    {
        if (size == 0xd8) { uint slider = _tree.AllocateNode(new ScrollGumpNode()); _windows.TrackChild(slider); return slider; }
        if (size != 0x38 || _nextBuffer >= 0x63000000) throw new ArgumentOutOfRangeException(nameof(size), "HTML 본문 자식 크기/주소 오류");
        uint token = _nextBuffer++; _bufferTokens.Add(token); return token;
    }

    /// <summary>현재 본문만 실제 글꼴과 화면 클립으로 출력한다. 호출자는 다른 Gump와의 표시 순서를 정한다.</summary>
    /// <param name="token">같은 트리에 생성·부착된 다중 줄 본문 토큰.</param>
    public void Draw(uint token)
    {
        if (_tree.Node(token) is not ScrollingTextGumpNode) throw new ArgumentException("HTML 본문 토큰 필요", nameof(token));
        if (_activeText is not null) throw new InvalidOperationException("HTML 본문 출력 재진입");
        byte[] pixels = _screen.Lock();
        try
        {
            GumpDraw.SetGumpDrawClip(_clip, new(0, 0, _screen.Width, _screen.Height));
            var target = new GumpPixelTarget(pixels, _screen.Pitch, _screen.Height, _clip);
            _activeText = new GumpTextDraw(_tree.Fonts, GumpTextDraw.MakeGumpGlyphPixelHooks(target, new()));
            var draw = new GumpDraw(_tree.Collection, _clip, new() { Draw = Text.Draw });
            draw.Draw(token);
        }
        finally { _activeText = null; _screen.Unlock(); }
        _screen.Update(_tree.Collection.Bounds(token));
    }

    /// <summary>공유 표면 출력 동안에만 스타일 글리프의 현재 장치를 설치하고 예외에도 해제한다.</summary>
    internal void WithTextDraw(GumpTextDraw text, Action draw)
    {
        ArgumentNullException.ThrowIfNull(text); ArgumentNullException.ThrowIfNull(draw);
        if (_activeText is not null) throw new InvalidOperationException("HTML 본문 출력 재진입");
        _activeText = text;
        try { draw(); } finally { _activeText = null; }
    }

    /// <summary>창 배치 장치가 같은 소유자의 본문인지 확인한다.</summary>
    internal bool BelongsTo(HostHtmlWindows windows) => ReferenceEquals(_windows, windows);

    /// <summary>HTML 부모의 본문 생성·파싱·스크롤 경계를 현재 실제 처리기에 연결한다.</summary>
    public void Bind(HostHtmlWindows windows)
    {
        ArgumentNullException.ThrowIfNull(windows);
        if (!ReferenceEquals(windows, _windows)) throw new ArgumentException("다른 HTML 소유자에 본문을 연결할 수 없다", nameof(windows));
        windows.BindBody(token => Widgets.Construct(token, 0, 0, 0, _bodyColor),
            (token, text) => Parser.Parse(token, Encoding.Latin1.GetBytes(text)),
            (token, value) => Widgets.SetScrollable(token, unchecked((uint)value)));
        windows.BindFit(this, _screen, () => new ScreenRect(0, 0, _screen.Width, _screen.Height));
    }
}
