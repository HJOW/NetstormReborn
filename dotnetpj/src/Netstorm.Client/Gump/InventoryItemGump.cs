using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 실제 생산 항목의 이미지/수량·입력·비용을 같은 내용물에 연결한다.

/// <summary>TYPE+11c와 현재 원본 프레임 코드 표다.</summary>
/// <param name="GumpFrame">-1이면 내용물 인자를 사용한다.</param><param name="Frames">같은 실제 프레임 코드 표다.</param>
public readonly record struct InventoryItemType(int GumpFrame, RiftTypeFrames? Frames);
/// <summary>현재 전체 decoder의 한 유효 칸이다.</summary>
/// <param name="Frame">현재 프레임.</param><param name="X">현재 가로 좌표.</param><param name="Y">현재 세로 좌표.</param>
public readonly record struct InventoryItemCell(int Frame, float X, float Y);

/// <summary>초기 사본 대신 실제 장기 owner의 현재 필드를 읽는다.</summary>
public sealed class InventoryItemState
{
    /// <summary>현재 SP 조회 owner다.</summary>
    public required Func<uint> LocalOwner { get; init; }
    /// <summary>현재 에디터 상태다.</summary>
    public required Func<bool> Editor { get; init; }
    /// <summary>현재 수량 표시 DWORD다.</summary>
    public required Func<uint> StockEnabled { get; init; }
    /// <summary>현재 입력 차단 DWORD다.</summary>
    public required Func<uint> InputDisabled { get; init; }
    /// <summary>같은 최근 선택 TYPE의 쓰기 가능한 실제 주소다.</summary>
    public required UIntBox LastType { get; init; }
    /// <summary>현재 실제 다리 TYPE다.</summary>
    public required Func<uint> BridgeType { get; init; }
    /// <summary>현재 지도 셀의 가로 픽셀 단위다.</summary>
    public required Func<float> PixelsX { get; init; }
    /// <summary>현재 지도 셀의 세로 픽셀 단위다.</summary>
    public required Func<float> PixelsY { get; init; }
    /// <summary>장식의 현재 질감 TYPE다.</summary>
    public required Func<uint> BackgroundType { get; init; }
    /// <summary>장식의 현재 질감 프레임이다.</summary>
    public required Func<uint> BackgroundFrame { get; init; }
}

/// <summary>현재 항목과 기존 이미지·문자·기하·트리/수명의 실제 경계다.</summary>
public sealed class InventoryItemHooks
{
    /// <summary>안정된 실제 항목이다.</summary>
    public Func<uint, InventoryItemNode?>? Node { get; set; }
    /// <summary>같은 실제 자식/기본 노드다.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 실제 gumpFrame/프레임 코드다.</summary>
    public Func<uint, InventoryItemType>? Type { get; set; }
    /// <summary>같은 기본 트리 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? BaseAttach { get; set; }
    /// <summary>실제 자식 Kill이다.</summary>
    public Action<uint>? ClearChildren { get; set; }
    /// <summary>현재 공통 지연 삭제 정리다.</summary>
    public Action? Cleanup { get; set; }
    /// <summary>실제 SID allowDead 검사다.</summary>
    public Func<uint, bool, bool>? Robust { get; set; }
    /// <summary>복귀하면 부분 상태에서 계속하는 진단이다.</summary>
    public Action<string>? Assertion { get; set; }
    /// <summary>전체 패턴 픽셀 측정이다.</summary>
    public Func<uint, uint, bool, PriestPlacementPixelShape>? Measure { get; set; }
    /// <summary>같은 실제 decoder의 모든 유효 칸이다.</summary>
    public Action<CanonTypeQuery, Action<InventoryItemCell>>? Walk { get; set; }
    /// <summary>같은0xb4 이미지/0xa4 문자 파생 할당이다.</summary>
    public Func<int, uint>? Allocate { get; set; }
    /// <summary>실제 ImageGump 생성자다.</summary>
    public Action<uint, uint, int, uint>? Image { get; set; }
    /// <summary>같은 한 줄 문자 생성자다.</summary>
    public Action<uint>? Text { get; set; }
    /// <summary>같은 실제 스타일 파싱이다.</summary>
    public Action<uint, string>? StyledText { get; set; }
    /// <summary>같은 실제 문자 맞춤이다.</summary>
    public Action<uint>? FitText { get; set; }
    /// <summary>같은 자식 가상 부착이다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>현재 실제 비용표 복호 정수다.</summary>
    public Func<uint, int>? CostUnits { get; set; }
    /// <summary>실제 현재 owner의 SP다.</summary>
    public Func<uint, float>? Money { get; set; }
    /// <summary>현재 실제 화면점이다.</summary>
    public Func<uint, ScreenPoint>? Position { get; set; }
    /// <summary>원문 질감 장식 본체다.</summary>
    public Action<ScreenRect, uint, uint, uint>? Background { get; set; }
    /// <summary>현재 사건의 실제 로컬 좌표다.</summary>
    public Func<uint, uint, uint, uint, (float X, float Y)>? Local { get; set; }
    /// <summary>현재 지식 창 관문이다.</summary>
    public Func<uint>? KnowledgeOpen { get; set; }
    /// <summary>실제 이미지 픽셀 적중이다.</summary>
    public Func<uint, float, float, uint>? ImageHit { get; set; }
    /// <summary>실제 정책 질의다.</summary>
    public Func<uint, uint, uint>? Denied { get; set; }
    /// <summary>실제 미션 안내다.</summary>
    public Action<string>? Tell { get; set; }
    /// <summary>현재 실제 부모 체인 전달이다.</summary>
    public Func<uint, uint, uint, uint, uint>? Forward { get; set; }
    /// <summary>실제 내용물 정보 메뉴다.</summary>
    public Func<uint, uint, uint, uint>? About { get; set; }
}

/// <summary>Source InventoryItemGump의 전체 부착·장식·입력·비용과 실제 자산 연결이다.</summary>
public sealed class InventoryItemGump
{
    /// <summary>유일한 실제 내용물 물리다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>현재 실제 Client getter와 쓰기 주소다.</summary>
    private readonly InventoryItemState _state;
    /// <summary>현재 실제 선택 장치다.</summary>
    private readonly InventoryItemHooks _hooks;
    /// <summary>선택한 실제 하위만 도달 위치에서 요구한다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("생산 항목 실제 하위 미연결");
    /// <summary>현재 동일 PATCH 물리를 빌린다.</summary>
    public InventoryItemGump(RawSidPool pool, InventoryItemState state, InventoryItemHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); if (pool.Edition != OriginalEdition.Patch1078) throw new ArgumentException("생산 항목 PATCH 자료 필요"); _pool = pool; _state = state; _hooks = hooks;
    }
    /// <summary>같은 현재 파생 주소다.</summary>
    private InventoryItemNode Node(uint token) => (token != 0 ? Need(_hooks.Node)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "생산 항목 저장소 없음");
    /// <summary>같은 실제 자식 주소다.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? Need(_hooks.Base)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "생산 항목 자식 저장소 없음");
    /// <summary>물리 범위 확인 후에만 SID를 좁힌다.</summary>
    private int Content(uint sid) => sid != 0 && sid < _pool.Capacity && sid <= ushort.MaxValue ? (int)sid : throw new ArgumentOutOfRangeException(nameof(sid), "생산 항목 실제 내용물SID 오류");
    /// <summary>같은 현재 비정렬 WORD다.</summary>
    private uint Word(int sid, int offset) => BinaryPrimitives.ReadUInt16LittleEndian(_pool.Slot(sid)[offset..]);
    /// <summary>비용을 먼저 float으로 저장하고 에디터에서도 현재 SP를 읽는다.</summary>
    public uint Affordable(uint content) { uint kind = Word(Content(content), 18); float cost = Need(_hooks.CostUnits)(kind), money = Need(_hooks.Money)(_state.LocalOwner()); return _state.Editor() || cost <= money ? 1U : 0U; }
    /// <summary>네 dead 진단/삭제 큐 뒤 전체 현재 패턴으로 실제 그림·수량 자식을 만든다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        var node = Node(token);
        // 각 실제 효과 뒤 같은 파생의 현재 죽음 플래그를 재검사한다.
        void Alive() { if ((node.Flags & 0x10000) != 0) Need(_hooks.Assertion)("!(flags & GF_DEAD)"); }
        Alive(); Need(_hooks.BaseAttach)(token, x, y, parent, mode); Alive(); Need(_hooks.ClearChildren)(token); Alive(); Need(_hooks.Cleanup)(); Alive();
        if (!Need(_hooks.Robust)(node.Content, true)) Need(_hooks.Assertion)("isSidRobust( contentSid )");
        int sid = Content(node.Content); uint kind = Word(sid, 18), argument = Word(sid, 12); var info = Need(_hooks.Type)(kind); bool explicitFrame = info.GumpFrame != -1;
        if (explicitFrame) argument = unchecked((uint)Need(_hooks.Type)(kind).GumpFrame); if (kind == 0) return;
        var bounds = Need(_hooks.Measure)(kind, argument, explicitFrame).Bounds; float scale = 1;
        if (node.IconScale == 0) { float rx = (float)(((double)bounds.Right - bounds.Left + 2.0) / node.Width), ry = (float)(((double)bounds.Bottom - bounds.Top + 2.0) / node.Height); if (rx > 1 || ry > 1) { float maximum = rx <= ry ? ry : rx; scale = (float)(1.0 / maximum); } } else scale = node.IconScale;
        float centerX = (float)(((double)node.Width - ((double)bounds.Right - bounds.Left) * scale) * 0.5 + 2.0), centerY = (float)(((double)node.Height - ((double)bounds.Bottom - bounds.Top) * scale) * 0.5 + 2.0);
        Need(_hooks.Walk)(new(kind, unchecked((int)argument), 0, 0, 0, explicitFrame), cell =>
        {
            float px = (float)((double)centerX + (double)cell.X * _state.PixelsX() * scale), py = (float)((double)centerY + (double)cell.Y * _state.PixelsY() * scale); int frame = cell.Frame;
            if (Word(sid, 18) == _state.BridgeType()) { var bridge = Need(_hooks.Type)(_state.BridgeType()); if (bridge.Frames is null || frame < 0 || frame >= bridge.Frames.Codes.Count) throw new ArgumentOutOfRangeException(nameof(frame), "생산 항목 현재 다리 프레임 코드 오류"); var code = bridge.Frames.Codes[frame]; byte quality = _pool.Slot(sid)[30]; uint signedFlags = unchecked((uint)(int)(sbyte)code.Flags); int flags = unchecked((int)(signedFlags | (quality == 1 ? 0x20U : quality == 3 ? 0x40U : 0U))); frame = bridge.Frames.FindFlags(code.Side, code.Variant, flags); }
            uint contentFlags = Word(sid, 20), imageMode = (contentFlags & 0x100) != 0 ? 1U : (contentFlags & 0x418) != 0 && Affordable((uint)sid) == 0 ? 2U : 0U;
            uint image = Need(_hooks.Allocate)(0xb4); if (image == 0) throw new OutOfMemoryException(); Need(_hooks.Image)(image, kind, frame, imageMode); var child = Base(image); child.Scale = scale; child.Width = (float)((double)scale * child.BaseWidth); child.Height = (float)((double)scale * child.BaseHeight); Need(_hooks.Attach)(image, px, py, token, 0);
            if (_state.StockEnabled() != 0 && (Word(sid, 20) & 8) != 0) { uint text = Need(_hooks.Allocate)(0xa4); if (text == 0) throw new OutOfMemoryException(); Need(_hooks.Text)(text); Need(_hooks.StyledText)(text, "~w~E" + _pool.Slot(sid)[30].ToString(CultureInfo.InvariantCulture)); Need(_hooks.FitText)(text); float tx = (float)((double)px + child.Width), ty = (float)((double)py + child.Height - 10.0); Need(_hooks.Attach)(text, tx, ty, token, 0); }
        });
    }
    /// <summary>현재 장식 조건과 위치/기본 크기로 원문 정수 영역만 그린다.</summary>
    public void Draw(uint token) { var node = Node(token); if (node.Decoration == 0) return; var p = Need(_hooks.Position)(token); Need(_hooks.Background)(new(unchecked(p.X + 3), unchecked(p.Y + 3), InventoryGump.Ftol((double)p.X + node.BaseWidth), InventoryGump.Ftol((double)p.Y + node.BaseHeight)), 0x100000, _state.BackgroundType(), _state.BackgroundFrame()); }
    /// <summary>로컬 변환 뒤 현재 관문을 읽고 실제 자식 픽셀/부모 전달/정보 메뉴를 선택한다.</summary>
    public uint Handle(uint token, uint code, uint first, uint second)
    {
        var node = Node(token); var point = Need(_hooks.Local)(token, code, first, second); if (Need(_hooks.KnowledgeOpen)() != 0 || _state.InputDisabled() != 0) return 0; uint kind = code & 0x03ffffff;
        if (kind == 0x02000107) return _state.LastType.Value == Word(Content(node.Content), 18) ? Need(_hooks.Forward)(token, 0x02000008, node.Content, node.Serial) : 0;
        if ((kind != 0x10000 && kind != 0x20000) || (code & 0x80000000) != 0) return 0; var visited = new HashSet<uint>();
        // 실패 뒤 현재 next를 다시 읽으며 순환은 실제 방문 시점에 거부한다.
        for (uint child = node.Child; child != 0; child = Base(child).Next)
        {
            if (!visited.Add(child)) throw new InvalidOperationException("생산 항목 자식 순환"); var current = Base(child); if (node.RequireImageHit != 0 && (current.Command != 3 || Need(_hooks.ImageHit)(child, point.X, point.Y) == 0)) continue;
            if (kind == 0x10000) { if (Need(_hooks.Denied)(1, 0x41) == 0) { _state.LastType.Value = Word(Content(node.Content), 18); return Need(_hooks.Forward)(token, 0x02000008, node.Content, node.Serial); } Need(_hooks.Tell)("NoBridgeYet"); }
            else if (Need(_hooks.Forward)(token, 0x02000018, node.Content, node.Serial) == 0 && Need(_hooks.KnowledgeOpen)() == 0) return Need(_hooks.About)(token, first, second);
            return 1;
        }
        return 0;
    }
    /// <summary>기존 실제 TYPE+11c/프레임·전체 픽셀 getter/decoder만 연결하고 GUI·내용물·SP 장치는 유지한다.</summary>
    public static void BindAssets(InventoryItemHooks hooks, OriginalEdition edition, IReadOnlyList<RiftTypeDefinition> definitions, IReadOnlyList<PriestPlainCanonType> frames, PriestPlacementGeometryState geometry, RawCanonPixelShape measure)
    {
        ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(definitions); ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(geometry); ArgumentNullException.ThrowIfNull(measure);
        if (edition != OriginalEdition.Patch1078 || measure.Pool.Edition != edition) throw new ArgumentException("생산 항목 실제 자산 판본 오류");
        hooks.Type = type => type < frames.Count ? new(type < 70 ? 0 : definitions[checked((int)type - 70)].SpecialFrames.GumpFrame, frames[(int)type].Frames) : throw new ArgumentOutOfRangeException(nameof(type), "생산 항목 실제 TYPE 프레임 범위 오류");
        hooks.Measure = (type, argument, explicitFrame) => measure.Measure(type, argument, 0, explicitFrame);
        hooks.Walk = (query, visitor) =>
        {
            if (query.Type >= frames.Count) throw new ArgumentOutOfRangeException(nameof(query), "생산 항목 실제 decoder 타입 오류"); var meta = frames[(int)query.Type]; var decoder = CanonTypeDecoder.DecodeCanonType(meta.Frames, meta.DefaultFrame, geometry.PatternTypes, query, edition, geometry.DecoderDiagnostics);
            // 모든 현재 유효 칸의 실제 visitor 효과 뒤 Advance한다.
            while (decoder.Valid()) { visitor(new(decoder.Frame(), decoder.X(), decoder.Y())); decoder.Advance(); }
        };
    }
}
