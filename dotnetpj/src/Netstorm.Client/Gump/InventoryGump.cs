using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 같은 실제 내용물 SID를 표시하는 생산 격자와 항목의 기본 본체다.

/// <summary>원본0xc4 격자의 실제 기본 노드와+a0..c0 확장 필드다.</summary>
public sealed class InventoryGridNode : GumpCreateNode
{
    /// <summary>자동 배치의 최소 열 또는 고정 열 수다.</summary>
    public int MinimumColumns { get; set; }
    /// <summary>자동 배치의 최소 행 또는 고정 행 수다.</summary>
    public int MinimumRows { get; set; }
    /// <summary>현재 배율을 적용한 칸 너비다.</summary>
    public float CellWidth { get; set; }
    /// <summary>현재 배율을 적용한 칸 높이다.</summary>
    public float CellHeight { get; set; }
    /// <summary>내용물 flags WORD와 비교하는 현재 마스크다.</summary>
    public uint ContentMask { get; set; }
    /// <summary>현재 커서 전체 플래그에서 허용하는 비트다.</summary>
    public uint AcceptedCursor { get; set; }
    /// <summary>항목 자식 이미지의 실제 hit를 요구한다.</summary>
    public uint RequireImageHit { get; set; }
    /// <summary>비0이면 최소 열/행을 고정 배치로 사용한다.</summary>
    public uint Fixed { get; set; }
    /// <summary>비0일 때 항목 아이콘에 그대로 전달할 배율이다.</summary>
    public float ItemScale { get; set; }
}

/// <summary>원본0xc0 내용물 항목이다. 미작성 DWORD/WORD도 같은 객체에 보존한다.</summary>
public sealed class InventoryItemNode : GumpCreateNode
{
    /// <summary>표시하고 선택하는 실제 내용물 SID다.</summary>
    public uint Content { get; set; }
    /// <summary>생성자가 쓰지 않는+a4다.</summary>
    public uint ReservedA4 { get; set; }
    /// <summary>다음 점멸 단계의 벽시계 기한이다.</summary>
    public double BlinkUntil { get; set; }
    /// <summary>남은 점멸 단계다.</summary>
    public byte Blink { get; set; }
    /// <summary>현재 항목의 질감 장식 조건이다.</summary>
    public byte Decoration { get; set; }
    /// <summary>생성자가 쓰지 않는+b2 WORD다.</summary>
    public ushort ReservedB2 { get; set; }
    /// <summary>현재 이미지 hit 요구다.</summary>
    public uint RequireImageHit { get; set; }
    /// <summary>원문 생성자가0으로 쓰는+b8이다.</summary>
    public uint Additional { get; set; }
    /// <summary>0이면 실제 픽셀 경계에 맞추는 아이콘 배율이다.</summary>
    public float IconScale { get; set; }
}

/// <summary>복사 전역 대신 소비 시점의 같은 커서를 읽는다.</summary>
public sealed class InventoryGumpState
{
    /// <summary>현재 화면 커서 좌표다.</summary>
    public required Func<ScreenPoint> CursorPoint { get; init; }
    /// <summary>현재 내용물 커서의 전체 DWORD다.</summary>
    public required Func<uint> CursorFlags { get; init; }
}

/// <summary>같은 실제 공통 트리/등록/내용물/입력과 선택된 효과의 경계다.</summary>
public sealed class InventoryGumpHooks
{
    /// <summary>안정된 같은 격자 파생 주소다.</summary>
    public Func<uint, InventoryGridNode?>? Grid { get; set; }
    /// <summary>안정된 같은 항목 파생 주소다.</summary>
    public Func<uint, InventoryItemNode?>? Item { get; set; }
    /// <summary>공통 자식 Kill이다.</summary>
    public Action<uint>? ClearChildren { get; set; }
    /// <summary>같은 기본 GumpTree 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>실제0xc0 파생 할당이다.</summary>
    public Func<int, uint>? AllocateItem { get; set; }
    /// <summary>실제 현재 가상 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>실제 현재 가상 Push다.</summary>
    public Action<uint>? Push { get; set; }
    /// <summary>같은 현재 부모 Form 연결이다.</summary>
    public Action<uint, uint>? Bind { get; set; }
    /// <summary>같은500칸 Idle 등록이다.</summary>
    public Action<uint, bool>? Idle { get; set; }
    /// <summary>현재 실제 벽시계 초다.</summary>
    public Func<double>? Now { get; set; }
    /// <summary>현재 내용물의 부모 SID다.</summary>
    public Func<uint, uint>? ContentParent { get; set; }
    /// <summary>현재 부모 체인 전달이다.</summary>
    public Func<uint, uint, uint, uint, uint>? Forward { get; set; }
    /// <summary>같은 기본 사건 본체다.</summary>
    public Func<uint, uint, uint, uint, uint>? BaseHandle { get; set; }
    /// <summary>현재 화면 커서의 실제 로컬 좌표 변환이다.</summary>
    public Func<uint, uint, ScreenPoint, (float X, float Y)>? Local { get; set; }
    /// <summary>실제 로컬 경계 판정이다.</summary>
    public Func<uint, (float X, float Y), bool>? Inside { get; set; }
    /// <summary>실제 내용물 커서를 취소한다.</summary>
    public Action? CancelCursor { get; set; }
    /// <summary>정상 복귀하면 원문을 계속하는 진단이다.</summary>
    public Action<string>? Assertion { get; set; }
    /// <summary>현재 가시성 변경 순서의 가상 redraw다.</summary>
    public Action<uint, bool>? Redraw { get; set; }
    /// <summary>종료 redraw 뒤 현재 내용물+15의bit2만 지운다.</summary>
    public Action<uint>? ClearBlink { get; set; }
}

/// <summary>Source InventoryGump의 생성·두 순회 배치·점멸·사건·부모 삭제 전체다.</summary>
public sealed class InventoryGump
{
    /// <summary>같은 기본 생성/등록 본체다.</summary>
    private readonly GumpCreate _factory;
    /// <summary>유일한 실제 내용물 물리다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 동일 목록 순회 정책이다.</summary>
    private readonly ContainedFinderState _finder;
    /// <summary>같은 현재 커서 getter다.</summary>
    private readonly InventoryGumpState _state;
    /// <summary>선택된 실제 외부 장치다.</summary>
    private readonly InventoryGumpHooks _hooks;
    /// <summary>도달한 실제 효과만 요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("생산 격자 실제 하위 미연결");
    /// <summary>PATCH의 실제 기본 생성/풀/정책을 빌리며 새 내용물 표를 만들지 않는다.</summary>
    public InventoryGump(GumpCreateState create, GumpCreate factory, RawSidPool pool, ContainedFinderState finder, InventoryGumpState state, InventoryGumpHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(finder); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (create.Collection.Edition != OriginalEdition.Patch1078 || pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("생산 격자 PATCH 자료 필요");
        _factory = factory; _pool = pool; _finder = finder; _state = state; _hooks = hooks;
    }
    /// <summary>같은 실제 파생 주소만 요구한다.</summary>
    private InventoryGridNode Grid(uint token) => (token != 0 ? Need(_hooks.Grid)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "생산 격자 저장소 없음");
    /// <summary>같은 실제 항목 주소만 요구한다.</summary>
    private InventoryItemNode Item(uint token) => (token != 0 ? Need(_hooks.Item)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "생산 항목 저장소 없음");
    /// <summary>DWORD의 물리 범위를 확인한 뒤에만 실제 SID로 좁힌다.</summary>
    private int Content(uint sid) => sid != 0 && sid < _pool.Capacity && sid <= ushort.MaxValue ? (int)sid : throw new ArgumentOutOfRangeException(nameof(sid), "생산 격자 내용물 SID 범위 오류");
    /// <summary>비정렬 현재 flags WORD다.</summary>
    private uint Flags(int sid) => BinaryPrimitives.ReadUInt16LittleEndian(_pool.Slot(sid)[20..]);
    /// <summary>CRT64비트 절삭의 하위 DWORD이며 불확정 정수는 하위0이다.</summary>
    internal static int Ftol(double value) => !double.IsFinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0 ? 0 : unchecked((int)(long)value);
    /// <summary>미작성 칸 크기는 보존하며 원문 종류12/마스크/배치 조건을 쓴다.</summary>
    public uint ConstructGrid(uint token, int columns, int rows, uint mask)
    {
        var node = Grid(token); _factory.Construct(token, 0, 0, 0, 1); node.Vtable = 0x506800; if (columns == 0) Need(_hooks.Assertion)("_numSlotsX != 0");
        node.Command = 12; node.MinimumColumns = columns; node.Flags |= 0xa00; node.AcceptedCursor = 0; node.ContentMask = mask; node.Fixed = 0; node.ItemScale = 0; node.MinimumRows = rows; node.RequireImageHit = 1; return token;
    }
    /// <summary>실제 내용물 flags200만 점멸을 시작하며 미작성+a4/+b2는 유지한다.</summary>
    public uint ConstructItem(uint token, float width, float height, uint content)
    {
        var node = Item(token); _factory.Construct(token, 0, width, height, 1); node.Vtable = 0x5068f4; node.BlinkUntil = 0; node.Flags |= 0x1a00;
        node.Content = content; node.Command = 8; node.Blink = 0; node.Decoration = 0; node.Additional = 0; node.IconScale = 0;
        if ((Flags(Content(content)) & 0x200) != 0) BeginBlink(token); node.RequireImageHit = 1; return token;
    }
    /// <summary>단계4→실제 Idle 등록→현재 시각+0.1 순서다.</summary>
    public void BeginBlink(uint token) { var node = Item(token); node.Blink = 4; Need(_hooks.Idle)(token, true); node.BlinkUntil = Need(_hooks.Now)() + 0.1; }
    /// <summary>현재 기한/두 시각 조회와 표시 변경 앞뒤의 가상 갱신 순서다.</summary>
    public void IdleItem(uint token)
    {
        var node = Item(token);
        if (node.Blink == 0) { Need(_hooks.Idle)(token, false); node.Flags |= 1; Need(_hooks.Redraw)(token, false); Need(_hooks.ClearBlink)(node.Content); return; }
        double now = Need(_hooks.Now)();
        if (node.BlinkUntil <= now) { node.Blink = unchecked((byte)(node.Blink - 1)); node.BlinkUntil = Need(_hooks.Now)() + 0.1; if ((node.Flags & 1) != 0) { Need(_hooks.Redraw)(token, false); node.Flags &= ~1U; } else { node.Flags |= 1; Need(_hooks.Redraw)(token, false); } }
    }
    /// <summary>기본 부착 뒤 같은 목록을 두 번 순회해 현재 슬롯/크기로 실제 항목을 만든다.</summary>
    public void AttachGrid(uint token, float x, float y, uint parent, uint mode)
    {
        var node = Grid(token); Need(_hooks.ClearChildren)(token); Need(_hooks.BaseAttach)(token, x, y, parent, mode); var finder = new RawContainedFinder(_pool, _finder); uint maximum = 0;
        // 첫 순회는 현재 mask와 일치하는 slot BYTE+1의 최댓값이다.
        for (int sid = finder.Begin(node.Object != 0 ? Content(node.Object) : 0); sid != 0; sid = finder.Next()) if ((Flags(sid) & node.ContentMask) != 0) maximum = Math.Max(maximum, (uint)_pool.Slot(sid)[23] + 1);
        int columns = node.MinimumColumns, rows = node.MinimumRows;
        if (node.Fixed == 0)
        {
            float count = maximum, root = MathF.Sqrt(count); columns = Math.Max(1, Ftol(Math.Ceiling(((double)node.BaseWidth / node.BaseHeight) * root))); rows = Math.Max(1, Ftol((double)count / columns));
            if (unchecked(rows * columns) < (int)maximum) rows = unchecked(rows + 1); if (columns == 1 && rows < node.MinimumRows) rows = node.MinimumRows; if (rows == 1 && columns < node.MinimumColumns) columns = node.MinimumColumns;
        }
        if (columns == 0 || rows == 0) throw new DivideByZeroException("생산 격자 원본0열/행 나눗셈 경로"); node.CellWidth = (float)((double)node.Width / columns); node.CellHeight = (float)((double)node.Height / rows);
        // 두 번째 순회는 효과 뒤 현재 목록/마스크를 다시 읽고 실제 내용물 SID를 전달한다.
        for (int sid = finder.Begin(node.Object != 0 ? Content(node.Object) : 0); sid != 0; sid = finder.Next())
        {
            if ((Flags(sid) & node.ContentMask) == 0) continue; int slot = _pool.Slot(sid)[23]; uint child = Need(_hooks.AllocateItem)(0xc0); if (child == 0) throw new OutOfMemoryException();
            ConstructItem(child, node.CellWidth, node.CellHeight, (uint)sid); var item = Item(child); if (node.ItemScale != 0) item.IconScale = node.ItemScale; item.RequireImageHit = node.RequireImageHit;
            Need(_hooks.Attach)(child, (float)((slot % columns) * (double)node.CellWidth), (float)((slot / columns) * (double)node.CellHeight), token, 0);
        }
    }
    /// <summary>현재 내용물 통지/커서/마스크/부모 전달·왼쪽 취소와 기본 사건을 원문 순서로 처리한다.</summary>
    public uint HandleGrid(uint token, uint code, uint first, uint second)
    {
        var node = Grid(token); uint kind = code & 0x03ffffff;
        if (kind == 0x02000031) { int content = Content(first); uint parent = Need(_hooks.ContentParent)(first); if (parent == node.Object && (node.ContentMask & Flags(content)) != 0) { Need(_hooks.Forward)(token, code, first, second); Need(_hooks.Push)(token); Need(_hooks.Attach)(token, node.X, node.Y, 0, 0); } }
        if (kind == 0x0200003f) { var point = Need(_hooks.Local)(token, kind, _state.CursorPoint()); if (Need(_hooks.Inside)(token, point) && (node.AcceptedCursor & _state.CursorFlags()) != 0) return 1; }
        if (kind == 0x0200005f) return node.ContentMask; if (kind == 0x0200005e) return Need(_hooks.Forward)(token, code, first, second);
        if ((code & 0x80000000) == 0 && kind == 0x10000) { Need(_hooks.CancelCursor)(); return 1; } return Need(_hooks.BaseHandle)(token, code, first, second);
    }
    /// <summary>진단→Push→연결 해제의 효과 뒤 현재 위치로 가상 재부착한다.</summary>
    public void ParentGone(uint token, uint parent) { var node = Grid(token); if (parent != node.Object) Need(_hooks.Assertion)("sid == parentSid"); Need(_hooks.Push)(token); Need(_hooks.Bind)(token, 0); Need(_hooks.Attach)(token, node.X, node.Y, 0, 0); }
}
