using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 MetaImage 자식을 소유하는 원본21칸 그림 목록 영역이다.

/// <summary>기본 Gump 뒤의 실제 목록 필드.미사용 ID/셀 크기/끝 슬롯도 원문대로 보존한다.</summary>
public sealed class MetaImageListNode : GumpCreateNode
{
    /// <summary>현재 유효한 두 배열의 접두부 길이.</summary>
    public uint Count { get; set; }
    /// <summary>실제 MetaImage 자식 토큰21칸.</summary>
    public uint[] Images { get; } = new uint[MetaImageList.Capacity];
    /// <summary>자식 인덱스와 별개인 원문 ID21칸.</summary>
    public uint[] Ids { get; } = new uint[MetaImageList.Capacity];
    /// <summary>선택 인덱스.-1은 선택 없음이다.</summary>
    public int Selection { get; set; } = -1;
    /// <summary>양수이면 grid 열 수,0이하면 가로 배치다.</summary>
    public int Columns { get; set; }
    /// <summary>배치 후 현재 셀 높이의float32.</summary>
    public float CellHeight { get; set; }
    /// <summary>배치 후 현재 셀 폭의float32.</summary>
    public float CellWidth { get; set; }
}
/// <summary>같은 공통 저장소/실제 MetaImage 생성과 가상 효과 경계.</summary>
public sealed class MetaImageListHooks
{
    /// <summary>실제 같은 목록 파생 노드.</summary>
    public Func<uint, MetaImageListNode?>? Node { get; set; }
    /// <summary>현재 실제 MetaImage 자식.</summary>
    public Func<uint, MetaImageGumpNode?>? Image { get; set; }
    /// <summary>같은 RawImage를 빌리는 실제 자식 생성/등록.</summary>
    public Func<RawImage, uint>? CreateImage { get; set; }
    /// <summary>현재 가상 Attach.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>같은 실제 기본 Attach.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>현재 실제 트리 분리.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>지연 자식 Kill.</summary>
    public Action<uint>? Kill { get; set; }
    /// <summary>같은 실제 기본 소멸자.</summary>
    public Action<uint>? DestructBase { get; set; }
    /// <summary>같은 현재 화면 경계.</summary>
    public Func<uint, ScreenRect>? Bounds { get; set; }
    /// <summary>현재 목록의 가상 Redraw.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>자신을 제외한 부모 Bubble.</summary>
    public Action<uint, uint, uint, uint>? Notify { get; set; }
    /// <summary>현재 녹색 전역의 전체DWORD.</summary>
    public Func<uint>? Green { get; set; }
    /// <summary>현재 클립의 실제 체크무늬 채우기.</summary>
    public Action<ScreenRect, uint>? Fill { get; set; }
}
/// <summary>cpppj/src/client/MetaImageList.cpp의 등록/배치·현재 필드/적중·선택/접두부 삭제·수명.</summary>
public sealed class MetaImageList
{
    /// <summary>원본 IML_MAX_SLOTS의 실제 저장소 크기.</summary>
    public const int Capacity = 21;
    /// <summary>원본 목록 가상 표.</summary>
    private const uint ListVtable = 0x50d0d4;
    /// <summary>원본 종류BYTE.</summary>
    private const uint ListKind = 22;
    /// <summary>선택 변경 부모 사건.</summary>
    private const uint SelectionEvent = 0x02000004;
    /// <summary>같은 실제 기본 생성/등록.</summary>
    private readonly GumpCreate _factory;
    /// <summary>현재 실제 저장소/가상 효과.</summary>
    private readonly MetaImageListHooks _hooks;
    /// <summary>실제 도달한 하위만 요구한다.</summary>
    private static T Require<T>(T? hook) where T : Delegate => hook ?? throw new ArgumentException("Image 목록 실제 하위 미연결");
    /// <summary>원문이 복원된10.78의 같은 기본 생성기를 빌린다.</summary>
    public MetaImageList(GumpCreateState create, GumpCreate factory, MetaImageListHooks hooks) { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(hooks); if (create.Collection.Edition != OriginalEdition.Patch1078) throw new ArgumentException("Image 목록 실제10.78 판본 필요"); _factory = factory; _hooks = hooks; }
    /// <summary>현재 실제 목록과 유효한count를 확인한다.생성 전 임의count는Construct에서 따로 취급한다.</summary>
    private MetaImageListNode Node(uint token) { var n = token != 0 ? Require(_hooks.Node)(token) : null; if (n is null || n.Count > Capacity) throw new ArgumentOutOfRangeException(nameof(token), "Image 목록 실제 저장소/count 오류"); return n; }
    /// <summary>현재 배열이 가리키는 실제 자식을 빌린다.</summary>
    private MetaImageGumpNode Child(uint token) => (token != 0 ? Require(_hooks.Image)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "Image 목록 실제 자식 없음");
    /// <summary>원문은 ID 배열과 두 셀 크기를 쓰지 않는다.기본 생성 후 자식/count/선택만 초기화한다.</summary>
    public uint Construct(uint token, int columns) { var n = Require(_hooks.Node)(token) ?? throw new ArgumentOutOfRangeException(nameof(token), "Image 목록 생성 저장소 없음"); _factory.Construct(token, 0, 0, 0, 1); n.Vtable = ListVtable; n.Command = ListKind; n.Count = 0; Array.Clear(n.Images); n.Flags |= 0x200; n.Selection = -1; n.Columns = columns; return token; }
    /// <summary>ID→실제 자식 생성→현재count 칸 설치→자식 부착→목록Push/재부착의 순서다.</summary>
    public bool Add(uint token, RawImage image, uint id) { var n = Node(token); if (n.Count >= Capacity) return false; n.Ids[n.Count] = id; uint child = Require(_hooks.CreateImage)(image); n.Images[n.Count] = child; n.Count = unchecked(n.Count + 1); Require(_hooks.Attach)(child, 0, 0, token, 0); Require(_hooks.Push)(token); Require(_hooks.Attach)(token, n.X, n.Y, 0, 0); return true; }
    /// <summary>기본 부착 뒤 현재 크기/count를 읽는다.셀 누적 좌표와 단계값은 각각float32로 저장한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        Require(_hooks.BaseAttach)(token, x, y, parent, mode); var n = Node(token); int count = (int)n.Count; if (count <= 0) return; int columns = n.Columns, rowLimit = columns; float stepX, stepY = 0;
        if (columns <= 0) { double width = (double)n.BaseWidth / count; stepX = (float)width; n.CellWidth = stepX; n.CellHeight = (float)(width > n.BaseHeight ? n.BaseHeight : width); rowLimit = count; }
        else { stepX = (float)((double)n.BaseWidth / columns); stepY = (float)((double)n.BaseHeight / (count / columns + (count % columns > 0 ? 1 : 0))); n.CellWidth = stepX; n.CellHeight = stepY; }
        float cellX = 0, cellY = 0; int column = 0;
        // 자식 Push/Attach 뒤 현재count·자식토큰/셀크기를 다시 읽는다.
        for (uint index = 0; index < n.Count; index++) { Require(_hooks.Push)(n.Images[index]); var child = Child(n.Images[index]); child.BaseWidth = n.CellWidth; child.Width = (float)((double)n.CellWidth * child.Scale); var current = Child(n.Images[index]); current.BaseHeight = n.CellHeight; current.Height = (float)((double)n.CellHeight * current.Scale); Require(_hooks.Attach)(n.Images[index], cellX, cellY, token, 0); cellX = (float)((double)cellX + stepX); if (++column >= rowLimit) { column = 0; cellX = 0; cellY = (float)((double)cellY + stepY); } }
    }
    /// <summary>오른쪽/아래는 제외한다.표시scale은 원문 적중에 없고grid는index==count도 그대로 반환한다.</summary>
    public int Hit(uint token, int x, int y)
    { var n = Node(token); var bounds = Require(_hooks.Bounds)(token); if (x < bounds.Left || x >= bounds.Right || y < bounds.Top || y >= bounds.Bottom) return -1; int dx = unchecked(x - bounds.Left); if (n.Columns <= 0) return GumpCollection.Ftol((double)unchecked((int)(n.Count * (uint)dx)) / n.BaseWidth); int dy = unchecked(y - bounds.Top), row = GumpCollection.Ftol((double)dy / n.CellHeight), column = GumpCollection.Ftol((double)dx / n.CellWidth), index = unchecked(row * n.Columns + column); return (int)n.Count < index ? -1 : index; }
    /// <summary>좌누름만 선택을 토글한다.화면 밖도 현재선택과비교해 원문 전체 인자로 통지한다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second) { if ((code & 0x03ffffff) != 0x10000 || (code & 0x80000000) != 0) return 0; var n = Node(token); int index = Hit(token, unchecked((int)first), unchecked((int)second)); if (index >= 0) { n.Selection = n.Selection == index ? -1 : index; Require(_hooks.Redraw)(token, false); } Require(_hooks.Notify)(token, SelectionEvent, unchecked((uint)index), n.Selection == index ? 1U : 0U); return 1; }
    /// <summary>절삭한왼쪽/위쪽에셀크기를더한뒤오른쪽/아래를절삭하여실제체크무늬로선택을표시한다.</summary>
    public void Draw(uint token)
    {
        var n = Node(token); if (n.Selection < 0) return; var bounds = Require(_hooks.Bounds)(token); if (n.Columns <= 0) { int left = GumpCollection.Ftol((double)n.Selection * n.BaseWidth / n.Count + bounds.Left); bounds = bounds with { Left = left, Right = GumpCollection.Ftol((double)left + (double)n.BaseWidth / n.Count) }; }
        else { int left = GumpCollection.Ftol((double)(n.Selection % n.Columns) * n.CellWidth + bounds.Left); bounds = bounds with { Left = left, Right = GumpCollection.Ftol((double)left + n.CellWidth) }; int top = GumpCollection.Ftol((double)(n.Selection / n.Columns) * n.CellHeight + bounds.Top); bounds = bounds with { Top = top, Bottom = GumpCollection.Ftol((double)top + n.CellHeight) }; } uint color = Require(_hooks.Green)(); Require(_hooks.Fill)(bounds, color);
    }
    /// <summary>원문 ID 조회.저장소 밖은진단이며유효접두부밖은0이다.</summary>
    public uint Id(uint token, int index) { if (index < 0 || index >= Capacity) throw new ArgumentOutOfRangeException(nameof(index), "Image 목록 ID 원문 index 진단"); var n = Node(token); return (uint)index < n.Count ? n.Ids[index] : 0; }
    /// <summary>유효접두부의같은현재자식이빌린Image이며그밖은null이다.</summary>
    public RawImage? Image(uint token, int index) { var n = Node(token); return index >= 0 && (uint)index < n.Count ? Child(n.Images[index]).Image : null; }
    /// <summary>Kill만예약하고두배열의유효접두부를압축한다.끝중복칸/선택은바꾸지않는다.</summary>
    public bool Remove(uint token, int index)
    {
        if (index < 0 || index >= Capacity) throw new ArgumentOutOfRangeException(nameof(index), "Image 목록 삭제 원문 index 진단"); var n = Node(token); if ((uint)index >= n.Count || n.Count == 0) return false; Require(_hooks.Kill)(n.Images[index]); n.Count = unchecked(n.Count - 1);
        // 두 배열만 한 칸씩 앞당기며 사용하지 않는 끝 슬롯은 그대로 남긴다.
        for (uint current = (uint)index; current < n.Count; current++) { n.Images[current] = n.Images[current + 1]; n.Ids[current] = n.Ids[current + 1]; } return true;
    }
    /// <summary>목록가상표를복원한뒤실제기본소멸자로간다.빌린Image의해제는없다.</summary>
    public void Destruct(uint token) { Node(token).Vtable = ListVtable; Require(_hooks.DestructBase)(token); }
    /// <summary>같은실제트리/지연수명/현재경계/부모사건과체크무늬픽셀장치를연결한다.자식생성과현재녹색은호스트가공급한다.</summary>
    public static void BindMetaImageListDevices(MetaImageListHooks hooks, GumpTree tree, GumpLifetime lifetime, GumpCollection collection, GumpEvents events, GumpPixelTarget pixels)
    { ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(tree); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(events); ArgumentNullException.ThrowIfNull(pixels); hooks.BaseAttach = tree.Attach; hooks.Push = tree.Push; hooks.Kill = lifetime.Kill; hooks.DestructBase = lifetime.DestructBase; hooks.Bounds = collection.Bounds; hooks.Notify = (t, c, a, b) => events.Bubble(t, c, a, b); hooks.Fill = (r, c) => pixels.Fill(r, c, true); }
    /// <summary>현재목록만실제Attach/부모사건/직접입력/Draw/자식우선scalar에분배하고다른파생은유지한다.</summary>
    public static void BindMetaImageList(GumpTreeHooks tree, GumpEventHooks events, GumpInputHooks input, GumpDrawHooks draw, GumpLifetimeHooks lifetime, Func<uint, MetaImageList?> resolve, Action<uint> release)
    {
        ArgumentNullException.ThrowIfNull(resolve); ArgumentNullException.ThrowIfNull(release); var attach = tree.Attach; tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } list) list.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); };
        // 부모 재귀 사건과 직접 입력은 같은 현재Handle로 전달한다.
        uint Route(Func<uint, uint, uint, uint, uint>? previous, uint t, uint c, uint a, uint b) => resolve(t) is { } list ? list.Handle(t, c, a, b) : Require(previous)(t, c, a, b);
        var e = events.Handle; var i = input.Handle; events.Handle = (t, c, a, b) => Route(e, t, c, a, b); input.Handle = (t, c, a, b) => Route(i, t, c, a, b); var render = draw.Draw; draw.Draw = t => { if (resolve(t) is { } list) list.Draw(t); else Require(render)(t); }; var deleting = lifetime.DeletingDestructor; lifetime.DeletingDestructor = (t, flags) => { if (resolve(t) is { } list) { list.Destruct(t); if ((flags & 1) != 0) release(t); } else Require(deleting)(t, flags); };
    }
}
