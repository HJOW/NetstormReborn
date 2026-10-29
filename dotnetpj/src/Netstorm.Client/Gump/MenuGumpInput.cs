using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 실제 메뉴 선택·명령·하위 메뉴·변경 영역·팝업 수명 입력 영역이다.

/// <summary>실제 문자열 사건 초기화/메뉴 종류 전역.</summary>
public sealed class MenuGumpInputState
{
    /// <summary>기본 command에 signed 확장한 메뉴 종류 BYTE.</summary>
    public sbyte Alignment { get; set; } = 26;
    /// <summary>동적 하위 메뉴 요청 ID.</summary>
    public uint RequestEvent { get; set; }
    /// <summary>메뉴 닫힘 부모 통지 ID.</summary>
    public uint ClosedEvent { get; set; }
}
/// <summary>같은 실제 노드/가상 사건·트리와 명령/이름 소리/변경 영역 경계.</summary>
public sealed class MenuGumpInputHooks
{
    /// <summary>현재 실제 메뉴.</summary>
    public Func<uint, MenuGumpNode?>? Node { get; set; }
    /// <summary>같은 기본/부모/팝업.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 가상 선택 통지.</summary>
    public Action<uint, uint, uint, uint>? Broadcast { get; set; }
    /// <summary>현재 가상 동적 자식 요청 처리.</summary>
    public Func<uint, uint, uint, uint, uint>? Handle { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>현재 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 가상 Redraw(false).</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>실제 명령/등록 ID/전체 인자 큐.</summary>
    public Action<uint, uint, uint>? Command { get; set; }
    /// <summary>원본 파일 이름의 현재 소리 재생 요청.</summary>
    public Action<string>? Sound { get; set; }
    /// <summary>끝 제외 현재 변경 영역.</summary>
    public Action<ScreenRect>? Invalidate { get; set; }
}
/// <summary>cpppj/src/client/MenuGumpInput.cpp의 콜백 뒤 현재 선택/flags/링크 재조회와 숨김·지연 삭제 순서.</summary>
public sealed class MenuGumpInput
{
    /// <summary>현재 실제 등록/판본.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 종류/사건 전역.</summary>
    private readonly MenuGumpInputState _state;
    /// <summary>같은 실제 메뉴 위치/확장.</summary>
    private readonly MenuGump _menu;
    /// <summary>같은 실제 위치/화면 역변환.</summary>
    private readonly GumpCollection _collection;
    /// <summary>같은 실제 부모 체인 통지.</summary>
    private readonly GumpEvents _events;
    /// <summary>같은 실제 지연 삭제.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>같은 실제 깊이 갱신.</summary>
    private readonly GumpTree _tree;
    /// <summary>현재 공유 노드/외부 가상 경계.</summary>
    private readonly MenuGumpInputHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("메뉴 입력 경계 필요");
    /// <summary>실제 같은 기본/파생 저장소와 기존 처리기를 빌린다.</summary>
    public MenuGumpInput(GumpCreateState create, MenuGumpInputState state, MenuGump menu, GumpCollection collection, GumpEvents events, GumpLifetime lifetime, GumpTree tree, MenuGumpInputHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(menu); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _create = create; _state = state; _menu = menu; _collection = collection; _events = events; _lifetime = lifetime; _tree = tree; _hooks = hooks; }
    /// <summary>현재 기본 노드는 메뉴 아닌 부모/팝업도 허용한다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("메뉴 입력 기본 노드 없음");
    /// <summary>현재 메뉴와 기본 객체가 같은지 확인한다.</summary>
    private MenuGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, Base(token))) throw new InvalidOperationException("메뉴 입력 공유 노드 없음"); return n; }
    /// <summary>현재 선택의 실제40칸 접근만 검사한다.사용하지 않는 유효 칸도 허용한다.</summary>
    private MenuGumpItem Item(uint token, int index) => index >= 0 && index < MenuGump.ItemCapacity ? Node(token).Items[index] : throw new ArgumentOutOfRangeException(nameof(index));
    /// <summary>childId는 토큰이 아닌 현재 등록 ID다.</summary>
    private uint Child(uint id) => id < _create.Entries.Length && _create.Entries[id] != 0 ? _create.Entries[id] : throw new InvalidOperationException("메뉴 자식 등록 ID 없음");
    /// <summary>패치 ordered 하한/CD unsigned float 비트 하한과 strict 상한을 유지한다.</summary>
    private bool Contains(uint token, (float X, float Y) local)
    { bool patch = _create.Collection.Edition == Netstorm.Core.Original.OriginalEdition.Patch1078; bool x = patch ? local.X >= 0 : BitConverter.SingleToUInt32Bits(local.X) <= 0x80000000U; bool y = patch ? local.Y >= 0 : BitConverter.SingleToUInt32Bits(local.Y) <= 0x80000000U; return x && local.X < Base(token).Width && y && local.Y < Base(token).Height; }
    /// <summary>정상 트리에 영향 없는 현재 재귀 경로의 순환만 거부한다.</summary>
    private static void Enter(HashSet<uint> active, uint token) { if (!active.Add(token)) throw new InvalidOperationException("메뉴 재귀 링크 순환"); }
    /// <summary>메뉴 종류인 자식만 재귀 검사한다.숨긴 자식의 하위 표시 메뉴도 방문하고 재귀 뒤 현재 next를 읽는다.</summary>
    private bool Covered(uint token, ScreenPoint point, HashSet<uint> active)
    {
        Enter(active, token); var siblings = new HashSet<uint>();
        // 투영 콜백이 바꾼 현재 next도 반영한다.표시는 자신의 경계 검사에만 적용한다.
        for (uint child = Base(token).Child; child != 0; child = Base(child).Next) { if (!siblings.Add(child)) throw new InvalidOperationException("메뉴 가림 형제 순환"); if (Base(child).Command != unchecked((uint)(int)_state.Alignment)) continue; var local = _collection.ScreenToLocal(child, point); if (((Base(child).Flags & 1) != 0 && Contains(child, local)) || Covered(child, point, active)) { active.Remove(token); return true; } } active.Remove(token); return false;
    }
    /// <summary>현재 가상 재그리기 요청.픽셀 Draw와 별개다.</summary>
    private void Redraw(uint token) => Require(_hooks.Redraw)(token, false);
    /// <summary>명시 인자 비트가 없으면 flags 전체 DWORD가 인자다.</summary>
    private uint Parameter(uint token, int index, uint flags) => (flags & 0x80) != 0 ? Item(token, index).Parameter : flags;
    /// <summary>도달한 원본 소리를 생략하지 않는다.</summary>
    private void Sound(string name) => Require(_hooks.Sound)(name);
    /// <summary>현재 개수/자식 flags로 재귀 닫고 동적 자식은 숨김 여부와 무관하게 Kill한다.</summary>
    private void CloseChildren(uint token, bool clearSelection, HashSet<uint> active)
    {
        Enter(active, token);
        // redraw/Kill 뒤 같은 항목의 현재 flags를 읽으며 선택 인덱스는 변경하지 않는다.
        for (int index = 0; index < unchecked((int)Node(token).ItemCount); index++) { var item = Item(token, index); if (clearSelection) item.Flags &= ~8U; if ((item.Flags & 0x20) == 0) continue; uint child = Child(item.ChildId); if ((Base(child).Flags & 1) != 0) { CloseChildren(child, clearSelection, active); Redraw(child); Base(child).Flags &= ~1U; } if ((item.Flags & 0x800) != 0) { _lifetime.Kill(child); item.ChildId = 0; item.Flags &= ~0x20U; } } active.Remove(token);
    }
    /// <summary>호출마다 독립 재귀 검사를 사용한다.clearSelection은 강조 비트만 지운다.</summary>
    public void Close(uint token, bool clearSelection) => CloseChildren(token, clearSelection, []);
    /// <summary>등록표0의 실제 루트 직계 sticky 팝업 중 비dead만 Kill한다.override 머리는 쓰지 않는다.</summary>
    public void ClosePopups()
    {
        var siblings = new HashSet<uint>();
        // Kill 전에 next를 보관해 분리/콜백이 바꾼 형제 링크와 독립적인 방문 순서를 유지한다.
        for (uint token = Base(_create.Entries[0]).Child; token != 0;) { if (!siblings.Add(token)) throw new InvalidOperationException("메뉴 팝업 형제 순환"); var n = Base(token); uint next = n.Next; if ((n.Flags & 0x4000) != 0 && (n.Flags & 0x10000) == 0) _lifetime.Kill(token); token = next; }
    }
    /// <summary>자신/부모 visible만 검사하고 compact 전체 또는 현재 행의 양수 변경 영역을 요청한다.</summary>
    public void RedrawItem(uint token, int index)
    {
        if ((Base(token).Flags & 1) == 0) return; var parents = new HashSet<uint>();
        // dead/void는 추가 필터가 아니다.현재 부모의 visible만 검사한다.
        for (uint parent = Base(token).Parent; parent != 0; parent = Base(parent).Parent) { if (!parents.Add(parent)) throw new InvalidOperationException("메뉴 행 부모 순환"); if ((Base(parent).Flags & 1) == 0) return; }
        var point = _collection.Position(token); int height; if ((Node(token).MenuFlags & 0x1000000) != 0) height = GumpCollection.Ftol(Node(token).Height); else { Item(token, index); point = new(point.X, GumpCollection.Ftol((double)point.Y + Node(token).Offsets[index])); height = GumpCollection.Ftol(Node(token).Heights[index]); } int width = GumpCollection.Ftol(Node(token).Width); if (width <= 0 || height <= 0) return; Require(_hooks.Invalidate)(new(point.X, point.Y, unchecked(point.X + width), unchecked(point.Y + height)));
    }
    /// <summary>로컬 범위/자식 가림→행 선택→이전/새 행 변경 영역과 현재 강조 쓰기 순서를 유지한다.</summary>
    public void Broadcast(uint token, uint eventCode, uint first, uint second)
    {
        var point = new ScreenPoint(unchecked((int)first), unchecked((int)second)); var local = _collection.ScreenToLocal(token, point); int selected = -1;
        if ((eventCode & 0x4000000) == 0 && Contains(token, local) && !Covered(token, point, []))
        {
            if ((Node(token).MenuFlags & 0x80000) == 0) selected = GumpCollection.Ftol((double)local.X / Node(token).ItemWidth);
            else { selected = 0; if ((Node(token).MenuFlags & 0x1000000) == 0)
                // 마지막까지 행을 찾지 못해도 마지막 인덱스를 유지하는 원본 탐색이다.
                while (selected < unchecked((int)Node(token).ItemCount - 1)) { Item(token, selected); if (local.Y >= Node(token).Offsets[selected] && (double)local.Y < (double)Node(token).Heights[selected] + Node(token).Offsets[selected]) break; selected++; }
            }
        }
        int old = Node(token).Selection; if (selected == old) return; if (old >= 0) { if ((Node(token).MenuFlags & 0x80000) != 0) RedrawItem(token, old); Item(token, Node(token).Selection).Flags &= ~8U; Node(token).Selection = -1; } if (selected >= 0 && (Item(token, selected).Flags & 0x200) == 0) { if ((Node(token).MenuFlags & 0x80000) != 0) RedrawItem(token, selected); Node(token).Selection = selected; Item(token, selected).Flags |= 8; } if ((Node(token).MenuFlags & 0x80000) == 0) Redraw(token);
    }
    /// <summary>누름 선택 통지→명령/닫기→동적/정적 하위 열기→현재 부모/닫힘 통지와 팝업 삭제.</summary>
    public uint Handle(uint token, uint eventCode, uint first, uint second)
    {
        uint kind = eventCode & 0x03ffffff; if (Node(token).Expanding != 0 || (kind != 0x10000 && kind != 0x20000) || (eventCode & 0x80000000) != 0) return 0; Require(_hooks.Broadcast)(token, eventCode, first, second); int selected = Node(token).Selection; if (selected < 0) { if ((Node(token).MenuFlags & 0x4000000) != 0) _lifetime.Kill(token); return 1; } uint flags = Item(token, selected).Flags;
        if ((flags & 1) != 0) { uint parameter = Parameter(token, selected, flags), command = Item(token, selected).Command, id = Base(token).Id; Require(_hooks.Command)(command, id, parameter); Sound("openSubGump.wav"); }
        Close(token, (flags & (0x20 | 2)) == 0); flags = Item(token, Node(token).Selection).Flags;
        if ((flags & 0x800) != 0 && (flags & 0x20) == 0) { uint parameter = Parameter(token, Node(token).Selection, flags), serial = Base(token).Serial, request = _state.RequestEvent; uint child = Require(_hooks.Handle)(token, request, serial, parameter); if (child != 0) { Item(token, Node(token).Selection).ChildId = Base(child).Id; Base(child).Flags &= ~1U; Require(_hooks.Attach)(child, 0, 0, token, 0); Item(token, Node(token).Selection).Flags |= 0x20; flags = Item(token, Node(token).Selection).Flags; } }
        if ((flags & 0x20) != 0) { uint child = Child(Item(token, Node(token).Selection).ChildId); Require(_hooks.Push)(child); Base(child).Flags |= 1; var position = _menu.ItemPosition(token, Node(token).Selection); Require(_hooks.Attach)(child, position.X, position.Y, 0, 0); _tree.AddDepth(child, uint.MaxValue); _menu.StartExpand(child, (Node(token).MenuFlags & 0x40000) != 0 ? 0x80000U : 0x40000U); Sound("openGump.wav"); Redraw(token); }
        else if ((flags & 2) == 0) { var parents = new HashSet<uint>(); uint current = token;
            // 메뉴 부모를 따라가다 루트 메뉴 경계에서 해당 가지의 자식들을 닫는다.
            while (current != 0) { if (!parents.Add(current)) throw new InvalidOperationException("메뉴 닫기 부모 순환"); uint parent = Base(current).Parent; if (Base(parent).Command != unchecked((uint)(int)_state.Alignment) || (Node(current).MenuFlags & 0x100000) != 0) { Close(current, true); break; } current = parent; }
        }
        if ((Node(token).MenuFlags & 0x200000) != 0) { if ((flags & 1) == 0) return 1; _lifetime.Kill(token); uint serial = Base(token).Serial; _events.Bubble(token, _state.ClosedEvent, serial, serial); } if ((flags & 1) != 0) ClosePopups(); return 1;
    }
    /// <summary>서로 다를 수 있는 사건/캡처 분배를 각각 감싸고 현재 메뉴만 입력/프레임 통지로 연결한다.</summary>
    public static void BindMenuGumpInput(GumpEventHooks events, GumpInputHooks input, GumpFrameHooks frame, Func<uint, MenuGumpInput?> resolve)
    {
        ArgumentNullException.ThrowIfNull(resolve);
        // 각 기존 반환 DWORD와 다른 파생 처리를 보존한다.
        Func<uint, uint, uint, uint, uint> Bind(Func<uint, uint, uint, uint, uint>? previous) => (t, e, a, b) => resolve(t) is { } menu ? menu.Handle(t, e, a, b) : Require(previous)(t, e, a, b);
        events.Handle = Bind(events.Handle); input.Handle = Bind(input.Handle); var previous = frame.Broadcast; frame.Broadcast = (t, e, a, b) => { if (resolve(t) is { } menu) menu.Broadcast(t, e, a, b); else Require(previous)(t, e, a, b); };
    }
    /// <summary>실제 SoundPlayer 장치 조립 경계에 원본 파일 이름과 나머지 인자5개 모두0을 전달한다.</summary>
    public static void BindMenuGumpInputSound(MenuGumpInputHooks hooks, Action<string, int, int, int, int, int> playByName) { ArgumentNullException.ThrowIfNull(playByName); hooks.Sound = name => playByName(name, 0, 0, 0, 0, 0); }
}
