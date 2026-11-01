using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host;

/// <summary>실제 공통 Gump 트리에 문자 편집기·버퍼·두 Slider를 조립한다.</summary>
public sealed class HostTextEntry : IDisposable
{
    private readonly GumpButtonTree _tree;
    private readonly HostHtmlWindows _windows;
    private readonly TextEntryStorage _storage;
    private readonly HashSet<uint> _buffers = [];
    private uint _nextBuffer = 0x70000000;
    private GumpTextDraw? _drawText;
    private GumpBackgroundDraw? _drawBackground;
    private GumpPixelTarget? _pixels;
    /// <summary>같은 문자/선택/Slider 본체다.</summary>
    public TextEntryGump Entry { get; }

    /// <summary>호스트의 실제 트리·팔레트·글꼴·시각 및 클립보드 경계를 사용한다.</summary>
    public HostTextEntry(HostMainMenu menu, HostHtmlWindows windows, HostHtmlText text, Screen screen,
        Func<double> now, Action<uint, bool> redraw, TextEntryClipboardRead? clipboardRead = null, TextEntryClipboardWrite? clipboardWrite = null)
    {
        _tree = menu.Tree; _windows = windows; _storage = new(clipboardRead, clipboardWrite);
        var metrics = new GumpText(_tree.Fonts); var slider = text.Slider; var widgets = text.SliderWidgets;
        var palette = screen.Colors();
        var state = new TextEntryGumpState { Brown = palette.Named[9].Index, Black = palette.Basic[0], Yellow = palette.Basic[5] };
        var hooks = new TextEntryGumpHooks
        {
            Node = token => _tree.Node(token) as TextEntryGumpNode, Console = token => _tree.Node(token) as ConsoleGumpNode,
            Base = _tree.Node, Buffer = _storage.Find, Slider = token => _tree.Node(token) as ScrollGumpNode,
            Allocate = Allocate, ReleaseBuffer = token => { _storage.ReleaseBuffer(token); _buffers.Remove(token); },
            ConstructBackground = (token, flags) => _tree.Backgrounds.Construct(token, 0, 0, flags, 1, 0, 0),
            ConstructSlider = (token, width, height, color, edge) => widgets.Construct(token, width, height, color, edge), BaseAttach = _tree.Tree.Attach,
            Attach = (token, x, y, parent, mode) => _tree.TreeHooks.Attach!(token, x, y, parent, mode), Push = _tree.Tree.Push,
            Idle = _tree.Lifetime.SetIdle, Redraw = redraw, Bubble = (token, code, first, second) => _tree.Events.Bubble(token, code, first, second),
            SliderRange = slider.SetRange, SliderPage = slider.SetPage, SliderPosition = slider.SetPosition, Now = now,
            Position = _tree.Collection.Position, AllocateBytes = _storage.Allocate, ReleaseBytes = _storage.Release, Bytes = _storage.Bytes,
            DestructBase = _tree.Lifetime.DestructBase, Assertion = line => throw new InvalidOperationException("TextEntry 원본 진단: " + line)
        };
        TextEntryGumpNode Node(uint token) => _tree.Node(token) as TextEntryGumpNode ?? throw new InvalidOperationException("문자 입력창 노드 없음");
        hooks.Measure = (uint token, ReadOnlySpan<byte> bytes, int length) => { int width = 0, height = 0; metrics.Measure(bytes, GumpText.DecodeGumpTextContext(Node(token).TextContext), ref width, ref height, length); return (width, height); };
        hooks.DrawBackground = token => (_drawBackground ?? throw new InvalidOperationException("문자 배경 출력 문맥 없음")).Draw(token);
        hooks.DrawText = (uint token, ReadOnlySpan<byte> bytes, ScreenPoint point, uint[]? context) =>
        {
            var glyph = _drawText ?? throw new InvalidOperationException("문자 글리프 출력 문맥 없음");
            if (context == null) glyph.DrawGump(_tree.Collection, token, bytes, point, Node(token).TextContext);
            else { var decoded = GumpText.DecodeGumpTextContext(context); glyph.DrawGump(_tree.Collection, token, bytes, point, Node(token).TextContext, in decoded); }
        };
        hooks.Fill = (rect, color) => (_pixels ?? throw new InvalidOperationException("문자 선택 출력 문맥 없음")).Fill(rect, color);
        Entry = new TextEntryGump(state, hooks);
        var drawHooks = new GumpDrawHooks { Draw = _ => throw new InvalidOperationException("문자 외 Gump 출력 경계") };
        TextEntryGump.BindTextEntryGump(_tree.TreeHooks, _tree.EventHooks, _tree.InputHooks, _tree.FrameHooks, drawHooks,
            _tree.LifetimeHooks, token => _tree.Node(token) is TextEntryGumpNode ? Entry : null, token => _tree.LifetimeHooks.Release!(token));
    }

    private uint Allocate(int size)
    {
        if (size == 0x28) { if (_nextBuffer == uint.MaxValue) throw new OverflowException("문자 버퍼 토큰 소진"); uint buffer = _nextBuffer++; _storage.Create(buffer); _buffers.Add(buffer); return buffer; }
        if (size != 0xd8) throw new ArgumentOutOfRangeException(nameof(size));
        uint slider = _tree.AllocateNode(new ScrollGumpNode()); _windows.TrackChild(slider); return slider;
    }

    /// <summary>새 파생 노드를 소유하고 원본 문자 입력창 생성기를 호출한다.</summary>
    public uint Create(int width, int height, ReadOnlyMemory<byte>? initial, uint font, uint background, uint color)
    {
        uint token = _tree.AllocateNode(new TextEntryGumpNode()); _windows.TrackChild(token);
        return Entry.Construct(token, width, height, initial, font, background, color);
    }

    /// <summary>공통 수집·정렬 중 현재 출력 장치를 잠시 빌려 같은 순서로 그린다.</summary>
    public void Draw(uint token, GumpTextDraw text, GumpBackgroundDraw background, GumpPixelTarget pixels)
    {
        if (_drawText != null) throw new InvalidOperationException("문자 출력 재진입");
        _drawText = text; _drawBackground = background; _pixels = pixels;
        try { Entry.Draw(token); } finally { _drawText = null; _drawBackground = null; _pixels = null; }
    }

    /// <summary>생성 도중 연결되지 않은 잔여 문자 버퍼를 반납한다.</summary>
    public void Dispose()
    {
        foreach (uint token in _buffers.ToArray()) { var buffer = _storage.Find(token) ?? throw new InvalidOperationException("문자 버퍼 소유권 불일치"); buffer.Destroy(); _storage.ReleaseBuffer(token); _buffers.Remove(token); }
    }
}
