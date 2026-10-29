using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 메뉴의 고정 항목·크기·확장 프레임·controlled 스택 영역이다.

/// <summary>원본60바이트 항목.라벨 뒤 다섯 DWORD와 실제 StyleText 소유 주소를 보관한다.</summary>
public sealed class MenuGumpItem
{
    /// <summary>원본 strncpy40 라벨.추가NUL은 쓰지 않는다.</summary>
    public byte[] Label { get; } = new byte[40];
    /// <summary>자식 등록 ID.토큰과 구별한다.</summary>
    public uint ChildId { get; set; }
    /// <summary>선택 명령.</summary>
    public uint Command { get; set; }
    /// <summary>명령 인자.</summary>
    public uint Parameter { get; set; }
    /// <summary>실제 소유 head.Clear/Parse/Measure에 ref로 넘긴다.</summary>
    public uint Styled;
    /// <summary>원본 항목 비트.</summary>
    public uint Flags { get; set; }
}
/// <summary>같은 실제 배경을 공유하는 메뉴 크기/항목/확장 필드.</summary>
public class MenuGumpNode : GumpBackgroundNode
{
    /// <summary>현재 확장 프레임 여부.</summary>
    public uint Expanding { get; set; }
    /// <summary>원본 방향/장식/간격 비트.</summary>
    public uint MenuFlags { get; set; }
    /// <summary>DWORD 항목 수.반복에서는 signed로 읽는다.</summary>
    public uint ItemCount { get; set; }
    /// <summary>현재 장식 토큰.</summary>
    public uint Fancy { get; set; }
    /// <summary>현재 제목 토큰.</summary>
    public uint Title { get; set; }
    /// <summary>현재 공통 항목 폭.</summary>
    public float ItemWidth { get; set; }
    /// <summary>목표 폭.</summary>
    public float TargetWidth { get; set; }
    /// <summary>목표 높이.</summary>
    public float TargetHeight { get; set; }
    /// <summary>항목별 현재 시작 위치.남는 칸은 보존한다.</summary>
    public float[] Offsets { get; } = new float[MenuGump.ItemCapacity];
    /// <summary>왼쪽/오른쪽/위/아래 여백.</summary>
    public float[] Margins { get; } = new float[4];
    /// <summary>현재 측정/가로 공통 높이.</summary>
    public float[] Heights { get; } = new float[MenuGump.ItemCapacity];
    /// <summary>고정 배열의 주소를 보존하는 실제40항목.각 객체의 필드만 갱신한다.</summary>
    public IReadOnlyList<MenuGumpItem> Items { get; } = Array.AsReadOnly(Enumerable.Range(0, MenuGump.ItemCapacity).Select(_ => new MenuGumpItem()).ToArray());
    /// <summary>현재 선택.-1은 없음.</summary>
    public int Selection { get; set; } = -1;
    /// <summary>선택 소멸 통지 식별자.</summary>
    public uint SelectionCallback { get; set; }
}
/// <summary>원본 controlled 표/현재 위치/정상 생성·소멸 개수.</summary>
public sealed class MenuGumpState
{
    /// <summary>스택0은 예약 칸이다.</summary>
    public uint[] Entries { get; } = new uint[MenuGump.ControlledMenuCapacity];
    /// <summary>signed index의 DWORD 비트.</summary>
    public uint Index { get; set; }
    /// <summary>정상 메뉴 생성/소멸에서 증감한다.</summary>
    public uint Instances { get; set; }
}
/// <summary>현재 같은 메뉴/기본 노드와 실제 가상 프레임/부착 경계.</summary>
public sealed class MenuGumpHooks
{
    /// <summary>현재 메뉴.</summary>
    public Func<uint, MenuGumpNode?>? Node { get; set; }
    /// <summary>같은 기본 노드와 장식/제목.</summary>
    public Func<uint, GumpCreateNode?>? BaseNode { get; set; }
    /// <summary>현재 가상 Push.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>현재 가상 Pop.</summary>
    public Action<uint>? Pop { get; set; }
    /// <summary>현재 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>객체 SID/현재 등록표 루트를 받는 가상+3c.</summary>
    public Action<uint, uint, float, float, uint, uint>? AttachObject { get; set; }
    /// <summary>현재 장식 재그리기.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>현재 기본 Gump 재그리기.</summary>
    public Action<uint, bool>? BaseRedraw { get; set; }
}
/// <summary>cpppj/src/client/MenuGump.cpp의 현재 필드·float 저장·확장과 controlled 스택.</summary>
public sealed class MenuGump
{
    /// <summary>원본 고정 항목 수.</summary>
    public const int ItemCapacity = 40;
    /// <summary>예약0을 포함한 controlled 표 길이.</summary>
    public const int ControlledMenuCapacity = 20;
    /// <summary>현재 등록/루트/판본.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 controlled 표.</summary>
    private readonly MenuGumpState _state;
    /// <summary>같은 실제 idle 등록.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>현재 노드/가상 경계.</summary>
    private readonly MenuGumpHooks _hooks;
    /// <summary>도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("메뉴 가상 경계 필요");
    /// <summary>같은 안정된 기본/파생 저장소와 현재 전역을 빌린다.</summary>
    public MenuGump(GumpCreateState create, MenuGumpState state, GumpLifetime lifetime, MenuGumpHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.BaseNode); _create = create; _state = state; _lifetime = lifetime; _hooks = hooks; }
    /// <summary>배경/수명/메뉴는 같은 실제 객체여야 한다.</summary>
    private MenuGumpNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.BaseNode!(token))) throw new InvalidOperationException("메뉴 공유 저장소 없음"); return n; }
    /// <summary>현재 제목/장식은 다른 기본 파생도 허용한다.</summary>
    private GumpCreateNode BaseNode(uint token) => (token != 0 ? _hooks.BaseNode!(token) : null) ?? throw new InvalidOperationException("메뉴 장식/제목 없음");
    /// <summary>현재 제목 높이의 음수-32를 CRT 절삭해 현재 장식을 재부착한다.</summary>
    private void AttachFancy(uint token) { var n = Node(token); float y = GumpCollection.Ftol(-(double)BaseNode(n.Title).BaseHeight - 32); Require(_hooks.Attach)(n.Fancy, -16, y, token, 0); }
    /// <summary>부모 폭→장식 Push→현재 폭+32의 좁힘 전 배율→현재 재부착.</summary>
    public void SetWidth(uint token, float width)
    { var n = Node(token); n.BaseWidth = width; n.Width = (float)((double)width * n.Scale); if ((n.MenuFlags & 0x800000) == 0) return; Require(_hooks.Push)(n.Fancy); double size = (double)n.BaseWidth + 32; var fancy = BaseNode(n.Fancy); fancy.BaseWidth = (float)size; fancy.Width = (float)(size * fancy.Scale); AttachFancy(token); }
    /// <summary>현재 제목 높이+메뉴 높이+48의 중간 합계로 장식 기본/배율 높이를 쓴다.</summary>
    public void SetHeight(uint token, float height)
    { var n = Node(token); n.BaseHeight = height; n.Height = (float)((double)height * n.Scale); if ((n.MenuFlags & 0x800000) == 0) return; Require(_hooks.Push)(n.Fancy); double size = (double)BaseNode(n.Title).BaseHeight + n.BaseHeight + 48; var fancy = BaseNode(n.Fancy); fancy.BaseHeight = (float)size; fancy.Height = (float)(size * fancy.Scale); AttachFancy(token); }
    /// <summary>장식은 메뉴 플래그와 무관하게 먼저 요청하며 기본 재그리기도 항상 수행한다.</summary>
    public void Redraw(uint token, bool erase) { uint fancy = Node(token).Fancy; if (fancy != 0) Require(_hooks.Redraw)(fancy, erase); Require(_hooks.BaseRedraw)(token, erase); }
    /// <summary>가로는 index×항목 폭/현재 배율 높이,세로는 현재 배율 폭/저장offset.</summary>
    public (float X, float Y) ItemPosition(uint token, int index)
    { var n = Node(token); if ((n.MenuFlags & 0x40000) != 0) return ((float)((double)index * n.ItemWidth), n.Height); if (index < 0 || index >= n.Offsets.Length) throw new ArgumentOutOfRangeException(nameof(index)); return (n.Width, n.Offsets[index]); }
    /// <summary>한 축0으로 시작한 뒤 실제 idle 등록→expanding 저장을 수행한다.</summary>
    public void StartExpand(uint token, uint flags)
    { var n = Node(token); if ((flags & 0x80000) != 0) { SetWidth(token, n.TargetWidth); SetHeight(token, 0); } else { SetWidth(token, 0); SetHeight(token, n.TargetHeight); } _lifetime.SetIdle(token, true); n.Expanding = 1; }
    /// <summary>미완료는 Push→현재 목표 높이/폭→Pop.완료 다음 호출은 expanding만 지우고 idle 등록은 남긴다.</summary>
    public void Idle(uint token)
    {
        var n = Node(token); if (n.Expanding == 0) return; if (n.Height >= n.TargetHeight && n.Width >= n.TargetWidth) { n.Expanding = 0; return; } var push = Require(_hooks.Push); var pop = Require(_hooks.Pop); push(token); bool patch = _create.Collection.Edition == OriginalEdition.Patch1078; if (patch ? n.Height < n.TargetHeight : !(n.Height >= n.TargetHeight)) SetHeight(token, n.TargetHeight); if (patch ? n.Width < n.TargetWidth : !(n.Width >= n.TargetWidth)) SetWidth(token, n.TargetWidth); pop(token);
    }
    /// <summary>signed 음수 비트도 배열 범위 밖이므로 접근 전에 거부한다.</summary>
    private int Index() => _state.Index < _state.Entries.Length ? (int)_state.Index : throw new ArgumentOutOfRangeException("Index");
    /// <summary>현재 칸은 null 토큰도 허용한다.</summary>
    public uint Current() => _state.Entries[Index()];
    /// <summary>유효 현재 메뉴의 DWORD 항목 수.</summary>
    public uint ItemCount() => Node(Current()).ItemCount;
    /// <summary>패치는 칸0→index감소,CD는 index감소→이전 칸0.</summary>
    private void DropCurrent() { int index = Index(); if (index == 0) throw new ArgumentOutOfRangeException("Index", "메뉴 예약 칸 꺼내기 오류"); if (_create.Collection.Edition == OriginalEdition.Patch1078) { _state.Entries[index] = 0; _state.Index--; } else { _state.Index--; _state.Entries[_state.Index + 1] = 0; } }
    /// <summary>반환 토큰을 저장한 뒤 현재 칸/위치를 제거한다.</summary>
    public uint TakeCurrent() { uint token = Current(); DropCurrent(); return token; }
    /// <summary>정수를 float32로 전달해 가상 부착한 뒤 콜백 이후 현재 칸을 제거한다.</summary>
    public void AttachCurrent(int x, int y, uint parent) { uint token = Current(); Node(token); Require(_hooks.Attach)(token, x, y, parent, 0); DropCurrent(); }
    /// <summary>현재 실제 등록표 루트와 객체 SID를 가상+3c에 넘긴 뒤 현재 칸을 제거한다.</summary>
    public void AttachObjectCurrent(uint objectId, int x, int y) { uint token = Current(); Node(token); Require(_hooks.AttachObject)(token, objectId, x, y, _create.Entries[0], 0); DropCurrent(); }
    /// <summary>현재 메뉴만 실제 idle/재그리기로 분배하며 다른 프레임을 보존한다.</summary>
    public static void BindMenuGumpFrame(GumpFrameHooks hooks, Func<uint, MenuGump?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var idle = hooks.Idle; var redraw = hooks.Redraw; hooks.Idle = t => { if (resolve(t) is { } m) m.Idle(t); else Require(idle)(t); }; hooks.Redraw = (t, erase) => { if (resolve(t) is { } m) m.Redraw(t, erase); else Require(redraw)(t, erase); }; }
}
