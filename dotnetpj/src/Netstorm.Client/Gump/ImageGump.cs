using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 원본 생산 아이콘의 같은 Gump/자산/색표/투명 적중 영역이다.

/// <summary>VT00501bb0의+a0..b0.기본 등록/트리/수명을 같은 객체에서 상속한다.</summary>
public sealed class ImageGumpNode : GumpCreateNode
{
    /// <summary>현재 실제TYPE 번호.</summary>
    public uint Type { get; set; }
    /// <summary>현재 실제SHP 물리 프레임.</summary>
    public uint Frame { get; set; }
    /// <summary>선택/비활성/그림자/단위 소유색 우회 비트.</summary>
    public uint Mode { get; set; }
    /// <summary>SHP signed기준점−TYPE int기준점을 마지막에만float으로 저장한X.</summary>
    public float HotspotX { get; set; }
    /// <summary>같은 원문 연산의Y.</summary>
    public float HotspotY { get; set; }
}
/// <summary>현재 TYPE의 실제 SHP 토큰과 두signed기준점.</summary>
/// <param name="Shape">현재 SHP 토큰.</param><param name="HotspotX">TYPE 기준점X.</param><param name="HotspotY">TYPE 기준점Y.</param>
public readonly record struct ImageGumpType(uint Shape, int HotspotX, int HotspotY);
/// <summary>추가 헤더의 실제signed WORD4개.</summary>
/// <param name="Width">폭.</param><param name="Height">높이.</param><param name="HotspotX">기준점X.</param><param name="HotspotY">기준점Y.</param>
public readonly record struct ImageGumpHeader(short Width, short Height, short HotspotX, short HotspotY);
/// <summary>같은 실제VFX 일반/변환 출력.회전은 원문처럼0이다.</summary>
/// <param name="Type">TYPE 번호.</param><param name="Shape">SHP 토큰.</param><param name="Frame">물리 프레임.</param><param name="Position">실제 기준점.</param><param name="Scale">두 축의signed16.16 배율.</param><param name="Scaled">변환 분기.</param><param name="Mapped">현재256색표 분기.</param>
public readonly record struct ImageGumpDrawRequest(uint Type, uint Shape, uint Frame, ScreenPoint Position, int Scale, bool Scaled, bool Mapped);
/// <summary>원본 임시200×200 그림 적중에 전달하는 원시 입력.</summary>
/// <param name="Type">TYPE 번호.</param><param name="Shape">SHP 토큰.</param><param name="Frame">물리 프레임.</param><param name="Point">로컬 검사점.</param><param name="Origin">이미지 자식 위치를 포함한 기준점.</param><param name="Scale">두 축의signed16.16 배율.</param>
public readonly record struct ImageGumpHitRequest(uint Type, uint Shape, uint Frame, ScreenPoint Point, ScreenPoint Origin, int Scale);
/// <summary>생성 시 복사하지 않는 현재 로컬owner/TYPE색표/그림자 위치 전역.</summary>
public sealed class ImageGumpState
{
    /// <summary>현재 원시owner DWORD.출력에서signed로 읽는다.</summary>
    public uint LocalOwner { get; set; }
    /// <summary>현재TYPE별9색표 기본 토큰.</summary>
    public uint[] OwnerMaps { get; set; } = [];
    /// <summary>선행 그림의 현재 정수 오프셋.</summary>
    public ScreenPoint ShadowOffset { get; set; }
}
/// <summary>같은 공통 저장소/현재 자산/표면의 실제 외부 경계.</summary>
public sealed class ImageGumpHooks
{
    /// <summary>안정된 같은 파생 노드.</summary>
    public Func<uint, ImageGumpNode?>? Node { get; set; }
    /// <summary>현재TYPE의SHP와기준점.</summary>
    public Func<uint, ImageGumpType>? Type { get; set; }
    /// <summary>현재 실제 프레임 헤더.</summary>
    public Func<uint, uint, ImageGumpHeader>? Header { get; set; }
    /// <summary>같은 공통Gump 화면 위치.</summary>
    public Func<uint, ScreenPoint>? Position { get; set; }
    /// <summary>실제 토큰의256색표 설치.</summary>
    public Action<uint>? Color { get; set; }
    /// <summary>같은 표면의 실제VFX 출력.</summary>
    public Action<ImageGumpDrawRequest>? Draw { get; set; }
    /// <summary>실제 임시 그림 적중.</summary>
    public Func<ImageGumpHitRequest, uint>? Hit { get; set; }
    /// <summary>같은 실제 기본Gump 부착.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
}
/// <summary>cpppj/src/client/ImageGump.cpp의 원문 float저장/현재 전역/색표 순서/적중이다.</summary>
public sealed class ImageGump
{
    /// <summary>원본10.78의 이미지 가상 표.</summary>
    private const uint Vtable = 0x501bb0;
    /// <summary>선행 그림 색표 토큰.</summary>
    private const uint Shadow = 0x556338;
    /// <summary>선택 색표 토큰.</summary>
    private const uint Selected = 0x556438;
    /// <summary>비활성 색표 토큰.</summary>
    private const uint Disabled = 0x556038;
    /// <summary>원본16.16의단위값.</summary>
    private const int FixedUnit = 65536;
    /// <summary>같은 실제 기본 생성/등록 처리기.</summary>
    private readonly GumpCreate _factory;
    /// <summary>현재owner/색표/선행 위치의 장기 상태.</summary>
    private readonly ImageGumpState _state;
    /// <summary>현재 파생/자산/효과 경계.</summary>
    private readonly ImageGumpHooks _hooks;
    /// <summary>실제 도달한 장치만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("이미지 Gump 실제 하위 미연결");
    /// <summary>원문 계약이 복원된10.78만 받는다.장치나 창은 만들지 않는다.</summary>
    public ImageGump(GumpCreateState create, GumpCreate factory, ImageGumpState state, ImageGumpHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); if (create.Collection.Edition != OriginalEdition.Patch1078) throw new ArgumentException("이미지 Gump PATCH 자료 필요"); _factory = factory; _state = state; _hooks = hooks; }
    /// <summary>호출한 위치의 실제 파생 저장소를 조회한다.</summary>
    private ImageGumpNode Node(uint token) => (token != 0 ? Require(_hooks.Node)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "이미지 Gump 저장소 없음");
    /// <summary>같은 기본 생성/등록→타입BYTE3→프레임 설정 순서다.</summary>
    public uint Construct(uint token, uint type, uint frame, uint mode) { var n = Node(token); _factory.Construct(token, 0, 0, 0, 1); n.Vtable = Vtable; n.Command = 3; SetFrame(token, type, frame, mode); return token; }
    /// <summary>TYPE int기준점을float으로 미리 좁히지 않는다.마지막 차이와 배율 크기만float으로 저장한다.</summary>
    public void SetFrame(uint token, uint type, uint frame, uint mode)
    { var n = Node(token); n.Type = type; n.Frame = frame; n.Mode = mode; var data = Require(_hooks.Type)(type); var header = Require(_hooks.Header)(type, n.Frame); n.BaseWidth = header.Width; n.Width = (float)((double)header.Width * n.Scale); n.BaseHeight = header.Height; n.Height = (float)((double)header.Height * n.Scale); n.HotspotX = (float)((double)header.HotspotX - data.HotspotX); n.HotspotY = (float)((double)header.HotspotY - data.HotspotY); }
    /// <summary>그림자→선택/비활성→현재owner색/일반의 원문 분기.색 설치 뒤 현재 필드를 다시 읽는다.</summary>
    public void Draw(uint token)
    {
        var n = Node(token); var point = Require(_hooks.Position)(token); var data = Require(_hooks.Type)(n.Type); bool unit = BitConverter.SingleToUInt32Bits(n.Scale) == 0x3f800000;
        // 저장된 위치/단위 분기와 현재 기준점/배율/그림자 위치를 결합한다.
        void Paint(bool mapped, bool shadow) { int px = shadow ? unchecked(point.X + _state.ShadowOffset.X) : point.X, py = shadow ? unchecked(point.Y + _state.ShadowOffset.Y) : point.Y; int x = GumpCollection.Ftol((double)px + (double)n.HotspotX * (unit ? 1.0 : n.Scale)), y = GumpCollection.Ftol((double)py + (double)n.HotspotY * (unit ? 1.0 : n.Scale) - (shadow && unit ? 1.0 : 0.0)); Require(_hooks.Draw)(new(n.Type, data.Shape, n.Frame, new(x, y), unit ? FixedUnit : GumpCollection.Ftol((double)n.Scale * FixedUnit), !unit, mapped)); }
        if ((n.Mode & 4) != 0) { Require(_hooks.Color)(Shadow); Paint(true, true); } if ((n.Mode & 3) != 0) { Require(_hooks.Color)((n.Mode & 1) != 0 ? Selected : Disabled); Paint(true, false); return; }
        if (n.Type >= _state.OwnerMaps.Length) throw new ArgumentOutOfRangeException(nameof(token), "이미지 Gump 현재TYPE 색표 범위 오류"); int owner = unchecked((int)_state.LocalOwner); uint table = _state.OwnerMaps[n.Type]; if ((!unit || (n.Mode & 8) == 0) && table != 0 && owner > 0) { Require(_hooks.Color)(unchecked(table + (_state.LocalOwner << 8))); Paint(true, false); } else Paint(false, false);
    }
    /// <summary>현재 자식 로컬 위치를 포함한 그림 기준점과 같은16.16을 실제 임시 버퍼에 전달한다.</summary>
    public uint Hit(uint token, float x, float y) { var n = Node(token); var type = Require(_hooks.Type)(n.Type); int scale = GumpCollection.Ftol((double)n.Scale * FixedUnit); return Require(_hooks.Hit)(new(n.Type, type.Shape, n.Frame, new(GumpCollection.Ftol(x), GumpCollection.Ftol(y)), new(GumpCollection.Ftol((double)n.HotspotX * n.Scale + n.X), GumpCollection.Ftol((double)n.HotspotY * n.Scale + n.Y)), scale)); }
    /// <summary>같은 기본 부착으로 전달한다.별도 보정/자식 생성은 없다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode) => Require(_hooks.Attach)(token, x, y, parent, mode);
    /// <summary>현재 이미지만 실제 가상Attach/Draw로 분배하고 다른 파생 경계를 보존한다.</summary>
    public static void BindImageGump(GumpTreeHooks tree, GumpDrawHooks draw, Func<uint, ImageGump?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var attach = tree.Attach; tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } image) image.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); }; var render = draw.Draw; draw.Draw = t => { if (resolve(t) is { } image) image.Draw(t); else Require(render)(t); }; }
}

/// <summary>전체GameAssets 조립 전에도 실제TYPE/SHP/논리수/기준점을 연결하는 입력.현재 TYPE 필드는 같은 객체에서 재조회한다.</summary>
public sealed class ImageGumpAssetBinding
{
    /// <summary>원본TYPE 번호.</summary>
    public uint Type { get; init; }
    /// <summary>실제SHP 블록.토큰은block+1이다.</summary>
    public int Block { get; init; }
    /// <summary>원본 프레임 코드 수.물리 수로 대체하지 않는다.</summary>
    public int FrameCount { get; set; }
    /// <summary>추가 레이어를 결정하는 현재flags1.</summary>
    public uint Flags { get; set; }
    /// <summary>현재TYPE signed 기준점X.</summary>
    public int HotspotX { get; set; }
    /// <summary>현재TYPE signed 기준점Y.</summary>
    public int HotspotY { get; set; }
}
/// <summary>원문 실제TYPE/SHP 조회의 읽기 전용 연결.전체GameAssets 소유/초기화는 상위에서 맡는다.</summary>
public sealed class ImageGumpAssets
{
    /// <summary>현재 실제SHP 데이터베이스.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>현재 TYPE필드/논리수와 안정된 물리 블록.</summary>
    private readonly Dictionary<uint, ImageGumpAssetBinding> _bindings = [];
    /// <summary>10.78의 명시 자산 대응만 받는다.사용하지 않는 프레임은 미리 정규화하지 않는다.</summary>
    public ImageGumpAssets(ShapeDatabase shapes, IEnumerable<ImageGumpAssetBinding> bindings, OriginalEdition edition = OriginalEdition.Patch1078)
    { ArgumentNullException.ThrowIfNull(shapes); ArgumentNullException.ThrowIfNull(bindings); if (edition != OriginalEdition.Patch1078) throw new ArgumentException("이미지 Gump 실제PATCH 자산 필요"); _shapes = shapes;
        // 같은 원본 로딩 순서의 현재TYPE 입력 객체를 빌린다.
        foreach (var b in bindings) { if (b.Type < 70 || b.Block < 0 || b.Block >= shapes.Blocks().Count || b.FrameCount < 0) throw new ArgumentException("이미지 Gump 자산 연결 범위 오류"); _bindings.Add(b.Type, b); }
    }
    /// <summary>현재 실제TYPE만 조회하고 내장/미연결 자료는 거부한다.</summary>
    private ImageGumpAssetBinding Binding(uint type) => _bindings.TryGetValue(type, out var b) ? b : throw new ArgumentOutOfRangeException(nameof(type), "이미지 Gump 실제TYPE 범위 오류");
    /// <summary>현재SHP 토큰과TYPE 기준점 값을 읽는다.</summary>
    public ImageGumpType Type(uint type) { var b = Binding(type); return new((uint)b.Block + 1, b.HotspotX, b.HotspotY); }
    /// <summary>논리수×추가레이어 관문 뒤 현재 물리 헤더의signed WORD를 읽는다.</summary>
    public ImageGumpHeader Header(uint type, uint frame) { var b = Binding(type); ulong logical = (ulong)b.FrameCount * ((b.Flags & 0x440000) != 0 ? 2U : 1U); if (frame >= logical) throw new ArgumentOutOfRangeException(nameof(frame), "이미지 Gump 실제frameCheck 범위 오류"); var (database, block) = Shape(type, frame); var h = database.SquidMetrics(block, (int)frame); return new(h.Width, h.Height, h.HotspotX, h.HotspotY); }
    /// <summary>출력/적중은 현재 물리 프레임만 확인한다.생성의논리수 검사와 구별한다.</summary>
    public (ShapeDatabase Database, int Block) Shape(uint type, uint frame) { var b = Binding(type); if (frame >= _shapes.Blocks()[b.Block].Frames.Count) throw new ArgumentOutOfRangeException(nameof(frame), "이미지 Gump 실제SHP 프레임 오류"); return (_shapes, b.Block); }
    /// <summary>같은 실제TYPE/헤더를 생성과매출력에 연결한다.</summary>
    public static void BindImageGumpAssets(ImageGumpHooks hooks, ImageGumpAssets assets) { ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(assets); hooks.Type = assets.Type; hooks.Header = assets.Header; }
}
/// <summary>원문 일반/직교배율 VFX와200×200 투명 적중을 같은 실제표면/자산/현재색표에 연결한다.</summary>
public sealed class ImageGumpPixelTarget
{
    /// <summary>원본 적중 임시 표면 한 변의 길이.</summary>
    private const int HitSize = 200;
    /// <summary>같은 실제UI 표면과현재클립.</summary>
    private readonly GumpPixelTarget _target;
    /// <summary>현재 실제TYPE/SHP 대응.</summary>
    private readonly ImageGumpAssets _assets;
    /// <summary>다른Gump와 공유하는 현재256색표.</summary>
    private readonly byte[] _currentColors;
    /// <summary>원본 토큰을 해석하는 실제256색표 조회.</summary>
    private readonly Func<uint, byte[]> _colors;
    /// <summary>같은 실제 물리 프레임의해제캐시.</summary>
    private readonly Dictionary<(ShapeDatabase, int, uint), IndexedImage> _images = [];
    /// <summary>실제장치를 빌린다.미연결/미복원 색표를 항등표로 바꾸지 않는다.</summary>
    public ImageGumpPixelTarget(GumpPixelTarget target, ImageGumpAssets assets, byte[] currentColors, Func<uint, byte[]> colors)
    { ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(assets); ArgumentNullException.ThrowIfNull(currentColors); ArgumentNullException.ThrowIfNull(colors); if (currentColors.Length != 256) throw new ArgumentException("이미지 Gump 현재256색표 필요"); _target = target; _assets = assets; _currentColors = currentColors; _colors = colors; }
    /// <summary>현재 실제 그림만 한 번 해제하고 같은 캐시를 빌린다.</summary>
    private IndexedImage Image(uint type, uint frame) { var (database, block) = _assets.Shape(type, frame); var key = (database, block, frame); if (!_images.TryGetValue(key, out var image)) { image = database.Decode(block, (int)frame); _images.Add(key, image); } return image; }
    /// <summary>노드/부착/자산 경계를 보존하고 실제 색/그림/적중만 연결한다.</summary>
    public ImageGumpHooks Hooks(ImageGumpHooks? hooks = null) { hooks ??= new(); hooks.Color = token => { byte[] colors = _colors(token); if (colors.Length != 256) throw new ArgumentException("이미지 Gump 실제256색표 필요"); colors.CopyTo(_currentColors, 0); }; hooks.Draw = Draw; hooks.Hit = Hit; return hooks; }
    /// <summary>특수 프레임은 출력하지 않는다.일반은실제상대RECT,배율은실제VFX커널을 사용한다.</summary>
    private void Draw(ImageGumpDrawRequest request) { var (database, block) = _assets.Shape(request.Type, request.Frame); var frame = database.Blocks()[block].Frames[(int)request.Frame]; if (frame.IsSpecial()) return; var image = Image(request.Type, request.Frame); byte[]? colors = request.Mapped ? _currentColors : null; if (request.Scaled) _target.ScaledImage(image, frame, request.Position, request.Scale, request.Scale, colors); else _target.Image(image, new(unchecked(request.Position.X + frame.Rect.Left), unchecked(request.Position.Y + frame.Rect.Top)), colors); }
    /// <summary>원본4daca0의 기준점clamp/검사점 보정·raw선형index와255 판정을 유지한다.</summary>
    private uint Hit(ImageGumpHitRequest request)
    {
        var (database, block) = _assets.Shape(request.Type, request.Frame); var frame = database.Blocks()[block].Frames[(int)request.Frame]; var point = request.Point; var origin = request.Origin; if (origin.X < 0) point = point with { X = unchecked(point.X - origin.X) }; if (origin.X > 199) point = point with { X = unchecked(point.X + 199 - origin.X) }; if (origin.Y < 0) point = point with { Y = unchecked(point.Y - origin.Y) }; if (origin.Y > 199) point = point with { Y = unchecked(point.Y + 199 - origin.Y) }; long index = (long)point.X + (long)point.Y * HitSize; if (index < 0 || index >= HitSize * HitSize) throw new ArgumentOutOfRangeException(nameof(request), "이미지 Gump 원본 적중 버퍼 밖 읽기"); Span<byte> scratch = stackalloc byte[HitSize * HitSize]; scratch.Fill(255); if (!frame.IsSpecial()) VfxTransform.DrawScaledVfxImage(scratch, HitSize, HitSize, Image(request.Type, request.Frame), frame, new(Math.Clamp(origin.X, 0, 199), Math.Clamp(origin.Y, 0, 199)), request.Scale, request.Scale, new(0, 0, HitSize, HitSize)); return scratch[(int)index] != 255 ? 1U : 0U;
    }
}
