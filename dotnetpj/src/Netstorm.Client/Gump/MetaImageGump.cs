using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 자산 TYPE 아이콘과 별도 수명의 동적 Image를 표시하는 원본 Gump 영역이다.

/// <summary>원본+a0/a4.동적 Image는 목록/창이 소유하고 이 노드는 같은 객체만 빌린다.</summary>
public sealed class MetaImageGumpNode : GumpCreateNode
{
    /// <summary>현재 실제 동적 Image.노드의 소멸이 Image를 반납하지 않는다.</summary>
    public RawImage? Image { get; set; }
    /// <summary>비0이면 정확한 단위배율에서만 비활성 색표를 설치한다.</summary>
    public uint ColorMode { get; set; }
}
/// <summary>현재 동적1.10 shape의 frame0/angle0 출력 요청.</summary>
/// <param name="Image">출력 동안만 빌리는 현재 Image.</param><param name="Position">Image 폭/높이를 더한 기준점.</param><param name="Scale">두 축의signed16.16.</param><param name="Scaled">확대/축소 분기.</param><param name="Mapped">현재256색표 분기.</param>
public readonly record struct MetaImageDrawRequest(RawImage Image, ScreenPoint Position, int Scale, bool Scaled, bool Mapped);
/// <summary>같은 실제 공통 저장소/부착/위치와 현재 픽셀·색표 장치.</summary>
public sealed class MetaImageGumpHooks
{
    /// <summary>안정된 같은 파생/기본 노드.</summary>
    public Func<uint, MetaImageGumpNode?>? Node { get; set; }
    /// <summary>현재 기본 Attach.기하 처리보다 먼저 호출한다.</summary>
    public Action<uint, float, float, uint, uint>? Attach { get; set; }
    /// <summary>같은 공통 화면 위치 getter.</summary>
    public Func<uint, ScreenPoint>? Position { get; set; }
    /// <summary>실제 고정 비활성 색표 설치.</summary>
    public Action<uint>? Color { get; set; }
    /// <summary>현재 실제 shape와 같은 클립/표면의 출력.</summary>
    public Action<MetaImageDrawRequest>? Draw { get; set; }
}
/// <summary>cpppj/src/client/MetaImageGump.cpp의 생성/기하·float저장/reciprocal·현재 Image 재조회/가상 출력.</summary>
public sealed class MetaImageGump
{
    /// <summary>원본10.78의 가상 표.</summary>
    private const uint MetaVtable = 0x50d290;
    /// <summary>기본 command에 넣는 원본 종류BYTE.</summary>
    private const uint MetaKind = 21;
    /// <summary>원본 고정 비활성 색표 주소.</summary>
    private const uint DisabledColors = 0x556038;
    /// <summary>같은 실제 기본 생성/등록.</summary>
    private readonly GumpCreate _factory;
    /// <summary>현재 공유 노드/장치 경계.</summary>
    private readonly MetaImageGumpHooks _hooks;
    /// <summary>실제 도달한 하위 효과만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("MetaImage 실제 하위 효과 미연결");
    /// <summary>10.78의 같은 공통 생성기만 사용한다.창/장치를 생성하지 않는다.</summary>
    public MetaImageGump(GumpCreateState create, GumpCreate factory, MetaImageGumpHooks hooks) { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(hooks); if (create.Collection.Edition != OriginalEdition.Patch1078) throw new ArgumentException("MetaImage 실제10.78 판본 필요"); _factory = factory; _hooks = hooks; }
    /// <summary>현재 같은 파생 노드를 조회하고 없는 노드는 거부한다.</summary>
    private MetaImageGumpNode Node(uint token) => (token != 0 ? Require(_hooks.Node)(token) : null) ?? throw new ArgumentOutOfRangeException(nameof(token), "MetaImage 실제 파생 저장소 없음");
    /// <summary>생성은 Image를 아직 측정하지 않는다.기본 크기0/배율1과 빌린 주소만 쓴다.</summary>
    public uint Construct(uint token, RawImage image) { ArgumentNullException.ThrowIfNull(image); var n = Node(token); _factory.Construct(token, 0, 0, 0, 1); n.Vtable = MetaVtable; n.Command = MetaKind; n.Image = image; n.Scale = 1; n.Width = n.BaseWidth; n.Height = n.BaseHeight; n.ColorMode = 0; return token; }
    /// <summary>int+1 감김→double→기본 크기float저장과 원래double×배율 결과를 구별한다.</summary>
    public void Resize(uint token) { var n = Node(token); if (n.Image is null) throw new InvalidOperationException("MetaImage 현재 Image 없음"); double width = unchecked(n.Image.Width() + 1), height = unchecked(n.Image.Height() + 1); n.BaseWidth = (float)width; n.Width = (float)(width * n.Scale); n.BaseHeight = (float)height; n.Height = (float)(height * n.Scale); }
    /// <summary>기본 부착 후 현재 Image를 읽는다.두비율만float으로 저장하고 reciprocal은 최종크기 곱셈까지double로 유지한다.</summary>
    public void Attach(uint token, float x, float y, uint parent, uint mode)
    {
        Require(_hooks.Attach)(token, x, y, parent, mode); var n = Node(token); if (n.Image is null) throw new InvalidOperationException("MetaImage 부착 후 Image 없음"); double scale = 1;
        if (n.Image.Width() != 0 && n.Image.Height() != 0) { float width = (float)((double)n.Image.Width() / n.BaseWidth), height = (float)((double)n.Image.Height() / n.BaseHeight); if (width > 1 || height > 1) scale = 1.0 / (height < width ? width : height); } n.Scale = (float)scale; n.Width = (float)(scale * n.BaseWidth); n.Height = (float)(scale * n.BaseHeight);
    }
    /// <summary>빈shape는 위치 조회 전 반환한다.정확한 단위배율만mapped,다른 배율은plain 변환이다.</summary>
    public void Draw(uint token)
    {
        var n = Node(token); if (n.Image is null) throw new InvalidOperationException("MetaImage 출력 Image 없음"); if (n.Image.Shape().IsEmpty) return; var point = Require(_hooks.Position)(token); bool unit = BitConverter.SingleToUInt32Bits(n.Scale) == 0x3f800000;
        if (unit) { if (n.ColorMode != 0) Require(_hooks.Color)(DisabledColors); var image = n.Image ?? throw new InvalidOperationException("MetaImage 색표 설치 후 Image 없음"); Require(_hooks.Draw)(new(image, new(unchecked(point.X + image.Width()), unchecked(point.Y + image.Height())), 65536, false, n.ColorMode != 0)); return; }
        int scale = GumpCollection.Ftol((double)n.Scale * 65536), y = GumpCollection.Ftol((double)n.Image.Height() * n.Scale + point.Y), x = GumpCollection.Ftol((double)n.Image.Width() * n.Scale + point.X); Require(_hooks.Draw)(new(n.Image, new(x, y), scale, true, false));
    }
    /// <summary>현재동적shape를매출력에서읽고공통클립/256색표에연결한다.이미지캐시로현재내용을고정하지않는다.</summary>
    public static MetaImageGumpHooks BindMetaImagePixels(MetaImageGumpHooks hooks, GumpPixelTarget target, byte[] current, byte[] disabled)
    {
        ArgumentNullException.ThrowIfNull(hooks); ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(current); ArgumentNullException.ThrowIfNull(disabled); if (current.Length != 256 || disabled.Length != 256) throw new ArgumentException("MetaImage 실제256색표 필요"); hooks.Color = token => { if (token != DisabledColors) throw new ArgumentOutOfRangeException(nameof(token), "MetaImage 고정 색표 주소 오류"); disabled.CopyTo(current, 0); };
        hooks.Draw = request => { if (request.Image is null) throw new InvalidOperationException("MetaImage 실제 픽셀 Image 없음"); var raw = request.Image.Shape(); if (raw.IsEmpty) return; var database = new ShapeDatabase(raw.ToArray()); var frame = database.Blocks()[0].Frames[0]; if (frame.IsSpecial()) return; var image = database.Decode(0, 0); byte[]? colors = request.Mapped ? current : null; if (request.Scaled) target.ScaledImage(image, frame, request.Position, request.Scale, request.Scale, colors); else target.Image(image, new(unchecked(request.Position.X + frame.Rect.Left), unchecked(request.Position.Y + frame.Rect.Top)), colors); }; return hooks;
    }
    /// <summary>현재MetaImage만실제가상Attach/Draw로분배한다.소멸은Image를소유하지않는기본경로를유지한다.</summary>
    public static void BindMetaImageGump(GumpTreeHooks tree, GumpDrawHooks draw, Func<uint, MetaImageGump?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var attach = tree.Attach; tree.Attach = (t, x, y, p, m) => { if (resolve(t) is { } image) image.Attach(t, x, y, p, m); else Require(attach)(t, x, y, p, m); }; var render = draw.Draw; draw.Draw = t => { if (resolve(t) is { } image) image.Draw(t); else Require(render)(t); }; }
}
