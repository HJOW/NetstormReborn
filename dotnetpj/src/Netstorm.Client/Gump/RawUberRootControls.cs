using System.Globalization;
using System.Text;
using Netstorm.Client.Display;
using Netstorm.Client.Input;

namespace Netstorm.Client.Gump; // 전체 HUD의 고정 파생을 같은 공통 트리/입력/수명에 조립한다.

/// <summary>Source RawUberRootContext와 실제 메뉴 소유자의 외부 장치를 공급한다. 생성은 Replace/Recreate까지 미룬다.</summary>
public sealed class RawUberRootContext
{
    /// <summary>같은 실제 화면/지도 전역.</summary>
    public required HudDockGlobals Globals { get; init; }
    /// <summary>실제 네 자식 공장·현재 전역·메시지 효과.</summary>
    public required UberRootHooks Root { get; init; }
    /// <summary>실제 dirty/화면 후처리/채팅 표시 장치.</summary>
    public required HudDockHooks Dock { get; init; }
    /// <summary>현재 실제 마우스 폴링.</summary>
    public required Func<uint, InputEvent> Poll { get; init; }
    /// <summary>현재 실제 커서 장치.</summary>
    public required Action<uint> Cursor { get; init; }
    /// <summary>같은 메뉴 처리기의 기본 Help.</summary>
    public required Func<uint, uint, uint, uint, uint> BaseHandle { get; init; }
    /// <summary>같은 메뉴 처리기의 실제 popup 정리.</summary>
    public required Action ClosePopups { get; init; }
    /// <summary>공통 등록0 교체 뒤 호스트의 현재 루트 별칭 반납.</summary>
    public Action<uint>? Replaced { get; init; }
    /// <summary>같은 실제 UI dirty 소유자 통지.</summary>
    public required Action Changed { get; init; }
    /// <summary>기존 실제 배경 GIF 공장의 Allocate. 메시지가 선택됐을 때만 필요하다.</summary>
    public Func<uint>? AllocateMessage { get; init; }
}

/// <summary>Source RawUberRootControls.cpp/h의 실제 전체 부모/도킹/입력/수명 조립이다.
/// 원본 본체를 반복 구현하지 않고 같은 GumpButtonTree의 실제 저장소와 가상 분배를 빌린다.</summary>
public sealed class RawUberRootControls : IDisposable
{
    /// <summary>같은 실제 노드/자산/등록/가상 분배 소유자.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>같은 현재 메뉴 인스턴스 owner.</summary>
    private readonly MenuGumpState _menus;
    /// <summary>실제 외부 장치와 같은 장기 지도 owner.</summary>
    private readonly RawUberRootContext _context;
    /// <summary>외부 함수 슬롯의 사본이며 현재 callback 상태는 그대로 공유한다.</summary>
    private readonly UberRootHooks _sourceRoot;
    /// <summary>세대별 실제 부모의 소유 토큰. 객체 자체는 공통 트리가 보유한다.</summary>
    private readonly SortedSet<uint> _roots = [];
    /// <summary>세대별 실제 구분선의 소유 토큰.</summary>
    private readonly SortedSet<uint> _dividers = [];
    /// <summary>같은 현재 공통 장치와 화면 효과.</summary>
    private readonly HudDockHooks _dock;
    /// <summary>같은 전체 구분선 생성/슬레이브/부착.</summary>
    private readonly HudDividerGump _divider;
    /// <summary>같은 실제 드래그/기본 Help/프레임 폴링.</summary>
    private readonly HudDividerInput _input;
    /// <summary>같은 전체 부모 생성/교체/재생성.</summary>
    private readonly UberRootGump _root;
    /// <summary>부분 생성의 미등록 물리를 실제 공통 저장소에 반납하는 이전 장치.</summary>
    private readonly Action<uint> _releaseBase;
    /// <summary>마지막 성공한 생성/재배치의 실제 화면 너비.</summary>
    private int _previousWidth;
    /// <summary>마지막 성공한 생성/재배치의 실제 화면 높이.</summary>
    private int _previousHeight;
    /// <summary>관리형 외부 분배 대여가 반납됐는지 나타낸다.</summary>
    private bool _disposed;
    /// <summary>선택된 실제 하위만 그 위치에서 요구한다.</summary>
    private static T Need<T>(T? hook) where T : Delegate => hook ?? throw new InvalidOperationException("전체 HUD 실제 외부 장치 미연결");
    /// <summary>실제 같은 트리에 가상 분배를 먼저 설치한다. 노드/장면/등록값을 생성하지 않는다.</summary>
    public RawUberRootControls(GumpButtonTree tree, MenuGumpState menus, RawUberRootContext context)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(menus); ArgumentNullException.ThrowIfNull(context);
        ArgumentNullException.ThrowIfNull(context.Globals); ArgumentNullException.ThrowIfNull(context.Root); ArgumentNullException.ThrowIfNull(context.Dock);
        if (!context.Globals.BorrowsView(tree.Create, tree.TreeState)) throw new ArgumentException("전체 HUD의 동일 화면/지도 owner 필요", nameof(context));
        _tree = tree; _menus = menus; _context = context; _sourceRoot = context.Root.CopyDevices(); _dock = context.Dock.CopyDevices(); _releaseBase = Need(tree.LifetimeHooks.Release);
        _dock.Node = tree.Node; _dock.Divider = Divider;
        _dock.ConstructBackground = (token, width, height, flags, type, frame) => tree.Backgrounds.Construct(token, width, height, flags, 1, type, frame);
        _dock.Activate = (token, enabled) => tree.Lifetime.SetOverlay(token, enabled != 0); _dock.BaseAttach = tree.Tree.Attach; _dock.Push = tree.Tree.Push;
        _dock.Attach = (token, x, y, parent, mode) => Need(tree.TreeHooks.Attach)(token, x, y, parent, mode); _dock.Move = tree.Tree.Move;
        _dock.ResolveSerial = Reference; _dock.Bubble = (token, code, first, second) => tree.Events.Bubble(token, code, first, second); _dock.ClosePopups = context.ClosePopups;
        _divider = new(() => tree.Create.ScreenWidth, () => tree.Create.ScreenHeight, () => 55, _dock);
        _input = new(tree.Collection, tree.Input, tree.Tree, new() { Divider = Divider, Base = tree.Node, Cursor = context.Cursor, Poll = context.Poll, BaseHandle = context.BaseHandle, MenuInstances = () => _menus.Instances });
        UberRootHooks effects = _sourceRoot.CopyDevices(); effects.Node = Root; effects.Divider = Divider; effects.Base = tree.Node; effects.Allocate = Allocate;
        effects.ConstructBase = (token, kind, width, height, flags) => tree.Factory.Construct(token, kind, width, height, flags);
        effects.ConstructDivider = (token, axis, width, height, flags, type, frame) => _divider.Construct(token, axis, width, height, flags, type, frame);
        effects.Attach = _dock.Attach; effects.Move = _dock.Move; effects.Push = _dock.Push; effects.RegisterSlave = _divider.RegisterSlave; effects.Cleanup = tree.Lifetime.Cleanup;
        effects.Overlay = tree.Lifetime.SetOverlay; effects.Kill = tree.Lifetime.Kill; effects.SetDepth = tree.Tree.SetDepth;
        effects.ReplaceRoot = token => { tree.Lifetime.ReplaceRoot(token); Need(context.Replaced)(token); context.Changed(); };
        _root = new(tree.Create, effects); Bind();
    }
    /// <summary>관리형 대여 반납 후 호출은 Source의 해제 객체 계약 밖이다.</summary>
    private void CheckAlive() => ObjectDisposedException.ThrowIf(_disposed, this);
    /// <summary>실제 소유 중인 같은 부모 주소만 조회한다.</summary>
    private UberRootNode? Root(uint token) => _roots.Contains(token) ? _tree.Node(token) as UberRootNode : null;
    /// <summary>실제 소유 중인 같은 구분선 주소만 조회한다.</summary>
    private HudDividerNode? Divider(uint token) => _dividers.Contains(token) ? _tree.Node(token) as HudDividerNode : null;
    /// <summary>공통 물리 반납 직전 자기 토큰 장부만 반납한다.</summary>
    private void Forget(uint token) { _roots.Remove(token); _dividers.Remove(token); }
    /// <summary>자기 노드가 없는 분배 대여는 약하게 보관해 닫힌 UI/World를 계속 보유하지 않는다.</summary>
    private void Bind()
    {
        var weak = new WeakReference<RawUberRootControls>(this);
        Func<uint, RawUberRootControls?> resolve = token => weak.TryGetTarget(out var owner) && !owner._disposed && owner.Node(token) is not null ? owner : null;
        var oldAttach = _tree.TreeHooks.Attach; var oldEvent = _tree.EventHooks.Handle; var oldInput = _tree.InputHooks.Handle; var oldBroadcast = _tree.FrameHooks.Broadcast;
        _tree.TreeHooks.Attach = (token, x, y, parent, mode) => { if (resolve(token) is { } owner) owner.Attach(token, x, y, parent, mode); else Need(oldAttach)(token, x, y, parent, mode); };
        _tree.EventHooks.Handle = (token, code, first, second) => resolve(token) is { } owner ? owner.Handle(token, code, first, second) : Need(oldEvent)(token, code, first, second);
        _tree.InputHooks.Handle = (token, code, first, second) => resolve(token) is { } owner ? owner.Handle(token, code, first, second) : Need(oldInput)(token, code, first, second);
        _tree.FrameHooks.Broadcast = (token, code, first, second) => { if (resolve(token) is { } owner) owner.Broadcast(token, code, first, second); else Need(oldBroadcast)(token, code, first, second); };
        Action<uint> release = _releaseBase;
        _tree.LifetimeHooks.Release = token => { if (weak.TryGetTarget(out var owner)) owner.Forget(token); release(token); };
    }
    /// <summary>생성 전에 같은 공통 저장소에 실제 파생을 확보한다. GIF는 기존 실제 공장을 사용한다.</summary>
    private uint Allocate(int bytes)
    {
        if (bytes == 0xb0) return Need(_context.AllocateMessage)();
        GumpCreateNode node = bytes switch { 0xc8 => new UberRootNode(), 0x11c => new HudDividerNode(), _ => throw new ArgumentOutOfRangeException(nameof(bytes), "전체 HUD 파생 할당 크기") };
        uint token = _tree.WithNodeOwner(this, () => _tree.AllocateNode(node)); if (node is UberRootNode) _roots.Add(token); else _dividers.Add(token); return token;
    }
    /// <summary>루트0을 제외한 고정999칸에서 현재 serial/비dead의 첫 일치만 해결한다.</summary>
    public uint Reference(uint serial)
    {
        // 일반 공통 Reference와 달리 루트/죽은 자식에 대체 연결하지 않는다.
        for (int index = 1; index < _tree.Create.Entries.Length; index++)
        {
            uint token = _tree.Create.Entries[index]; GumpCreateNode? node = token != 0 ? _tree.Node(token) : null;
            if (node is not null && node.Serial == serial && (node.Flags & 0x10000) == 0) return token;
        }
        return 0;
    }
    /// <summary>해당 실제 this의 현재 필드를 빌리는 짧은 도킹 실행기.</summary>
    private HudDock Dock(uint token) => new(_root.DockView(token), _context.Globals, () => 55, _dock);
    /// <summary>같은 전체 원문 공장과 기존 자식 이관/루트 교체를 실행한다.</summary>
    public void Replace() { CheckAlive(); _root.Replace(); _previousWidth = _tree.Create.ScreenWidth; _previousHeight = _tree.Create.ScreenHeight; }
    /// <summary>삭제 선행 전체 원문 생성과 같은 루트 별칭 갱신을 실행한다.</summary>
    public void Recreate() { CheckAlive(); _root.Recreate(); _previousWidth = _tree.Create.ScreenWidth; _previousHeight = _tree.Create.ScreenHeight; }
    /// <summary>실제 현재 전체 부모/필수 자식이 살아 있는지 읽는다. 따로 닫힌 미니맵의 옛 도킹 포인터는 구별한다.</summary>
    public bool Current()
    {
        if (_disposed) return false; UberRootNode? node = Root(_tree.Create.Entries[0]); if (node is null || (node.Flags & 0x10000) != 0) return false;
        // 필수 자식의 현재 주소를 읽으며 부분 생성/반납을 완성한 UI로 판정하지 않는다.
        foreach (uint token in new[] { node.Vertical, node.MinimapDivider, node.ChatDivider, node.Minimap, node.Production, node.Knowledge, node.Chat })
        {
            GumpCreateNode? child = token != 0 ? _tree.Node(token) : null;
            if (child is null || (child.Flags & 0x10000) != 0)
            {
                if (token != 0 && token == node.Minimap && _sourceRoot.Read is not null && _sourceRoot.Read(0x5949d0) != token) continue;
                return false;
            }
        }
        return true;
    }
    /// <summary>현재 소유 기본/파생 주소만 공통 조회에 빌린다.</summary>
    public GumpCreateNode? Node(uint token) => Root(token) ?? (GumpCreateNode?)Divider(token);
    /// <summary>같은 구분선 배경을 사본 없이 빌린다.</summary>
    public GumpBackgroundNode? Background(uint token) => Divider(token);
    /// <summary>부모는 공통 Attach, 구분선은 전체 파생 Attach를 사용한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        CheckAlive(); if (Divider(token) is not null) _divider.Attach(token, x, y, parent, mode);
        else if (Root(token) is not null) _tree.Tree.Attach(token, x, y, parent, mode);
        else throw new ArgumentOutOfRangeException(nameof(token), "전체 HUD 부착 대상 없음");
    }
    /// <summary>구분선은 실제 드래그/Help, 부모는 해당 this 도킹/기본 Help로 분배한다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second)
    {
        CheckAlive(); if (Divider(token) is not null) return _input.Handle(token, code, first, second); if (Root(token) is null) throw new ArgumentOutOfRangeException(nameof(token), "전체 HUD 입력 대상 없음");
        _dock.BaseHandle = (eventCode, a, b) => _context.BaseHandle(token, eventCode, a, b); return Dock(token).Handle(code, first, second);
    }
    /// <summary>구분선만 실제 폴링하며 부모의 Source 기본 방송은 빈 본문이다.</summary>
    public void Broadcast(uint token, uint code, uint first, uint second) { CheckAlive(); if (Divider(token) is not null) _input.Broadcast(token, code, first, second); }
    /// <summary>두 실제 파생의 원문 공통 scalar 소멸자를 호출한다.</summary>
    public void DeleteScalar(uint token, uint flags) { CheckAlive(); _tree.Lifetime.DeleteScalar(token, flags); }
    /// <summary>실제 scalar/부분 할당 반납 위치에서만 자기 저장소를 반납한다. 공통 등록 해제는 호출자의 책임이다.</summary>
    public void Release(uint token)
    {
        if (!_roots.Contains(token) && !_dividers.Contains(token)) return; Forget(token); _releaseBase(token);
    }
    /// <summary>현재 출력 대여는 부모의 빈 Draw/구분선의 실제 배경 Draw만 담당한다.</summary>
    public bool TryDraw(uint token, GumpBackgroundDraw background)
    {
        ArgumentNullException.ThrowIfNull(background); if (_disposed) return false;
        if (Divider(token) is not null) { background.Draw(token); return true; } return Root(token) is not null;
    }
    /// <summary>원본 고정 화면 본체와 별도인 Windows 크기 변경 재배치다. 현재 하단 간격과 숨긴 채팅 복원 위치를 유지한다.</summary>
    public void Reflow()
    {
        CheckAlive(); if (!Current()) throw new InvalidOperationException("실제 전체 HUD 없는 화면 재배치"); int width = _tree.Create.ScreenWidth, height = _tree.Create.ScreenHeight; if (width == _previousWidth && height == _previousHeight) return;
        UberRootNode node = Root(_tree.Create.Entries[0])!; double delta = (double)height - _previousHeight; float miniY = (float)(Divider(node.MinimapDivider)!.Y + delta), chatY = (float)(Divider(node.ChatDivider)!.Y + delta);
        if (Divider(node.ChatDivider)!.Y >= _previousHeight) _context.Globals.SavedChatY = (float)((double)_context.Globals.SavedChatY + delta);
        HudDividerNode vertical = Divider(node.Vertical)!; vertical.BaseHeight = height; vertical.Height = (float)((double)height * vertical.Scale);
        HudDividerNode mini = Divider(node.MinimapDivider)!; mini.MinY = unchecked(height - 123); mini.MaxY = height; Divider(node.ChatDivider)!.MaxY = unchecked(height - 25);
        _tree.Lifetime.Cleanup(); _tree.Tree.Move(node.Vertical, vertical.X, vertical.Y, 0); _tree.Lifetime.Cleanup();
        _tree.Tree.Move(node.MinimapDivider, Divider(node.MinimapDivider)!.X, miniY, 0); _tree.Lifetime.Cleanup();
        _tree.Tree.Move(node.ChatDivider, Divider(node.ChatDivider)!.X, chatY, 0); _tree.Lifetime.Cleanup();
        _previousWidth = width; _previousHeight = height; Need(_dock.FinishResize)(); _context.Changed();
    }
    /// <summary>검사 입력용 현재 표시 구분선의 실제 경계 중앙을 읽기만 한다.</summary>
    public ScreenPoint? Point(string label)
    {
        if (!Current()) return null; UberRootNode node = Root(_tree.Create.Entries[0])!; uint token = label switch { "hud:vertical" => node.Vertical, "hud:minimap-divider" => node.MinimapDivider, "hud:chat-divider" => node.ChatDivider, _ => 0 };
        if (token == 0 || (Divider(token)!.Flags & 1) == 0) return null; ScreenRect bounds = _tree.Collection.Bounds(token); return new(unchecked(bounds.Left + bounds.Right) / 2, unchecked(bounds.Top + bounds.Bottom) / 2);
    }
    /// <summary>원문 진단의 현재 unsigned 숫자를 문화권 없이 기록한다.</summary>
    private static string U(uint value) => value.ToString(CultureInfo.InvariantCulture);
    /// <summary>원문 진단의 현재 signed 숫자를 문화권 없이 기록한다.</summary>
    private static string I(int value) => value.ToString(CultureInfo.InvariantCulture);
    /// <summary>C++ 기본6자리 진단 정밀도로 실제 단정도만 기록한다.</summary>
    private static string F(float value) => value.ToString("G6", CultureInfo.InvariantCulture).Replace('E', 'e');
    /// <summary>현재 실제 부모/구분선/지도 상태만 Source의 TSV 순서로 읽는다.</summary>
    public string Report()
    {
        HudDockGlobals globals = _context.Globals; var text = new StringBuilder();
        text.AppendJoin('\t', "raw_uber", U(_tree.Create.Entries[0]), I(_roots.Count), I(_dividers.Count), I(globals.MapRect.Left), I(globals.MapRect.Top), I(globals.MapRect.Right), I(globals.MapRect.Bottom)).Append('\n');
        // 실제 부모의 현재 자식 필드만 주소 순서로 관찰한다.
        foreach (uint token in _roots)
        {
            UberRootNode node = Root(token)!; text.AppendJoin('\t', "raw_uber_children", U(token), U(node.Vertical), U(node.MinimapDivider), U(node.ChatDivider), U(node.Minimap), U(node.Production), U(node.Knowledge), U(node.Chat)).Append('\n');
        }
        // 실제 구분선의 현재 기하/제한/상태를 주소 순서로 관찰한다.
        foreach (uint token in _dividers)
        {
            HudDividerNode node = Divider(token)!; text.AppendJoin('\t', "raw_hud_divider", U(token), U(node.Serial), F(node.X), F(node.Y), F(node.BaseWidth), F(node.BaseHeight), U(node.Axis), I(node.MinX), I(node.MaxX), I(node.MinY), I(node.MaxY), U(node.Flags)).Append('\n');
        }
        return text.ToString();
    }
    /// <summary>자기 파생의 실제 등록/부분 생성 물리를 정리하고 다른 UI의 저장소와 분배는 보존한다.</summary>
    public void Dispose()
    {
        if (_disposed) return; ReleaseSet(_roots); ReleaseSet(_dividers); _disposed = true;
    }
    /// <summary>등록된 노드는 실제 자식 우선 수명, 미등록 부분 할당은 Source처럼 물리만 반납한다.</summary>
    private void ReleaseSet(SortedSet<uint> tokens)
    {
        // 원문 정렬 저장소의 첫 항목을 실제 반납 뒤에만 다음으로 진행한다.
        while (tokens.Count != 0)
        {
            uint token = tokens.Min; GumpCreateNode node = _tree.Node(token) ?? throw new InvalidOperationException("HUD 소유 장부/공통 저장소 불일치");
            if (node.Id < _tree.Create.Entries.Length && _tree.Create.Entries[node.Id] == token) { _tree.Lifetime.Kill(token); _tree.Lifetime.Cleanup(); }
            else Release(token);
        }
    }
}
