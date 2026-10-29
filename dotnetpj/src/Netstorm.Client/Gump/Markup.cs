using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 스타일 체인의 그림 조각과 실제 SHP 출력 영역이다.

/// <summary>원본 그림 조각의 파생 필드. 공통 next/태그는 StyleText 표에 있다.</summary>
public sealed class MarkupPicture
{
    /// <summary>현재 타입 번호.</summary>
    public uint Type { get; set; }
    /// <summary>현재 프레임 번호.</summary>
    public uint Frame { get; set; }
    /// <summary>원본 float32 배율.</summary>
    public float Scale { get; set; } = 1;
    /// <summary>원본 색 치환 BYTE.</summary>
    public byte Mapped { get; set; }
}

/// <summary>현재 타입 수와 공통 토큰을 공유하는 그림 저장소.</summary>
public sealed class MarkupState
{
    /// <summary>원본 signed 타입 수.</summary>
    public int TypeCount { get; set; }
    /// <summary>삽입으로 기존 참조가 바뀌지 않는 파생 저장소.</summary>
    public SortedDictionary<uint, MarkupPicture> Pictures { get; } = [];
}

/// <summary>현재 자산 타입의 안정된 주소와 기준점.</summary>
public sealed class MarkupType
{
    /// <summary>타입 조회 토큰.</summary>
    public uint Token { get; set; }
    /// <summary>현재 SHP 식별자.</summary>
    public uint Shape { get; set; }
    /// <summary>논리 프레임 수.</summary>
    public int FrameCount { get; set; }
    /// <summary>타입의 signed X 기준점.</summary>
    public int HotspotX { get; set; }
    /// <summary>타입의 signed Y 기준점.</summary>
    public int HotspotY { get; set; }
}

/// <summary>원본 메타 헤더의 마지막 여섯 signed WORD.</summary>
/// <param name="Width">폭.</param><param name="Height">높이.</param><param name="HotspotX">X 기준점.</param><param name="HotspotY">Y 기준점.</param><param name="ExtraX">추가 X.</param><param name="ExtraY">추가 Y.</param>
public readonly record struct MarkupFrame(short Width, short Height, short HotspotX, short HotspotY, short ExtraX, short ExtraY);
/// <summary>현재 조회가 반환한 타입/SHP/보정 헤더의 값 복사.</summary>
/// <param name="Type">타입 번호.</param><param name="Shape">SHP 번호.</param><param name="Frame">현재 보정 헤더.</param>
public readonly record struct MarkupFrameView(uint Type, uint Shape, MarkupFrame Frame);
/// <summary>원본 VFX 일반/16.16 배율 출력 요청.</summary>
/// <param name="Type">타입 번호.</param><param name="Shape">SHP 번호.</param><param name="Frame">현재 물리 프레임.</param><param name="Position">출력 기준점.</param><param name="Mapped">현재 색표 사용.</param><param name="Scaled">배율 출력 여부.</param><param name="Angle">회전 각도.</param><param name="ScaleX">X 배율.</param><param name="ScaleY">Y 배율.</param>
public readonly record struct MarkupShapeRequest(uint Type, uint Shape, uint Frame, ScreenPoint Position, bool Mapped, bool Scaled, int Angle, int ScaleX, int ScaleY);

/// <summary>현재 타입/헤더와 색표 설치/실제 그림 출력 경계.</summary>
public sealed class MarkupHooks
{
    /// <summary>현재 타입의 안정된 객체. 무자산이면 null.</summary>
    public Func<uint, MarkupType?>? Type { get; set; }
    /// <summary>타입/abstract/논리 프레임의 현재 헤더.</summary>
    public Func<uint, uint, uint, MarkupFrame>? Frame { get; set; }
    /// <summary>현재 공유 VFX 색표에 그림 치환표를 설치한다.</summary>
    public Action? Color { get; set; }
    /// <summary>현재 실제 그림을 출력한다.</summary>
    public Action<MarkupShapeRequest>? Shape { get; set; }
}

/// <summary>cpppj/src/client/Markup.cpp의 공통 체인·그림 수명·WORD 감김·판본별 배율과 현재 필드 조회를 옮긴다.</summary>
public sealed class Markup
{
    /// <summary>공통 조각 소유 표.</summary>
    private readonly StyleTextState _fragments;
    /// <summary>같은 토큰의 그림 필드.</summary>
    private readonly MarkupState _state;
    /// <summary>현재 자산/출력 경계.</summary>
    private readonly MarkupHooks _hooks;
    /// <summary>배율 계산의 중간 저장을 고르는 판본.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>실제로 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("Markup 자산/출력 경계 필요");
    /// <summary>현재 공통 저장소의 살아 있는 조각.</summary>
    private StyleTextFragment Fragment(uint token) => token != 0 && _fragments.Fragments.TryGetValue(token, out var n) ? n : throw new ArgumentOutOfRangeException(nameof(token), "Markup 공통 조각 없음");
    /// <summary>소멸 전 External 분류인 그림만 직접 처리한다.</summary>
    private MarkupPicture Picture(uint token) => _state.Pictures.TryGetValue(token, out var n) && Fragment(token).Kind == StyleTextFragmentKind.External ? n : throw new ArgumentOutOfRangeException(nameof(token), "Markup 파생 조각 없음");
    /// <summary>같은 공통/파생 저장소와 현재 자산 경계를 빌린다.</summary>
    public Markup(StyleTextState fragments, MarkupState state, MarkupHooks hooks, OriginalEdition edition = OriginalEdition.Patch1078)
    { ArgumentNullException.ThrowIfNull(fragments); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks); _fragments = fragments; _state = state; _hooks = hooks; _edition = edition; }
    /// <summary>새 그림은 scale1/mapped0이며 등록 실패에서는 새 공통 조각만 정리한다.</summary>
    public uint NewPicture(uint type, uint frame, uint metadata = 0)
    {
        uint token = StyleText.RegisterStyleTextFragment(_fragments, new() { Kind = StyleTextFragmentKind.External, Metadata = metadata });
        try { _state.Pictures.Add(token, new() { Type = type, Frame = frame }); } catch { _fragments.Fragments.Remove(token); throw; } return token;
    }
    /// <summary>타입/프레임/태그만 복사하며 배율과 색 치환은 초기화한다.</summary>
    public uint ClonePicture(uint token) { var p = Picture(token); return NewPicture(p.Type, p.Frame, Fragment(token).Metadata); }
    /// <summary>필드와 링크를 남기고 기본 가상 분류로 전환한다.</summary>
    public void Destruct(uint token) { Picture(token); Fragment(token).Kind = StyleTextFragmentKind.Base; }
    /// <summary>공통 scalar 반납 직전에 파생 저장소만 제거한다.</summary>
    public void Release(uint token) => _state.Pictures.Remove(token);
    /// <summary>기본 분류로 바뀐 뒤에도 scalar 반납 전까지 소속을 유지한다.</summary>
    public bool Contains(uint token) => _state.Pictures.ContainsKey(token);
    /// <summary>bool 정규화 없이 하위 BYTE를 저장한다.</summary>
    public void SetMapped(uint token, uint mapped) => Picture(token).Mapped = unchecked((byte)mapped);
    /// <summary>현재 signed 타입/프레임을 검사하고 타입 기준점을 signed WORD 감김으로 뺀다.</summary>
    public MarkupFrameView Lookup(uint token)
    {
        int type = unchecked((int)Picture(token).Type); if (type <= 0 || type >= _state.TypeCount) return default; MarkupType? view = Require(_hooks.Type)((uint)type); if (view is null || view.Shape == 0) return default;
        int frame = unchecked((int)Picture(token).Frame); if (frame < 0 || frame >= view.FrameCount) return default; MarkupFrame f = Require(_hooks.Frame)(view.Token, 0, (uint)frame);
        f = f with { HotspotX = unchecked((short)(f.HotspotX - view.HotspotX)), HotspotY = unchecked((short)(f.HotspotY - view.HotspotY)) }; return new((uint)type, view.Shape, f);
    }
    /// <summary>10.78의 세로 float32 저장과 CD의 NaN 분기를 유지하며 1 미만 배율만 쓴다.</summary>
    public void Fit(uint token, int width, int height)
    {
        MarkupFrame f = Lookup(token).Frame; double wr = (double)width / f.Width, hr = (double)height / f.Height;
        if (_edition == OriginalEdition.Patch1078) { float stored = (float)hr; double selected = wr < stored ? wr : stored; if (selected < 1) Picture(token).Scale = (float)selected; }
        else { double selected = wr > hr ? hr : wr; if (!(selected >= 1)) Picture(token).Scale = (float)selected; }
    }
    /// <summary>현재 보정 높이/Y 기준점에 CRT 절삭 배율을 적용한다.</summary>
    public (int Height, int Ascent) Baseline(uint token)
    { MarkupFrame f = Lookup(token).Frame; float s = Picture(token).Scale; return s == 1 ? (f.Height, f.HotspotY) : (GumpCollection.Ftol((double)f.Height * s), GumpCollection.Ftol((double)f.HotspotY * s)); }
    /// <summary>현재 폭/높이를 측정하고 CD의 폭 재확인까지 유지한다.</summary>
    public (int Width, int Height) Measure(uint token)
    {
        MarkupFrame f = Lookup(token).Frame; float s = Picture(token).Scale; if (s == 1) return (f.Width, f.Height); var result = (Width: GumpCollection.Ftol((double)f.Width * s), Height: GumpCollection.Ftol((double)f.Height * s));
        if (_edition == OriginalEdition.Cd1072 && GumpCollection.Ftol((double)f.Width * Picture(token).Scale) != result.Width) throw new InvalidOperationException("Markup CD 폭 재확인 오류"); return result;
    }
    /// <summary>색 설치 이전의 기준점/배율/SHP는 저장하되 설치 뒤 현재 프레임을 다시 읽는다.</summary>
    public void Draw(uint token, ScreenPoint position)
    {
        MarkupFrameView v = Lookup(token); if (v.Shape == 0) return; float s = Picture(token).Scale; bool scaled = s != 1;
        ScreenPoint p = scaled ? new(GumpCollection.Ftol((double)v.Frame.HotspotX * s + position.X), GumpCollection.Ftol((double)v.Frame.HotspotY * s + position.Y)) : new(unchecked(position.X + v.Frame.HotspotX), unchecked(position.Y + v.Frame.HotspotY));
        int scale = scaled ? GumpCollection.Ftol((double)s * 65536) : 0; bool mapped = Picture(token).Mapped != 0; if (mapped) Require(_hooks.Color)(); Require(_hooks.Shape)(new(v.Type, v.Shape, Picture(token).Frame, p, mapped, scaled, 0, scale, scale));
    }
    /// <summary>같은 그림의 가상 동작/반납만 연결하고 기존 다른 파생 분배를 보존한다.</summary>
    public static void BindMarkup(StyleTextHooks hooks, Markup markup)
    {
        var metrics = hooks.ExternalMetrics; var measure = hooks.ExternalMeasure; var draw = hooks.ExternalDraw; var destroy = hooks.ExternalDestroy; var release = hooks.Release;
        hooks.ExternalMetrics = t => markup.Contains(t) ? markup.Baseline(t) : Require(metrics)(t); hooks.ExternalMeasure = t => markup.Contains(t) ? markup.Measure(t) : Require(measure)(t);
        hooks.ExternalDraw = (t, p, c) => { if (markup.Contains(t)) markup.Draw(t, p); else Require(draw)(t, p, c); }; hooks.ExternalDestroy = (t, f) => { if (markup.Contains(t)) markup.Destruct(t); else Require(destroy)(t, f); };
        hooks.Release = t => { release?.Invoke(t); markup.Release(t); };
    }
}

/// <summary>전체 GameAssets 조립 이전에도 실제 타입/SHP/논리 수/기준점을 명시적으로 연결한다.</summary>
/// <param name="Type">타입 번호.</param><param name="Block">실제 블록.</param><param name="FrameCount">논리 수.</param><param name="HotspotX">타입 X 기준점.</param><param name="HotspotY">타입 Y 기준점.</param>
public readonly record struct MarkupAssetBinding(uint Type, int Block, int FrameCount, int HotspotX, int HotspotY);
/// <summary>실제 SHP의 논리 메타와 물리 출력 범위를 구별하는 읽기 전용 어댑터. 전체 GameAssets 소유 연결은 후속이다.</summary>
public sealed class MarkupAssets
{
    /// <summary>메타와 픽셀의 같은 DB.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>안정된 타입 객체와 물리 블록 대응.</summary>
    private readonly Dictionary<uint, (MarkupType Type, int Block)> _views = [];
    /// <summary>내장 타입까지 포함한 원본 타입 수.</summary>
    public int TypeCount { get; }
    /// <summary>실제 DB/전체 타입 수와 명시 연결을 검증한다. 물리 수로 논리 수를 대체하지 않는다.</summary>
    public MarkupAssets(ShapeDatabase shapes, int typeCount, IEnumerable<MarkupAssetBinding> bindings)
    {
        ArgumentNullException.ThrowIfNull(shapes); ArgumentNullException.ThrowIfNull(bindings); if (typeCount < 0) throw new ArgumentOutOfRangeException(nameof(typeCount)); _shapes = shapes; TypeCount = typeCount;
        // 로딩 순서의 타입/블록 대응을 안정된 객체에 보관한다.
        foreach (var b in bindings) { if (b.Type == 0 || b.Type >= typeCount || b.Block < 0 || b.Block >= shapes.Blocks().Count || b.FrameCount < 0 || b.FrameCount > shapes.Blocks()[b.Block].Frames.Count) throw new ArgumentException("Markup 자산 연결 범위 오류"); _views.Add(b.Type, (new() { Token = b.Type, Shape = b.Type, FrameCount = b.FrameCount, HotspotX = b.HotspotX, HotspotY = b.HotspotY }, b.Block)); }
    }
    /// <summary>무자산/내장 타입이면 null, 그 외에는 같은 객체다.</summary>
    public MarkupType? Type(uint token) => _views.TryGetValue(token, out var v) ? v.Type : null;
    /// <summary>abstract0의 논리 프레임에서 원본 여섯 signed WORD를 읽는다.</summary>
    public MarkupFrame Frame(uint type, uint abstractLayer, uint frame)
    {
        if (abstractLayer != 0 || Type(type) is not { } v || frame >= v.FrameCount) throw new ArgumentOutOfRangeException(nameof(frame), "Markup 논리 프레임 오류"); byte[] h = _shapes.SquidHeader(_views[type].Block, (int)frame);
        return new(BinaryPrimitives.ReadInt16LittleEndian(h.AsSpan(0x18)), BinaryPrimitives.ReadInt16LittleEndian(h.AsSpan(0x1a)), BinaryPrimitives.ReadInt16LittleEndian(h.AsSpan(0x1c)), BinaryPrimitives.ReadInt16LittleEndian(h.AsSpan(0x1e)), BinaryPrimitives.ReadInt16LittleEndian(h.AsSpan(0x20)), BinaryPrimitives.ReadInt16LittleEndian(h.AsSpan(0x22)));
    }
    /// <summary>출력은 논리 수와 별개인 실제 물리 프레임 범위를 검사한다.</summary>
    public (ShapeDatabase Database, int Block) Shape(uint shape, uint frame) => _views.TryGetValue(shape, out var v) && frame < _shapes.Blocks()[v.Block].Frames.Count ? (_shapes, v.Block) : throw new ArgumentOutOfRangeException(nameof(frame), "Markup 물리 프레임 오류");
    /// <summary>같은 타입 수/현재 타입/논리 헤더 조회를 연결한다.</summary>
    public static void BindMarkupAssets(MarkupState state, MarkupHooks hooks, MarkupAssets assets) { state.TypeCount = assets.TypeCount; hooks.Type = assets.Type; hooks.Frame = assets.Frame; }
}

/// <summary>원본 그림의 색표/투명/직교 배율을 같은 실제 8비트 표면으로 출력한다.</summary>
public sealed class MarkupPixelTarget
{
    /// <summary>현재 공유 클립의 표면.</summary>
    private readonly GumpPixelTarget _target;
    /// <summary>실제 SHP 자산.</summary>
    private readonly MarkupAssets _assets;
    /// <summary>현재 공유 VFX 색표.</summary>
    private readonly byte[] _colors;
    /// <summary>설치할 그림 색표.</summary>
    private readonly byte[] _pictureColors;
    /// <summary>실제 물리 프레임 해제 캐시.</summary>
    private readonly Dictionary<(ShapeDatabase, int, uint), IndexedImage> _images = [];
    /// <summary>색표의 공유 참조를 유지하며 모든 출력을 같은 표면으로 연결한다.</summary>
    public MarkupPixelTarget(GumpPixelTarget target, MarkupAssets assets, byte[] currentColors, byte[] pictureColors)
    { ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(assets); if (currentColors.Length != 256 || pictureColors.Length != 256) throw new ArgumentException("Markup 256색 변환표 필요"); _target = target; _assets = assets; _colors = currentColors; _pictureColors = pictureColors; }
    /// <summary>각도0 출력은 실제 픽셀로, 회전 요청은 기존 분배로 연결한다.</summary>
    public MarkupHooks Hooks(MarkupHooks? hooks = null)
    {
        hooks ??= new(); var rotated = hooks.Shape; hooks.Color = () => _pictureColors.CopyTo(_colors, 0); hooks.Shape = r => { if (!r.Scaled || r.Angle == 0) Image(r); else if (rotated is not null) rotated(r); else throw new ArgumentException("Markup 회전 커널 경계 필요"); }; return hooks;
    }
    /// <summary>물리 프레임을 재조회하고 special을 건너뛰며 일반/배율 픽셀을 실제 출력한다.</summary>
    private void Image(MarkupShapeRequest r)
    {
        var (database, block) = _assets.Shape(r.Shape, r.Frame); var frame = database.Blocks()[block].Frames[(int)r.Frame]; if (frame.IsSpecial()) return; var key = (database, block, r.Frame); if (!_images.TryGetValue(key, out var image)) { image = database.Decode(block, (int)r.Frame); _images.Add(key, image); }
        byte[]? colors = r.Mapped ? _colors : null; if (r.Scaled) _target.ScaledImage(image, frame, r.Position, r.ScaleX, r.ScaleY, colors); else _target.Image(image, new(unchecked(r.Position.X + frame.Rect.Left), unchecked(r.Position.Y + frame.Rect.Top)), colors);
    }
}
