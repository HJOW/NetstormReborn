using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;
using Netstorm.Client.Input;
using Netstorm.Core.Original;

// 같은 실제 트리/Console/조각/controlled 메뉴에 전체 Chat 부모를 조립한다.
namespace Netstorm.Client.Host;

/// <summary>RawChatWindowContext의 현재 Client/자산/문자열과 선택된 외부 효과다.</summary>
public sealed class HostChatDevices
{
    /// <summary>공장에 없는 현재 Client 전역을 사용 시점에 읽는다.</summary>
    public Func<uint, uint>? Read { get; init; }
    /// <summary>현재 표시 BYTE 번역이다.</summary>
    public Func<string, string>? Translate { get; init; }
    /// <summary>안정된 Chat 문자열 이외 실제 입력 포인터를 해석한다.</summary>
    public Func<uint, string>? Text { get; init; }
    /// <summary>최초 자산 TYPE의 실제 클러스터 개수다.</summary>
    public Func<uint, int>? FrameCount { get; init; }
    /// <summary>최초 자산 SHP 헤더의 실제 signed 높이다.</summary>
    public Func<uint, uint, short>? FrameHeight { get; init; }
    /// <summary>현재 같은 Identity로 다중 수신 대상을 검사한다.</summary>
    public Func<string, bool>? Recipient { get; init; }
    /// <summary>현재 manager와 실제 입력 BYTE를 제출 부모에 전달한다.</summary>
    public Action<uint, string>? SendChat { get; init; }
    /// <summary>현재60BYTE Identity를 같은 실제 저장소에서 조회한다.</summary>
    public Func<byte[], bool>? Resolve { get; init; }
    /// <summary>원문 종류/서식/인자의 실제 진단이다.</summary>
    public Action<uint, string, uint>? Diagnostic { get; init; }
    /// <summary>현재 Config의 signed 옵션을 읽는다.</summary>
    public Func<string, int>? Config { get; init; }
    /// <summary>동일 공통 GumpHelp로 나머지 입력을 전달한다.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; init; }
    /// <summary>현재 선택된 실제 로그 파일의 유효성이다.</summary>
    public Func<uint, bool>? FileValid { get; init; }
    /// <summary>같은 실제 파일에 NUL 전 BYTE만 쓴다.</summary>
    public ClientChatLogWrite? WriteLog { get; init; }
    /// <summary>원문 quit callback의 현재 종료 사건을 실제 큐에 예약한다.</summary>
    public Action<uint, uint, uint>? Quit { get; init; }
    /// <summary>돌아오면 계속하는 원문 줄 번호 진단이다.</summary>
    public Action<uint>? Assertion { get; init; }
}

/// <summary>RawMenuChat.inc/RawMenuControls의 전체 실제 Chat 공장과 가상 입력/부착/수명이다.</summary>
public sealed class HostChatWindows : IDisposable
{
    /// <summary>메뉴/HTML과 공유하는 실제 공통 노드 저장소다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>같은 실제 다중 줄/조각/Slider 소유자다.</summary>
    private readonly HostHtmlText _text;
    /// <summary>동일 Console 편집/버퍼/완성 장치다.</summary>
    private readonly HostTextEntry _entry;
    /// <summary>동일 controlled 메뉴 스택과 안정 payload 소비자다.</summary>
    private readonly HostHtmlMenus _menus;
    /// <summary>현재 팔레트를 매번 조회하는 같은 논리 화면이다.</summary>
    private readonly Screen _screen;
    /// <summary>창 교체 후에도 유지하는 실제 Client 상태다.</summary>
    private readonly ClientChatWindowGlobals _globals;
    /// <summary>같은 최초 say/인접 표의 실제 명령 등록이다.</summary>
    private readonly ClientDotCommandGlobals _commands;
    /// <summary>같은 로그와 callback/편집 부모다.</summary>
    private readonly ClientChatWindowRuntime _runtime;
    /// <summary>동일 Root가 설치한 실제 네 자식 기하다.</summary>
    public HudChatGeometry Geometry { get; }
    /// <summary>현재 Client getter와 선택 장치다.</summary>
    private readonly HostChatDevices _devices;
    /// <summary>같은 저장소에 설치한 전체 원문 부모다.</summary>
    public ChatWindow Window { get; }
    /// <summary>공통 delegate를 끊지 않고 최종 대여 반납만 한 번 수행한다.</summary>
    private bool _disposed;
    /// <summary>선택된 현재 장치만 요구하며 가짜 성공으로 대신하지 않는다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("Chat 공장의 실제 선택 장치 미연결");
    /// <summary>표시 BYTE 문자열에 안정된 첫NUL을 덧붙인다.</summary>
    private static byte[] Bytes(string text) => Encoding.Latin1.GetBytes(text + '\0');
    /// <summary>현재 같은 공통 저장소의 실제 Chat root다.</summary>
    private HudChatNode? Node(uint token) => _tree.Node(token) as HudChatNode;
    /// <summary>같은 현재 Client 포인터를 임의 복구하지 않고 죽음 표시도 확인한다.</summary>
    public uint CurrentRoot => Node(_globals.Current.Value) is { } node && (node.Flags & 0x10000) == 0 ? _globals.Current.Value : 0;
    /// <summary>실제 동일 장치를 빌리고 공유128줄/전체 부모/가상 분배를 설치한다.창은 Create에서 만든다.</summary>
    public HostChatWindows(HostMainMenu menu, HostHtmlWindows windows, HostHtmlText text, HostTextEntry entry,
        HostHtmlMenus menus, Screen screen, ClientChatWindowGlobals globals, ClientDotCommandGlobals commands,
        ClientChatWindowRuntime runtime, HudChatGeometry geometry, HostChatDevices devices)
    {
        ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(windows);
        _tree = menu.Tree; _text = text ?? throw new ArgumentNullException(nameof(text)); _entry = entry ?? throw new ArgumentNullException(nameof(entry)); _menus = menus ?? throw new ArgumentNullException(nameof(menus));
        _screen = screen ?? throw new ArgumentNullException(nameof(screen)); _globals = globals ?? throw new ArgumentNullException(nameof(globals)); _commands = commands ?? throw new ArgumentNullException(nameof(commands));
        _runtime = runtime ?? throw new ArgumentNullException(nameof(runtime)); Geometry = geometry ?? throw new ArgumentNullException(nameof(geometry)); _devices = devices ?? throw new ArgumentNullException(nameof(devices));
        Need(devices.Read); Need(devices.Translate); Need(devices.Text); if (!_text.BelongsTo(windows)) throw new ArgumentException("Chat의 같은 실제 HTML/조각 저장소 필요");
        _entry.BindChatEditing(value => Need(devices.Recipient)(Encoding.Latin1.GetString(value)), _runtime.CompleteEntry);
        _runtime.BindSharedLog(this, _text.Buffers); _text.TrackBorrowedBuffer(0x548758, _globals.Log);
        var metrics = new GumpText(_tree.Fonts);
        var hooks = new ChatWindowHooks
        {
            Chat = Node, Base = _tree.Node, Console = token => _tree.Node(token) as ConsoleGumpNode,
            Text = token => _tree.Node(token) as ScrollingTextGumpNode, Buffer = _entry.FindBuffer, Entry = _entry.Entry, Geometry = Geometry,
            Global = Read, Allocate = Allocate, ConstructBase = (token, width, height) => _tree.Factory.Construct(token, 0, width, height, 1),
            ConstructHelper = (token, label, color) => _tree.Stones.Construct(token, 0, 0, Bytes(label), color, null), FitHelper = _tree.Stones.FitToText,
            Title = _tree.SetHelpText, Help = _tree.SetHelpWord, Translate = devices.Translate,
            Measure = (token, value) => { var node = _tree.Node(token) ?? throw new InvalidOperationException("Chat 실제 측정 root 없음"); int width = 0, height = 0; metrics.Measure(Bytes(value), GumpText.DecodeGumpTextContext(node.TextContext), ref width, ref height); return (width, height); },
            ConstructBackground = (token, width, height, flags) => _tree.Backgrounds.Construct(token, width, height, flags, 1, 0, 0),
            ConstructScrolling = (token, width, height, font, color) => _text.Widgets.Construct(token, width, height, font, color),
            ReplaceBuffer = _text.Widgets.ReplaceBuffer, SharedBuffer = () => 0x548758, SetSlider = _text.Widgets.SetSlider, SetScrollable = _text.Widgets.SetScrollable,
            Attach = (token, x, y, parent, mode) => Need(_tree.TreeHooks.Attach)(token, x, y, parent, mode), Push = _tree.Tree.Push, DestructBase = _tree.Lifetime.DestructBase,
            RegisterCommand = (name, callback, flag) => _commands.Register(Bytes(name), callback, flag),
            BufferAppendCallback = (token, callback) => { if (token != 0x548758 || callback != 0x42f590) throw new InvalidOperationException("Chat 실제 공유 로그 callback 불일치"); _runtime.InstallLogCallback(devices.FileValid, devices.WriteLog); },
            FrameCount = type => Need(devices.FrameCount)(type), FrameHeight = (type, frame) => Need(devices.FrameHeight)(type, frame),
            RequestScroll = _text.Text.RequestScroll, RelativeScroll = _text.Text.RequestRelativeScroll, RequestBottom = token => _text.Text.RequestBottom(token), AtBottom = _text.Text.AtBottom,
            AppendLog = (token, value) => _text.Parser.Parse(token, Bytes(value)), BaseHandle = (token, code, first, second) => Need(devices.BaseHandle)(token, code, first, second),
            Bubble = (token, code, first, second) => _tree.Events.Bubble(token, code, first, second),
            Recipient = value => Need(devices.Recipient)(value), SendChat = (manager, value) => Need(devices.SendChat)(manager, value), Resolve = record => Need(devices.Resolve)(record),
            Log = (level, format, parameter) => Need(devices.Diagnostic)(level, format, parameter), Config = key => Need(devices.Config)(key),
            MenuBegin = () => _menus.Widgets.BeginVertical(0), MenuFinalize = () => _menus.Menu.Current(), MenuTitle = value => _menus.Controls.AddTitle(Bytes(value)),
            MenuItem = (label, code, payload) => _menus.Controls.AddText(Bytes(label), code, payload is null ? 0 : _globals.Store(payload), true, true, false),
            MenuChecked = (check, label, code, payload) => _menus.Controls.AddChecked(check != 0, Bytes(label), code, _globals.Store(payload), true, true),
            MenuTake = _menus.Menu.TakeCurrent, MenuSubmenu = (label, child) => _menus.Controls.AddSubmenu(Bytes(label), child, true, false),
            MenuOpen = (x, y, parent) => _menus.Menu.AttachCurrent(unchecked((int)x), unchecked((int)y), parent), Assertion = devices.Assertion,
        };
        Window = new(_globals.Window, hooks); BindVirtuals();
    }
    /// <summary>원문 PE 사건/현재 팔레트와 장기 창 상태만 공장에서 해석하고 나머지는 실제 Client getter로 읽는다.</summary>
    private uint Read(uint address) => address switch
    {
        0x540424 => 9, 0x548740 => _globals.Current.Value, 0x548744 => _globals.Window.Registered, 0x54da91 => _globals.Window.CompletionReady,
        0x5acd24 => _screen.Colors().Named[9].Index, 0x5b5dc0 => _screen.Colors().Named[32].Index, 0x5b5e60 => _screen.Colors().Named[30].Index,
        0x5b5dd4 => _screen.Colors().Named[31].Index, 0x5b5dd8 => _screen.Colors().Named[33].Index,
        0x542b24 => 0x02000115, 0x5427a8 => 0x0200002c, 0x5429f0 => 0x020000bf, 0x542914 => 0x02000088, 0x5427d8 => 0x02000038,
        0x542794 => 0x02000027, 0x542af8 => 0x02000101, 0x54298c => 0x020000a6, 0x542b08 => 0x02000105, 0x542b0c => 0x02000106, 0x5428ac => 0x0200006d,
        _ => Need(_devices.Read)(address),
    };
    /// <summary>기본 생성 전 부분 실패까지 같은 owner가 소유하며 실제 요청 크기에 맞는 파생만 할당한다.</summary>
    private uint Allocate(int size) => size == 0x128 ? _entry.AllocateConsole() : _tree.AllocateNode(size switch
    {
        0x13c => new HudChatNode(), 0x170 => new StoneButtonGumpNode(), 0xb8 => new GumpBackgroundNode(), 0xd8 => new ScrollingTextGumpNode(),
        _ => throw new ArgumentOutOfRangeException(nameof(size), "Chat 실제 할당 크기 미지원"),
    });
    /// <summary>기존 모든 파생 분배를 보존하고 같은 실제 Chat root의 가상 함수만 먼저 선택한다.</summary>
    private void BindVirtuals()
    {
        var attach = _tree.TreeHooks.Attach; _tree.TreeHooks.Attach = (token, x, y, parent, mode) => { if (Node(token) is not null) Window.Attach(token, x, y, parent, mode); else Need(attach)(token, x, y, parent, mode); };
        var events = _tree.EventHooks.Handle; _tree.EventHooks.Handle = (token, code, first, second) => Node(token) is not null ? Handle(token, code, first, second) : Need(events)(token, code, first, second);
        var input = _tree.InputHooks.Handle; _tree.InputHooks.Handle = (token, code, first, second) => Node(token) is not null ? Handle(token, code, first, second) : Need(input)(token, code, first, second);
        var destroy = _tree.LifetimeHooks.DeletingDestructor; _tree.LifetimeHooks.DeletingDestructor = (token, flags) => { if (Node(token) is not null) { Window.Destruct(token); if ((flags & 1) != 0) Need(_tree.LifetimeHooks.Release)(token); } else Need(destroy)(token, flags); };
    }
    /// <summary>문자열 삽입 사건에서만 현재 안정 payload 또는 실제 포인터를 해석한다.</summary>
    private uint Handle(uint token, uint code, uint first, uint second)
    {
        string? value = (code & 0x03ffffff) == Read(0x542b24) ? _globals.Strings.TryGetValue(second, out string? text) ? text : Need(_devices.Text)(second) : null; return Window.Handle(token, code, first, second, value);
    }
    /// <summary>전체 생성/자식 생성/부착 동안 같은 owner를 사용하고 controlled 메뉴는 그 범위 밖에서 생성한다.</summary>
    public uint Create() { ObjectDisposedException.ThrowIf(_disposed, this); return _tree.WithNodeOwner(this, Window.Create); }
    /// <summary>현재 실제 입력이 숨겨진 Enter만 표시/상위 사건으로 처리한다.</summary>
    public bool OpenInput(InputEvent input) => HudChatGeometry.DispatchChatInputKey(input, Geometry.InputVisible,
        () => Geometry.SetInputVisible(_globals.Current.Value, 1), () => Show());
    /// <summary>사용 시점의 현재 root를 같은 실제 부모 체인에 전달한다.</summary>
    public void Show() { uint token = _globals.Current.Value; if (token != 0) _tree.Events.Bubble(token, 0x02000088, 0, 1); }
    /// <summary>현재 root가 있는 경우 같은 실제 공유 버퍼/스타일/스크롤 부모에 추가한다.</summary>
    public void Print(string value) { uint token = _globals.Current.Value; if (token != 0) Window.AppendLog(token, value); }
    /// <summary>원문 clear는 완료 줄만 지우고 quit는 현재 전체 사건 DWORD를 실제 큐에 예약한다.</summary>
    public uint InvokeCommand(uint callback)
    {
        if (callback == 0x42f680) { uint token = _globals.Current.Value; if (token != 0) { var node = Node(token) ?? throw new InvalidOperationException("Chat clear 현재 실제 root 없음"); _text.Text.ReplaceText(node.Text, [0], 0); _text.Text.RequestBottom(node.Text); } return 0; }
        if (callback == 0x42e520) { Need(_devices.Quit)(Read(0x542944), 0, 0); return 0; } throw new InvalidOperationException("Chat 실제 명령 callback 미지원");
    }
    /// <summary>실제 등록표 순서로 자기 노드만 죽이고 기본 생성 전 부분 할당을 마지막으로 반납한다.공유 로그는 유지한다.</summary>
    public void Close()
    {
        // 자식 우선 Kill의 현재 dead 비트를 다시 읽어 중복 요청을 피한다.
        foreach (uint token in _tree.Create.Entries.Skip(1).ToArray())
            if (token != 0 && ReferenceEquals(_tree.NodeOwner(token), this) && _tree.Node(token) is { } node && (node.Flags & 0x10000) == 0) _tree.Lifetime.Kill(token);
        _tree.Lifetime.Cleanup();
        // 살아 있는 등록 객체가 있으면 조용히 버리지 않고 명시 진단한다.
        foreach (uint token in _tree.OwnedNodes(this)) { if (_tree.Create.Entries.Contains(token)) throw new InvalidOperationException("Chat 정리 뒤 등록 객체 잔류"); _tree.ReleaseUnregistered(token); }
    }
    /// <summary>표시 자식 정리가 성공한 뒤 같은 장기 owner의 공유 조각을 한 번 반납한다.</summary>
    public void Dispose() { if (_disposed) return; Close(); _runtime.ReleaseSharedLog(this, _text.Buffers); _disposed = true; }
}
