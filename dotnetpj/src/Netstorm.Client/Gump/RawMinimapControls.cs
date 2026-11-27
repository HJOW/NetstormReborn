using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Client.Input;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 실제 물리와 공통 트리에 Sky Overview를 조립한다.

/// <summary>현재 카메라/선택/입력과 실제 공통 UI 외부 장치다. 상태는 매 호출 시 현재 owner에서 읽는다.</summary>
public sealed class RawMinimapHooks
{
    /// <summary>같은 현재 카메라와 실제 지도 표시 영역이다.</summary>
    public required Func<MinimapGeometryView> View { get; init; }
    /// <summary>00497f20의 실제 현재 카메라 중심 변경이다.</summary>
    public required Action<float, float> Center { get; init; }
    /// <summary>0042a360/004d5bb0의 현재 선택 이동이다.</summary>
    public required Action<float, float> Move { get; init; }
    /// <summary>같은 현재 입력 모드2/5를 읽는다.</summary>
    public required Func<uint> InputMode { get; init; }
    /// <summary>원본ffff 마우스 폴링의 현재 사건이다.</summary>
    public required Func<InputEvent> Poll { get; init; }
    /// <summary>같은 메뉴 처리기의 실제 기본 Help다.</summary>
    public required Func<uint, uint, uint, uint, uint> BaseHandle { get; init; }
    /// <summary>원본 기본 GumpRedraw가 요청한 실제 사각형/flags를 받는다.</summary>
    public required Action<ScreenRect, uint> Invalidate { get; init; }
    /// <summary>크기/부착의 실제 공통 UI 구조 변경 통지다.</summary>
    public required Action Changed { get; init; }
}

/// <summary>개별 노드보다 오래 사는005949d0/005949dc와 실제 미니맵 전용 색이다.</summary>
public sealed class RawMinimapGlobals
{
    /// <summary>성공한 마지막 실제 공통 노드 토큰이다.</summary>
    public uint Current { get; set; }
    /// <summary>성공한 마지막 기본 생성자의 현재 serial이다.</summary>
    public uint Serial { get; set; }
    /// <summary>모든 같은 미니맵 셀 그림이 빌리는 현재 색/모드 전역이다.</summary>
    public MinimapRasterState Colors { get; } = new();
}

/// <summary>cpppj/src/client/RawMinimapControls.cpp/h의 전체 현재 입력/표시/물리 통지와 수명 조립이다.</summary>
public sealed class RawMinimapControls : IDisposable
{
    /// <summary>원본 minmapGump의 파생 가상표다.</summary>
    private const uint MinimapVtable = 0x50d51c;
    /// <summary>같은 실제 등록/입력/수명과 출력 소유자다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>Session보다 오래 사는 실제 물리이며 현재 World는 보유하지 않는다.</summary>
    private readonly RawSurfaceStorage _storage;
    /// <summary>같은 공유 bitmap/큐/갱신 DWORD다.</summary>
    private readonly MetadisplayDirtyState _state;
    /// <summary>노드보다 오래 사는 같은 현재 포인터/serial/색이다.</summary>
    private readonly RawMinimapGlobals _globals;
    /// <summary>현재 외부 owner의 실제 장치다.</summary>
    private readonly RawMinimapHooks _hooks;
    /// <summary>한 처리기가 반복 생성할 때도 같은 실제 기본 주소다.</summary>
    private readonly GumpCreateNode _node = new();
    /// <summary>현재 공통 소유 토큰이며 scalar 반납 뒤에만0이 된다.</summary>
    private uint _token;
    /// <summary>현재 노드의 crop/드래그 상태이며 bitmap 수명과 구별한다.</summary>
    private MinimapGeometryState _geometry = new();
    /// <summary>같은 현재 raw 월드를 읽는 기존 셀 그림 본체다.</summary>
    private readonly RawMinimapRaster _raster;
    /// <summary>같은 공유 상태의 기존 갱신 큐다.</summary>
    private readonly RawMetadisplayDirty _dirty;
    /// <summary>현재 기본 영역과 실제 지도 owner를 읽는 원문 재그리기다.</summary>
    private readonly GumpRedraw _redraw;
    /// <summary>현재 World의 Pop을 짧게 빌린다. UI가 World의 수명을 늘리지 않는다.</summary>
    private WeakReference<RawPop>? _pop;
    /// <summary>현재 World의 Unpop을 같은 짧은 대여로 빌린다.</summary>
    private WeakReference<RawUnpop>? _unpop;
    /// <summary>자기 외부 출력만 반납하는 실제 등록 번호다.</summary>
    private readonly uint _drawRegistration;
    /// <summary>공통 기본 물리 반납의 이전 장치다.</summary>
    private readonly Action<uint> _releaseBase;
    /// <summary>현재 Refresh가 실제 재그리기/셀 쓰기를 했는지다.</summary>
    private bool _modified;
    /// <summary>외부 장치 대여가 반납됐는지다.</summary>
    private bool _disposed;
    /// <summary>선택된 공통 장치 누락을 실제 호출 위치에서 거부한다.</summary>
    private static T Need<T>(T? device) where T : Delegate => device ?? throw new InvalidOperationException("미니맵 실제 공통 장치 미연결");
    /// <summary>실제 Session과 같은 물리/현재 Core의 통지까지 설치한다. Session 객체를 보유하지 않는다.</summary>
    public RawMinimapControls(GumpButtonTree tree, RawCampaignSession session, MetadisplayDirtyState state, RawMinimapGlobals globals, UIntBox fullRefresh, RawMinimapHooks hooks)
        : this(tree, (session ?? throw new ArgumentNullException(nameof(session))).Storage(), state, globals, fullRefresh, hooks)
    {
        BindWorld(session.Core());
    }
    /// <summary>Session 이전에도 같은 장기 물리/현재 전역으로 조립한다. fullRefresh에는 실제 Gameplay.MapRevision을 공급한다.</summary>
    public RawMinimapControls(GumpButtonTree tree, RawSurfaceStorage storage, MetadisplayDirtyState state, RawMinimapGlobals globals, UIntBox fullRefresh, RawMinimapHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(globals); ArgumentNullException.ThrowIfNull(fullRefresh); ArgumentNullException.ThrowIfNull(hooks);
        if (storage.Pool.Edition != OriginalEdition.Patch1078 || storage.Types.Count == 0) throw new ArgumentException("미니맵의 실제10.78 물리/TYPE 저장소 필요", nameof(storage));
        if (hooks.View is null || hooks.Center is null || hooks.Move is null || hooks.InputMode is null || hooks.Poll is null || hooks.BaseHandle is null || hooks.Invalidate is null || hooks.Changed is null) throw new ArgumentException("실제 미니맵 장치 연결 누락", nameof(hooks));
        if (state.SharedFullRefresh is not null && !ReferenceEquals(state.SharedFullRefresh, fullRefresh)) throw new ArgumentException("미니맵의 동일 MapRevision owner 필요", nameof(fullRefresh));
        _tree = tree; _storage = storage; _state = state; _globals = globals; _hooks = hooks; _releaseBase = Need(tree.LifetimeHooks.Release);
        state.SharedFullRefresh = fullRefresh;
        _raster = new(storage.Pool, storage.Hash, storage.Types, globals.Colors);
        _redraw = new(tree.Collection, new() { Node = tree.Node, Map = () => tree.TreeState.MapView, Invalidate = hooks.Invalidate });
        _dirty = new(storage.Pool, state, new()
        {
            Rebuild = () => { SyncSize(); if (MinimapGeometry.Rebuild(_geometry, _hooks.View())) { _redraw.Whole(_token); _modified = true; } },
            RedrawCell = (x, y) => { globals.Colors.Challenge = storage.OwnerMode.Challenge ? 1U : 0U; globals.Colors.Frame = state.Frames; _raster.DrawCell(state.Bitmap, x, y); _modified = true; },
        });
        var weak = new WeakReference<RawMinimapControls>(this);
        Func<uint, RawMinimapControls?> resolve = token => weak.TryGetTarget(out var owner) && !owner._disposed && owner.Node(token) is not null ? owner : null;
        _drawRegistration = tree.ExternalDrawing.RegisterExternalGumps(new() { Owns = token => resolve(token) is not null, Draw = (token, target, _, _) => resolve(token)!.Draw(target) });
        Bind(resolve, weak);
    }
    /// <summary>자기 토큰만 실제 가상 부착/사건/idle/물리 반납에 연결한다. 다른 UI 분배는 그대로 둔다.</summary>
    private void Bind(Func<uint, RawMinimapControls?> resolve, WeakReference<RawMinimapControls> weak)
    {
        var attach = _tree.TreeHooks.Attach; var events = _tree.EventHooks.Handle; var input = _tree.InputHooks.Handle; var idle = _tree.FrameHooks.Idle; var release = _releaseBase;
        _tree.TreeHooks.Attach = (token, x, y, parent, mode) => { if (resolve(token) is { } owner) owner.Attach(token, x, y, parent, mode); else Need(attach)(token, x, y, parent, mode); };
        _tree.EventHooks.Handle = (token, code, first, second) => resolve(token) is { } owner ? owner.Handle(token, code, first, second) : Need(events)(token, code, first, second);
        _tree.InputHooks.Handle = (token, code, first, second) => resolve(token) is { } owner ? owner.Handle(token, code, first, second) : Need(input)(token, code, first, second);
        _tree.FrameHooks.Idle = token => { if (resolve(token) is { } owner) owner.Idle(token); else Need(idle)(token); };
        _tree.LifetimeHooks.Release = token => { if (weak.TryGetTarget(out var owner)) owner.Forget(token); release(token); };
    }
    /// <summary>관리형 대여 반납 뒤의 사용을 거부한다.</summary>
    private void CheckAlive() => ObjectDisposedException.ThrowIf(_disposed, this);
    /// <summary>현재 실제 기본 노드의 원본 크기만 기하에 전달한다.</summary>
    private void SyncSize() { _geometry.Width = _node.BaseWidth; _geometry.Height = _node.BaseHeight; }
    /// <summary>동일한 현재 물리/모듈 통지만 설치하며 별도 World/bitmap을 만들지 않는다.</summary>
    public void BindWorld(RawWorldCore core)
    {
        CheckAlive(); ArgumentNullException.ThrowIfNull(core);
        if (_pop is not null || _unpop is not null || !ReferenceEquals(core.Pool, _storage.Pool) || !ReferenceEquals(core.Hash, _storage.Hash) || !ReferenceEquals(core.Unpop.Pool, _storage.Pool) || !ReferenceEquals(core.OwnerMode, _storage.OwnerMode) || !ReferenceEquals(core.Types, _storage.Types) || !ReferenceEquals(core.Spots, _storage.Spots)) throw new ArgumentException("미니맵의 현재 월드 물리/대여 불일치", nameof(core));
        BindPair(core.Pop, core.Unpop);
    }
    /// <summary>장기 저장소의 실제 Pop/Unpop 실행기에 같은 미니맵 큐를 연결한다.</summary>
    /// <remarks>제품 HostSession은 RawWorldCore 대신 같은 저장소 실행기를 사용한다. 별도 세션 전용 실행기는 해당 소유자에 연결해야 한다.</remarks>
    public void BindStorageExecutors()
    {
        CheckAlive();
        if (_pop is not null || _unpop is not null) throw new InvalidOperationException("미니맵 월드 대여 중복");
        BindPair(_storage.PopExecutor(), _storage.UnpopExecutor());
    }
    /// <summary>검증된 같은 물리의 두 실행기에 큐를 설치하고 전체 재그리기를 예약한다.</summary>
    private void BindPair(RawPop pop, RawUnpop unpop)
    {
        _pop = new(pop); _unpop = new(unpop); pop.SetMetadisplay(_dirty); unpop.SetMinimapQueue(_dirty); _state.FullRefresh = 1;
    }
    /// <summary>현재 자기 통지만 expected owner 확인으로 반납하고 장기 물리/bitmap은 유지한다.</summary>
    public void UnbindWorld()
    {
        CheckAlive(); if (_pop is not null && _pop.TryGetTarget(out var pop)) pop.ReleaseMetadisplay(_dirty); if (_unpop is not null && _unpop.TryGetTarget(out var unpop)) unpop.ReleaseMinimapQueue(_dirty); _pop = null; _unpop = null;
    }
    /// <summary>004815b0→004801b0의 현재 bitmap 폭/부모0 생성 뒤 Help와 현재 serial/표시를 게시한다.</summary>
    public uint Construct()
    {
        CheckAlive(); if (_token != 0) throw new InvalidOperationException("현재 미니맵 중복 생성");
        _token = _tree.WithNodeOwner(this, () => _tree.AllocateNode(_node)); float width = _state.BitmapWidth;
        _tree.Factory.Construct(_token, 0, width, width, 0x301); _node.Vtable = MinimapVtable; _node.Command = 29; _geometry = new();
        _tree.SetHelpText(_token, "Sky Overview"); _tree.SetHelpWord(_token, "minimapGump"); _globals.Serial = _node.Serial; _globals.Current = _token;
        _state.DisplayPresent = true; _state.DisplayFlags = unchecked((byte)_node.Flags); return _token;
    }
    /// <summary>현재 같은 공통 노드 토큰이다.</summary>
    public uint RootToken => _token;
    /// <summary>bitmap 최초 준비→생성→128크기/현재 표시/갱신→실제 기본 부착 순서다.</summary>
    public void Open()
    {
        CheckAlive(); _dirty.Refresh(default); Construct(); _node.BaseWidth = _node.Width = 128; _node.BaseHeight = _node.Height = 128;
        _state.DisplayPresent = true; _state.DisplayFlags = unchecked((byte)_node.Flags); _state.FullRefresh = 1; Attach(_token, 0, 0, _tree.Create.Entries[0], 0); _node.Flags |= 0x20000;
    }
    /// <summary>자기 등록과 캡처/Idle을 같은 자식 우선 수명으로 반납한다. 부분 생성은 자기 물리만 반납한다.</summary>
    public void Close()
    {
        CheckAlive(); if (_token == 0) return; uint token = _token;
        if (_node.Id < _tree.Create.Entries.Length && _tree.Create.Entries[_node.Id] == token) { _tree.Lifetime.Kill(token); _tree.Lifetime.Cleanup(); } else Release(token);
    }
    /// <summary>정상 mode3의 기본 도킹(0,H-75),80×75를 실제 부착한다. 전체 도킹 드래그는 기존 HudDock가 맡는다.</summary>
    public void Resize(int screenHeight)
    {
        CheckAlive(); if (_token == 0 || screenHeight <= 80) throw new ArgumentException("미니맵 실제 화면 높이/수명 오류"); float y = screenHeight - 75;
        if (_node.BaseWidth == 80 && _node.BaseHeight == 75 && _node.X == 0 && _node.Y == y) return;
        _node.BaseWidth = _node.Width = 80; _node.BaseHeight = _node.Height = 75; _tree.Tree.Push(_token); Attach(_token, 0, y, _tree.Create.Entries[0], 0); _hooks.Changed();
    }
    /// <summary>현재 파일 팔레트의 정확한 미니맵 전용 색 순서를 사용하며 실제 변경 때만 같은DWORD를 증가시킨다.</summary>
    public void SetPalette(PaletteColorTable palette)
    {
        CheckAlive(); ArgumentNullException.ThrowIfNull(palette); if (palette.Named.Count < 20) throw new ArgumentException("10.78 미니맵 파일 팔레트 색 전역 누락", nameof(palette));
        var colors = _globals.Colors; uint[] owners = [palette.Named[11].Index, palette.Basic[1], palette.Basic[7], palette.Named[10].Index, palette.Basic[4], palette.Basic[5], palette.Named[12].Index, palette.Basic[6]];
        bool changed = colors.Black != palette.Basic[0] || !colors.Owners.AsSpan().SequenceEqual(owners) || colors.NeutralIsle != palette.Named[18].Index || colors.NeutralNoIsland != palette.Named[19].Index || colors.ChallengeGreen != palette.Basic[2];
        colors.Black = palette.Basic[0]; owners.CopyTo(colors.Owners, 0); colors.NeutralIsle = palette.Named[18].Index; colors.NeutralNoIsland = palette.Named[19].Index; colors.ChallengeGreen = palette.Basic[2];
        if (changed) _state.FullRefresh = unchecked(_state.FullRefresh + 1);
    }
    /// <summary>현재 표시 토큰일 때만 실제 현재 view/flags를 읽고 커널 앞 미니맵 갱신을 수행한다.</summary>
    public bool Refresh()
    {
        CheckAlive(); if (_token == 0 || _globals.Current != _token) return false; var view = _hooks.View(); _modified = false; _state.DisplayPresent = true; _state.DisplayFlags = unchecked((byte)_node.Flags);
        _dirty.Refresh(new() { CameraX = view.CameraX, CameraY = view.CameraY, Left = view.Viewport.Left, Top = view.Viewport.Top, Right = view.Viewport.Right, Bottom = view.Viewport.Bottom }); return _modified;
    }
    /// <summary>같은 현재 공통 저장소에 살아 있는 자기 기본 주소만 반환한다.</summary>
    public GumpCreateNode? Node(uint token) => token != 0 && token == _token && ReferenceEquals(_tree.Node(token), _node) ? _node : null;
    /// <summary>실제 자기 주소를 요구하며 다른 UI 토큰을 대신 처리하지 않는다.</summary>
    private void RequireNode(uint token) { CheckAlive(); if (Node(token) is null) throw new ArgumentOutOfRangeException(nameof(token), "미니맵 실제 노드 없음"); }
    /// <summary>현재 실제 크기 동기화→crop 보정→같은 기본 부착 순서다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        RequireNode(token); SyncSize(); MinimapGeometry.ClampAttachment(_geometry); _tree.Tree.Attach(token, x, y, parent, mode);
    }
    /// <summary>00481620. 사건 검사 전 로컬 변환, 선택 이동 또는 카메라/기본 영역/캡처/Idle과 기본 Help 순서다.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        RequireNode(token); SyncSize(); var local = _tree.Collection.ScreenToLocal(token, new(unchecked((int)first), unchecked((int)second)));
        if ((eventCode & 0x03ffffff) == 0x10000)
        {
            if ((eventCode & 0x80000000) != 0) { _geometry.Scrolling = 0; _tree.Lifetime.SetIdle(token, false); return 1; }
            var target = MinimapGeometry.Target(_geometry, local.X, local.Y); uint mode = _hooks.InputMode();
            if (mode is 2 or 5) { _hooks.Move(target.X, target.Y); return 1; }
            _hooks.Center(target.X, target.Y); _redraw.Whole(token); _tree.Input.Capture(token); _geometry.Scrolling = 1; _tree.Lifetime.SetIdle(token, true);
        }
        return _hooks.BaseHandle(token, eventCode, first, second);
    }
    /// <summary>00480440. 실제 폴링/로컬 변환→바깥 crop 이동→현재 카메라→기본 영역 요청이다.</summary>
    public void Idle(uint token)
    {
        RequireNode(token); if (_geometry.Scrolling == 0) throw new InvalidOperationException("미니맵 드래그Idle 상태 오류"); SyncSize(); var mouse = _hooks.Poll(); var local = _tree.Collection.ScreenToLocal(token, new(mouse.X, mouse.Y)); var target = MinimapGeometry.Drag(_geometry, local.X, local.Y); _hooks.Center(target.X, target.Y); _redraw.Whole(token);
    }
    /// <summary>같은 기본 scalar 소멸자를 실행하며 공유 bitmap은 반납하지 않는다.</summary>
    public void DeleteScalar(uint token, uint flags) { RequireNode(token); _tree.Lifetime.DeleteScalar(token, flags); }
    /// <summary>실제 물리 반납 직전 자기 토큰과 현재 표시 관찰만 끊는다. 이전 표시가 새 표시를 지우지 않는다.</summary>
    private void Forget(uint token) { if (token != _token || token == 0) return; _token = 0; if (_globals.Current == token) _state.DisplayPresent = false; }
    /// <summary>등록된 scalar 반납 또는 미등록 부분 생성 정리가 끝난 자기 공통 물리만 반납한다.</summary>
    public void Release(uint token) { if (token == _token && token != 0) { Forget(token); _releaseBase(token); } }
    /// <summary>현재 같은 crop/위치/Bounds로 평면 bitmap과 원본 위/아래/왼쪽/오른쪽 외곽선을 그린다.</summary>
    private void Draw(GumpPixelTarget target)
    {
        CheckAlive(); if (_state.Bitmap.Length == 0) return; SyncSize(); var plan = MinimapGeometry.PlanDraw(_geometry, _hooks.View(), _tree.Collection.Position(_token)); var bounds = _tree.Collection.Bounds(_token);
        target.RawBitmap(_state.Bitmap, 128, (long)plan.Source.Y * 128 + plan.Source.X, plan.Size, plan.Destination, bounds);
        int left = plan.Camera.Left, top = plan.Camera.Top, right = unchecked(plan.Camera.Right - 1), bottom = unchecked(plan.Camera.Bottom - 1); uint color = _globals.Colors.Owners[2];
        target.LinesClipped([new(new(left, top), new(right, top), color), new(new(left, bottom), new(right, bottom), color), new(new(left, top), new(left, bottom), color), new(new(right, top), new(right, bottom), color)], bounds);
    }
    /// <summary>자동 입력용 현재 실제 경계의 상대점을 읽기만 한다.</summary>
    public ScreenPoint? Point(string label)
    {
        if (_token == 0 || !label.StartsWith("minimap:", StringComparison.Ordinal)) return null; int comma = label.IndexOf(','); if (comma < 0) return null; var origin = _tree.Collection.Position(_token);
        return new(unchecked(origin.X + int.Parse(label.AsSpan(8, comma - 8), CultureInfo.InvariantCulture)), unchecked(origin.Y + int.Parse(label.AsSpan(comma + 1), CultureInfo.InvariantCulture)));
    }
    /// <summary>현재 등록/경계/crop/큐와 전체 bitmap FNV32를 원문TSV 순서로 읽기만 한다.</summary>
    public string Report()
    {
        if (_token == 0) return ""; var bounds = _tree.Collection.Bounds(_token); uint hash = 2166136261;
        // 초기화/셀 그림을 구별하도록 모든 실제BYTE를 포함한다.
        foreach (byte value in _state.Bitmap) hash = unchecked((hash ^ value) * 16777619);
        return FormattableString.Invariant($"minimap\t{_node.Id}\t{bounds.Left}\t{bounds.Top}\t{unchecked(bounds.Right - bounds.Left)}\t{unchecked(bounds.Bottom - bounds.Top)}\t{_geometry.CropX}\t{_geometry.CropY}\t{_geometry.Scrolling}\t{_state.Frames}\t{_state.Count}\t{_state.FullRefresh}\t{hash}");
    }
    /// <summary>노드→현재 World 통지→자기 외부 출력 순서로 반납하며 장기 상태/물리는 유지한다.</summary>
    public void Dispose()
    {
        if (_disposed) return; Close(); UnbindWorld(); _tree.ExternalDrawing.UnregisterExternalGumps(_drawRegistration); _disposed = true;
    }
}
