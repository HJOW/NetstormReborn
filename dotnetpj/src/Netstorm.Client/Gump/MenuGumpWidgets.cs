using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 메뉴 생성·장식/제목·controlled 시작·부착·소유 수명 영역이다.

/// <summary>원본 varargs의 문자열/자식 포인터를 구별하는 호스트 입력이다.</summary>
public abstract record MenuGumpConstructionSource;
/// <summary>원본 std::string::c_str처럼 안정된 NUL 종료 BYTE 저장소를 소유한다.생성 후 콜백도 같은 배열을 본다.</summary>
public sealed record MenuGumpConstructionText : MenuGumpConstructionSource
{
    /// <summary>추가NUL을 포함한 원본 문자열 바이트.</summary>
    public byte[] Bytes { get; }
    /// <summary>호스트 입력을 한 번 소유하고 이후 SetText에는 같은 주소를 전달한다.</summary>
    public MenuGumpConstructionText(ReadOnlySpan<byte> text) { Bytes = [.. text, 0]; }
}
/// <summary>생성 중 앞 항목에 연결할 실제 자식 토큰.</summary>
/// <param name="Token">같은 기본 등록표의 자식 토큰.</param>
public sealed record MenuGumpConstructionChild(uint Token) : MenuGumpConstructionSource;
/// <summary>명령/인자는 조건 검사보다 먼저 쓴다.조건 false도 해당 필드 쓰기는 생략하지 않는다.</summary>
/// <param name="Source">문자열 또는 자식 토큰.</param><param name="Flags">원본 항목 플래그.</param><param name="Command">bit1 명령.</param><param name="Parameter">bit80 명시 인자.</param><param name="Condition">bit100 조건.0은 생략.</param>
public readonly record struct MenuGumpConstructionEntry(MenuGumpConstructionSource Source, uint Flags, uint Command = 0, uint Parameter = 0, uint Condition = 1);
/// <summary>실제 게임 초기화가 공급하는 메뉴 종류/배경/색/Form 전역.</summary>
public sealed class MenuGumpWidgetState
{
    /// <summary>기본 command에 signed 확장할 BYTE.</summary>
    public sbyte Alignment { get; set; } = 26;
    /// <summary>현재 배경 타입.</summary>
    public uint BackgroundType { get; set; }
    /// <summary>현재 배경 프레임.</summary>
    public uint BackgroundFrame { get; set; }
    /// <summary>현재 전경 팔레트 번호.</summary>
    public uint Foreground { get; set; }
    /// <summary>현재 그림자 팔레트 번호.</summary>
    public uint Shadow { get; set; }
    /// <summary>객체 연결에 만들 실제 Form 타입.</summary>
    public uint ObjectFormType { get; set; }
}
/// <summary>같은 공용 SID 풀의 Form 토큰/인덱스.</summary>
/// <param name="Token">Form 조회 토큰.</param><param name="Index">기본 Gump+28에 쓸 풀 인덱스.</param>
public readonly record struct MenuGumpObjectForm(uint Token, uint Index);
/// <summary>같은 실제 기본/메뉴 저장소와 할당·부착·선택 통지·Form 경계.</summary>
public sealed class MenuGumpWidgetHooks
{
    /// <summary>현재 실제 메뉴.</summary>
    public Func<uint, MenuGumpNode?>? Node { get; set; }
    /// <summary>같은 실제 기본/장식/제목.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>원본 크기별0xb90 메뉴/0xc4 장식/0xd8 제목 할당.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>현재 실제 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 선택 소멸 통지.</summary>
    public Action<uint, int>? SelectionNotice { get; set; }
    /// <summary>현재 Form(type,2)의 실제 생성.</summary>
    public Func<uint, uint, MenuGumpObjectForm>? ObjectForm { get; set; }
    /// <summary>Form에 현재 Gump 등록 ID를 쓴다.</summary>
    public Action<uint, uint>? FormOwner { get; set; }
    /// <summary>같은 Form을 현재 객체에 가상+74로 부착한다.</summary>
    public Action<uint, uint, uint>? FormObject { get; set; }
}
/// <summary>cpppj/src/client/MenuGumpWidgets.cpp의 부분 초기화·조건부 항목·현재 전역·판본별 float 저장·실제 수명.</summary>
public sealed class MenuGumpWidgets
{
    /// <summary>원본 문자열 항목 비트.</summary>
    private const uint Text = 0x10000;
    /// <summary>원본 생성 자식 항목 비트.</summary>
    private const uint Menu = 0x20000;
    /// <summary>가로 메뉴 비트.</summary>
    private const uint Horizontal = 0x40000;
    /// <summary>장식 메뉴 비트.</summary>
    private const uint FancyFlag = 0x800000;
    /// <summary>현재 등록/판본/화면.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 controlled 표/생성 개수.</summary>
    private readonly MenuGumpState _menus;
    /// <summary>현재 종류/자산/색 전역.</summary>
    private readonly MenuGumpWidgetState _state;
    /// <summary>같은 실제 배경 생성기.</summary>
    private readonly GumpBackgroundCreate _background;
    /// <summary>같은 실제 장식.</summary>
    private readonly GumpDecoration _decoration;
    /// <summary>같은 실제 제목/소유 버퍼/Slider 생성기.</summary>
    private readonly ScrollingTextGumpWidgets _title;
    /// <summary>같은 실제 항목/레이아웃.</summary>
    private readonly MenuGumpLayout _layout;
    /// <summary>같은 실제 소유 조각.</summary>
    private readonly StyleText _style;
    /// <summary>같은 자식 우선 기본 수명.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>같은 실제 화면 위치.</summary>
    private readonly GumpCollection _collection;
    /// <summary>같은 실제 기본 부착.</summary>
    private readonly GumpTree _tree;
    /// <summary>현재 공유 저장/가상 경계.</summary>
    private readonly MenuGumpWidgetHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("메뉴 Widget 경계 필요");
    /// <summary>같은 실제 저장소/기존 처리기를 빌린다.순환 생성 의존성은 현재 분류 콜백으로 연결한다.</summary>
    public MenuGumpWidgets(GumpCreateState create, MenuGumpState menus, MenuGumpWidgetState state, GumpBackgroundCreate background, GumpDecoration decoration, ScrollingTextGumpWidgets title, MenuGumpLayout layout, StyleText style, GumpLifetime lifetime, GumpCollection collection, GumpTree tree, MenuGumpWidgetHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(menus); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(background); ArgumentNullException.ThrowIfNull(decoration); ArgumentNullException.ThrowIfNull(title); ArgumentNullException.ThrowIfNull(layout); ArgumentNullException.ThrowIfNull(style); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _create = create; _menus = menus; _state = state; _background = background; _decoration = decoration; _title = title; _layout = layout; _style = style; _lifetime = lifetime; _collection = collection; _tree = tree; _hooks = hooks; }
    /// <summary>현재 같은 실제 기본/메뉴 저장소.</summary>
    private MenuGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("메뉴 Widget 공유 노드 없음"); return n; }
    /// <summary>실제 도달한 자식/부모.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("메뉴 Widget 기본 노드 없음");
    /// <summary>부분 쓰기를 되돌리지 않고 실제 고정40칸만 확인한다.</summary>
    private MenuGumpItem Item(uint token, uint index) => index < MenuGump.ItemCapacity ? Node(token).Items[(int)index] : throw new ArgumentOutOfRangeException(nameof(index));
    /// <summary>콜백 뒤 현재 index를 읽는다.예약0은 Begin의 증가 직후만 별도 거부한다.</summary>
    private int Index() => _menus.Index < _menus.Entries.Length ? (int)_menus.Index : throw new ArgumentOutOfRangeException("Index");
    /// <summary>실제 자식의 현재 가상 Attach에 mode0을 전달한다.</summary>
    private void AttachChild(uint token, float x, float y, uint parent) { Base(token); Require(_hooks.Attach)(token, x, y, parent, 0); }
    /// <summary>명령/인자를 먼저 쓰고 조건부 문자열은 Reflow 없이,자식은 앞 항목에 연결한다.</summary>
    private void Entry(uint token, MenuGumpConstructionEntry entry)
    {
        uint flags = entry.Flags; if ((flags & 1) != 0) Item(token, Node(token).ItemCount).Command = entry.Command; if ((flags & 0x80) != 0) Item(token, Node(token).ItemCount).Parameter = entry.Parameter; bool enabled = (flags & 0x100) == 0 || entry.Condition != 0;
        if ((flags & Text) != 0)
        {
            if ((flags & Menu) != 0) throw new ArgumentException("메뉴 생성 문자열/자식 플래그 중복"); if (enabled) { if (entry.Source is not MenuGumpConstructionText text) throw new ArgumentException("메뉴 생성 실제 문자열 필요"); uint index = Node(token).ItemCount; Item(token, index).Flags = flags; uint[] context = (uint[])Node(token).TextContext.Clone(); _layout.SetText(token, unchecked((int)index), text.Bytes, context); Node(token).ItemCount = unchecked(Node(token).ItemCount + 1); } if (unchecked((int)Node(token).ItemCount) >= MenuGump.ItemCapacity) throw new ArgumentException("메뉴 생성 numItems < 40 계약 위반");
        }
        else
        {
            if ((flags & Menu) == 0) throw new ArgumentException("메뉴 생성 문자열 또는 자식 비트 필요"); if (enabled) { if (entry.Source is not MenuGumpConstructionChild child) throw new ArgumentException("메뉴 생성 실제 자식 필요"); Base(child.Token).Flags &= ~1U; AttachChild(child.Token, 0, 0, token); if (unchecked((int)Node(token).ItemCount) > 0) { Item(token, Node(token).ItemCount - 1).Flags |= 0x20; Item(token, Node(token).ItemCount - 1).ChildId = Base(child.Token).Id; } }
        }
    }
    /// <summary>실제 장식→제목/버퍼/Slider/overlay를 만들고 원본 부착/깊이/배경 비트를 쓴다.</summary>
    private void Fancy(uint token)
    { if ((Node(token).MenuFlags & Horizontal) != 0) throw new ArgumentException("장식 메뉴의 가로 비트 금지"); var allocate = Require(_hooks.Allocate); uint decoration = allocate(0xc4); Node(token).Fancy = decoration != 0 ? _decoration.Construct(decoration, 1, 1) : 0; AttachChild(Node(token).Fancy, 0, 0, token); Base(Node(token).Fancy).SecondaryDepth = 1; uint title = allocate(0xd8); Node(token).Title = title != 0 ? _title.Construct(title, 0, 0, 0, _state.Foreground) : 0; AttachChild(Node(token).Title, 16, 16, Node(token).Fancy); Base(Node(token).Title).SecondaryDepth = uint.MaxValue; Node(token).BackgroundFlags |= 0x40000; }
    /// <summary>원본 미작성 offsets/목표 크기는 보존하고 최종 방향의 Reflow에 맡긴다.고정 항목 객체 주소는 바꾸지 않는다.</summary>
    public uint Construct(uint token, ReadOnlySpan<MenuGumpConstructionEntry> entries, uint flags)
    {
        Node(token); _background.Construct(token, 0, 0, 0x03e00000, 0x201, _state.BackgroundType, _state.BackgroundFrame); var n = Node(token); n.Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x50c6f0U : 0x501fa0U; n.Fancy = n.Title = n.SelectionCallback = 0; n.TextContext[1] = 0; n.Command = unchecked((uint)(int)_state.Alignment); n.TextContext[2] = _state.Foreground; n.TextContext[7] = 0; n.TextContext[5] = _state.Shadow; n.TextContext[6] = 1; n.TextContext[0] |= 2; _menus.Instances = unchecked(_menus.Instances + 1); n.Expanding = 0; n.ItemWidth = 0; n.MenuFlags = n.ItemCount = 0; n.Flags |= 0x20; n.Depth = 0xffffffc4; n.Selection = -1; Array.Clear(n.Heights);
        // 원본 전체 항목0 쓰기만 수행한다.소유 Clear나 배열 객체 교체를 추가하지 않는다.
        foreach (var item in n.Items) { Array.Clear(item.Label); item.ChildId = item.Command = item.Parameter = item.Styled = item.Flags = 0; }
        // 입력 span의 고정 순서대로 수행하며 각 항목 콜백 뒤 현재 개수를 읽는다.
        foreach (var entry in entries) Entry(token, entry);
        Node(token).MenuFlags = flags; if ((flags & FancyFlag) != 0) Fancy(token); _layout.Reflow(token); Node(token).Flags |= 0x100; return token;
    }
    /// <summary>현재 칸 증가→할당/생성→콜백 뒤 현재 칸 쓰기→idle/추가 배경 비트 순서.</summary>
    private uint Begin(uint flags, bool idle, bool extra, uint backgroundFlags)
    { _menus.Index = unchecked(_menus.Index + 1); if (_menus.Index == 0) throw new ArgumentOutOfRangeException("Index", "controlled 예약 칸 금지"); if (_menus.Entries[Index()] != 0) throw new ArgumentException("controlled 시작 칸은 비어 있어야 함"); uint token = Require(_hooks.Allocate)(0xb90); uint created = token != 0 ? Construct(token, [], flags) : 0; _menus.Entries[Index()] = created; if (idle) _lifetime.SetIdle(_menus.Entries[Index()], true); if (extra) Node(_menus.Entries[Index()]).BackgroundFlags |= backgroundFlags; return _menus.Entries[Index()]; }
    /// <summary>원본 가로 메뉴는 별도 idle 표에 등록한다.</summary>
    public uint BeginHorizontal() => Begin(0x140000, true, false, 0);
    /// <summary>원본 장식 세로 메뉴는 별도 idle 등록이 없다.</summary>
    public uint BeginFancy() => Begin(0xa80000, false, false, 0);
    /// <summary>원본 세로 메뉴는 idle 뒤 현재 배경 플래그에 인자를 OR한다.</summary>
    public uint BeginVertical(uint backgroundFlags = 0) => Begin(0x280000, true, true, backgroundFlags);
    /// <summary>원본 min의 unordered는 요청값이다.패치만 요청을 float32로 먼저 저장한다.</summary>
    private static int Limit(double request, double maximum, bool narrow) { if (narrow) request = (float)request; return GumpCollection.Ftol(maximum < request ? maximum : request); }
    /// <summary>root 비트/parent0 재사용과 판본별 전체 화면 보정 뒤 실제 기본 Attach에 전달한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        if (parent == 0) parent = Node(token).Parent; if (parent == _create.Collection.Root) Node(token).Flags |= 0x4000;
        if ((mode & 1) == 0 && (Node(token).Flags & 0x20000) == 0)
        {
            var origin = _collection.Position(parent); bool patch = _create.Collection.Edition == OriginalEdition.Patch1078; int screenX, screenY;
            if ((Node(token).MenuFlags & FancyFlag) != 0) { var decoration = Base(Node(token).Fancy); screenX = GumpCollection.Ftol(((double)origin.X + decoration.X) + x); screenY = GumpCollection.Ftol(patch ? ((double)origin.Y + y) + decoration.Y : ((double)origin.Y + decoration.Y) + y); if (screenX < 0) screenX = 0; if (screenY < 0) screenY = 0; screenX = Limit(screenX, (double)_create.ScreenWidth - decoration.Width, patch); screenY = Limit(screenY, (double)_create.ScreenHeight - decoration.Height, patch); screenX = GumpCollection.Ftol((double)screenX - decoration.X); screenY = GumpCollection.Ftol((double)screenY - decoration.Y); }
            else { screenX = Limit((double)origin.X + x, (double)_create.ScreenWidth - Node(token).Width, patch); screenY = Limit((double)origin.Y + y, (double)_create.ScreenHeight - Node(token).Height, patch); }
            x = unchecked(screenX - origin.X); y = unchecked(screenY - origin.Y);
        }
        _tree.Attach(token, x, y, parent, mode);
    }
    /// <summary>파생 표/개수 감소→현재 선택 통지→모든40항목 역순 Clear→같은 기본 scalar.</summary>
    public uint DeleteScalar(uint token, uint flags)
    {
        Node(token).Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x50c6f0U : 0x501fa0U; _menus.Instances = unchecked(_menus.Instances - 1); if (Node(token).SelectionCallback != 0) Require(_hooks.SelectionNotice)(Node(token).SelectionCallback, Node(token).Selection);
        // itemCount/styled 플래그와 무관하게 실제 소유 필드40개를 끝부터 비운다.
        for (int index = MenuGump.ItemCapacity; index > 0; index--) _style.Clear(ref Item(token, (uint)(index - 1)).Styled); return _lifetime.DeleteScalar(token, flags);
    }
    /// <summary>객체0은 기존 연결을 유지한다.실제 Form 생성/소유 ID/객체 부착 뒤 판본별 기본 필드를 쓴다.</summary>
    public void Reference(uint token, uint objectId)
    { if (objectId != 0) { var create = Require(_hooks.ObjectForm); var owner = Require(_hooks.FormOwner); var attach = Require(_hooks.FormObject); var form = create(_state.ObjectFormType, 2); if (form.Token == 0) throw new InvalidOperationException("메뉴 객체 Form 생성 실패"); owner(form.Token, Base(token).Id); attach(form.Token, objectId, 0); if (_create.Collection.Edition == OriginalEdition.Patch1078) { Base(token).Flags |= 0x40; Base(token).Object = objectId; } else { Base(token).Object = objectId; Base(token).Flags |= 0x40; } Base(token).Form = form.Index; } }
    /// <summary>현재 실제 Form 연결 후 같은 가상 Attach를 호출한다.</summary>
    public void AttachObject(uint token, uint objectId, float x, float y, uint parent, uint mode) { Reference(token, objectId); Require(_hooks.Attach)(token, x, y, parent, mode); }
    /// <summary>정보 메뉴의 별도 idle 요청을 같은 공통 표에 전달한다.</summary>
    public void SetIdle(uint token, bool enabled) => _lifetime.SetIdle(token, enabled);
    /// <summary>실제 제목 RequestScroll/FitContent로 기존 Reflow 경계를 연결한다.</summary>
    public static void BindMenuTitleLayout(MenuGumpLayoutHooks hooks, ScrollingTextGump text) { ArgumentNullException.ThrowIfNull(text); hooks.TitleFit = (t, p) => text.RequestScroll(t, unchecked((int)p)); hooks.TitleLayout = (t, w, h) => text.FitContent(t, unchecked((int)w), unchecked((int)h)); }
    /// <summary>현재 메뉴만 실제 파생 Attach/scalar로 분배하고 다른 파생 처리를 보존한다.</summary>
    public static void BindMenuGumpWidgets(GumpTreeHooks tree, GumpLifetimeHooks lifetime, Func<uint, MenuGumpWidgets?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var attach = tree.Attach; var delete = lifetime.DeletingDestructor; tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } w) w.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); }; lifetime.DeletingDestructor = (t, f) => { if (resolve(t) is { } w) w.DeleteScalar(t, f); else Require(delete)(t, f); }; }
    /// <summary>controlled 객체 부착의 가상+3c를 같은 실제 메뉴에 연결한다.</summary>
    public static void BindMenuGumpObjectAttach(MenuGumpHooks hooks, Func<uint, MenuGumpWidgets?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var attach = hooks.AttachObject; hooks.AttachObject = (t, o, x, y, p, m) => { if (resolve(t) is { } w) w.AttachObject(t, o, x, y, p, m); else Require(attach)(t, o, x, y, p, m); }; }
}
