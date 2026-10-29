using Netstorm.Client.Display;
using Netstorm.Core.Original;

namespace Netstorm.Client.Gump; // 메뉴/대화창 장식의 배경·모서리·변경 영역과 실제 픽셀 영역이다.

/// <summary>배경과 별개로 원본+b8/+bc/+c0의 캐시 SHP/모서리 타입/첫 프레임을 보관한다. HTML 창도 같은 기본 저장소를 상속한다.</summary>
public class GumpDecorationNode : GumpBackgroundNode
{
    /// <summary>생성 시 캐시한 SHP 식별자.</summary>
    public uint CornerShape { get; set; }
    /// <summary>현재 모서리 타입 번호.</summary>
    public uint CornerType { get; set; }
    /// <summary>I 글자 구간의 첫 물리 프레임 비트.</summary>
    public uint CornerFrame { get; set; }
}
/// <summary>실제 게임 초기화가 공급하는 장식 자산/종류 BYTE 전역.</summary>
public sealed class GumpDecorationState
{
    /// <summary>기본 command에 signed 확장할 원본 BYTE.</summary>
    public sbyte Alignment { get; set; } = 57;
    /// <summary>현재 배경 타입.</summary>
    public uint BackgroundType { get; set; }
    /// <summary>현재 배경 프레임.</summary>
    public uint BackgroundFrame { get; set; }
    /// <summary>현재 모서리 타입.</summary>
    public uint CornerType { get; set; }
}
/// <summary>현재 타입의 SHP와 I 첫 프레임을 값으로 돌려준다.</summary>
/// <param name="Shape">캐시할 SHP 토큰.</param><param name="FirstFrame">I 구간 첫 프레임.</param>
public readonly record struct GumpDecorationType(uint Shape, uint FirstFrame);
/// <summary>같은 실제 장식/기본 노드와 현재 타입·원색 출력·half-open 변경 영역.</summary>
public sealed class GumpDecorationHooks
{
    /// <summary>현재 실제 장식.</summary>
    public Func<uint, GumpDecorationNode?>? Node { get; set; }
    /// <summary>같은 기본/부모.</summary>
    public Func<uint, GumpCreateNode?>? Base { get; set; }
    /// <summary>현재 모서리 타입 필드를 값으로 읽는다.</summary>
    public Func<uint, GumpDecorationType>? Type { get; set; }
    /// <summary>현재 캐시 SHP/물리 프레임의 원색 출력.</summary>
    public Action<uint, uint, ScreenPoint>? Shape { get; set; }
    /// <summary>끝 제외 변경 영역 요청.</summary>
    public Action<ScreenRect>? Invalidate { get; set; }
}
/// <summary>cpppj/src/client/GumpDecoration.cpp의 배경 뒤 현재 모서리 조회·visible 부모 검사·기본 수명.</summary>
public sealed class GumpDecoration
{
    /// <summary>현재 생성/판본 전역.</summary>
    private readonly GumpCreateState _create;
    /// <summary>현재 종류/자산 전역.</summary>
    private readonly GumpDecorationState _state;
    /// <summary>같은 실제 배경 생성기.</summary>
    private readonly GumpBackgroundCreate _factory;
    /// <summary>같은 실제 배경 출력.</summary>
    private readonly GumpBackgroundDraw _background;
    /// <summary>같은 실제 위치/경계.</summary>
    private readonly GumpCollection _collection;
    /// <summary>같은 실제 기본 scalar 수명.</summary>
    private readonly GumpLifetime _lifetime;
    /// <summary>현재 공유 저장/픽셀/변경 영역 경계.</summary>
    private readonly GumpDecorationHooks _hooks;
    /// <summary>실제 도달한 경계만 요구한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("장식 배경 경계 필요");
    /// <summary>기존 실제 처리기와 같은 기본/파생 저장소를 빌린다.</summary>
    public GumpDecoration(GumpCreateState create, GumpDecorationState state, GumpBackgroundCreate factory, GumpBackgroundDraw background, GumpCollection collection, GumpLifetime lifetime, GumpDecorationHooks hooks)
    { ArgumentNullException.ThrowIfNull(create); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(background); ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(lifetime); ArgumentNullException.ThrowIfNull(hooks); Require(hooks.Node); Require(hooks.Base); _create = create; _state = state; _factory = factory; _background = background; _collection = collection; _lifetime = lifetime; _hooks = hooks; }
    /// <summary>배경/기본과 같은 실제 객체인지 확인한다.</summary>
    private GumpDecorationNode Node(uint token) { var n = token != 0 ? _hooks.Node!(token) : null; if (n is null || !ReferenceEquals(n, _hooks.Base!(token))) throw new InvalidOperationException("장식 배경 공유 노드 없음"); return n; }
    /// <summary>현재 부모 기본 필드.</summary>
    private GumpCreateNode Base(uint token) => (token != 0 ? _hooks.Base!(token) : null) ?? throw new InvalidOperationException("장식 배경 부모 없음");
    /// <summary>실제 배경 생성 뒤 종류/모서리 타입/현재 SHP와 첫 I 프레임을 캐시한다.</summary>
    public uint Construct(uint token, float width, float height)
    { Node(token); _factory.Construct(token, width, height, 0x03e00000, 1, _state.BackgroundType, _state.BackgroundFrame); Node(token).Vtable = _create.Collection.Edition == OriginalEdition.Patch1078 ? 0x50ae7cU : 0x504928U; Node(token).Command = unchecked((uint)(int)_state.Alignment); Node(token).CornerType = _state.CornerType; var type = Require(_hooks.Type)(_state.CornerType); Node(token).CornerShape = type.Shape; Node(token).CornerFrame = type.FirstFrame; return token; }
    /// <summary>출력할 때마다 현재 캐시 SHP/첫 프레임을 읽는다.위치는 Draw가 저장한 값이다.</summary>
    private void Corner(uint token, uint offset, ScreenPoint position) { var shape = Require(_hooks.Shape); var n = Node(token); shape(n.CornerShape, unchecked(n.CornerFrame + offset), position); }
    /// <summary>실제 배경→저장 Position→TL/TR/BR/BL이며 각 크기 절삭/재조회 시점을 보존한다.</summary>
    public void Draw(uint token)
    { _background.Draw(token); var position = _collection.Position(token); Corner(token, 0, position); Corner(token, 1, new(unchecked(position.X + GumpCollection.Ftol(Node(token).Width)), position.Y)); int bottom = unchecked(position.Y + GumpCollection.Ftol(Node(token).Height)); Corner(token, 2, new(unchecked(position.X + GumpCollection.Ftol(Node(token).Width)), bottom)); Corner(token, 3, new(position.X, unchecked(position.Y + GumpCollection.Ftol(Node(token).Height)))); }
    /// <summary>자신/모든 부모가 visible일 때 원점-1/크기+4의 변경 영역을 요청한다.dead/void/erase는 필터가 아니다.</summary>
    public void Redraw(uint token, bool erase)
    {
        if ((Node(token).Flags & 1) == 0) return; var seen = new HashSet<uint>();
        // 원본 visible 검사와 현재 부모 재조회를 유지하며 정상 링크에 영향 없는 순환만 거부한다.
        for (uint parent = Node(token).Parent; parent != 0; parent = Base(parent).Parent) { if (!seen.Add(parent) || seen.Count > GumpCreateState.RegistryCapacity) throw new InvalidOperationException("장식 배경 부모 순환"); if ((Base(parent).Flags & 1) == 0) return; }
        var position = _collection.Position(token); int height = GumpCollection.Ftol((double)Node(token).Height + 4), width = GumpCollection.Ftol((double)Node(token).Width + 4); if (width <= 0 || height <= 0) return; int left = unchecked(position.X - 1), top = unchecked(position.Y - 1); Require(_hooks.Invalidate)(new(left, top, unchecked(left + width), unchecked(top + height)));
    }
    /// <summary>SHP를 소유하지 않으므로 기본 scalar 소멸/반납만 수행한다.</summary>
    public uint DeleteScalar(uint token, uint flags) { Node(token); return _lifetime.DeleteScalar(token, flags); }
    /// <summary>현재 장식만 Draw/Redraw/scalar로 분배하고 이전 파생 처리기를 보존한다.</summary>
    public static void BindGumpDecoration(GumpDrawHooks draw, GumpFrameHooks frame, GumpLifetimeHooks lifetime, Func<uint, GumpDecoration?> resolve)
    { ArgumentNullException.ThrowIfNull(resolve); var previousDraw = draw.Draw; var previousRedraw = frame.Redraw; var previousDelete = lifetime.DeletingDestructor; draw.Draw = t => { if (resolve(t) is { } d) d.Draw(t); else Require(previousDraw)(t); }; frame.Redraw = (t, e) => { if (resolve(t) is { } d) d.Redraw(t, e); else Require(previousRedraw)(t, e); }; lifetime.DeletingDestructor = (t, f) => { if (resolve(t) is { } d) d.DeleteScalar(t, f); else Require(previousDelete)(t, f); }; }
}
/// <summary>실제 타입/SHP와 I 첫 프레임의 명시 연결.전체 GameAssets 소유 조립은 후속이다.</summary>
/// <param name="Type">원본 타입 번호.</param><param name="Block">SHP 블록.</param><param name="FirstFrame">I 첫 프레임 비트.미지원 값도 출력 때 검증한다.</param>
public readonly record struct GumpDecorationAssetBinding(uint Type, int Block, uint FirstFrame);
/// <summary>장식 캐시를 실제 SHP 물리 프레임에 연결한다.배경의 논리 수/레이어 제한으로 바꾸지 않는다.</summary>
public sealed class GumpDecorationAssets
{
    /// <summary>현재 실제 SHP 데이터베이스.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>타입별 실제 물리 블록/첫 I 프레임.</summary>
    private readonly Dictionary<uint, GumpDecorationAssetBinding> _views = [];
    /// <summary>실제 자산 타입 범위/블록을 검증해 번호를 같은 SHP 토큰으로 보관한다.</summary>
    public GumpDecorationAssets(ShapeDatabase shapes, IEnumerable<GumpDecorationAssetBinding> bindings)
    { ArgumentNullException.ThrowIfNull(shapes); ArgumentNullException.ThrowIfNull(bindings); _shapes = shapes;
        // 원본 자산 타입은70부터다.미작성 첫 I 프레임은 생성 시 정규화하지 않는다.
        foreach (var b in bindings) { if (b.Type < 70 || b.Block < 0 || b.Block >= shapes.Blocks().Count) throw new ArgumentException("장식 자산 연결 범위 오류"); _views.Add(b.Type, b); }
    }
    /// <summary>현재 타입의 값 사본.생성 뒤 캐시와 독립적으로 조회할 수 있다.</summary>
    public GumpDecorationType Type(uint type) => _views.TryGetValue(type, out var b) ? new(type, b.FirstFrame) : throw new ArgumentOutOfRangeException(nameof(type), "장식 실제 타입 없음");
    /// <summary>논리 수 대신 실제 물리 프레임만 확인한다.</summary>
    public (ShapeDatabase Database, int Block) Shape(uint shape, uint frame) => _views.TryGetValue(shape, out var b) && frame < _shapes.Blocks()[b.Block].Frames.Count ? (_shapes, b.Block) : throw new ArgumentOutOfRangeException(nameof(frame), "장식 물리 프레임 범위 오류");
    /// <summary>같은 실제 SHP와 I 첫 프레임을 생성 경계에 공급한다.</summary>
    public static void BindGumpDecorationAssets(GumpDecorationHooks hooks, GumpDecorationAssets assets) { ArgumentNullException.ThrowIfNull(assets); hooks.Type = assets.Type; }
}
/// <summary>현재 캐시 SHP를 같은8비트 표면/클립에 원색으로 출력한다.색표나 배율을 적용하지 않는다.</summary>
public sealed class GumpDecorationPixelTarget
{
    /// <summary>현재 공유 클립/픽셀.</summary>
    private readonly GumpPixelTarget _target;
    /// <summary>같은 실제 SHP 대응.</summary>
    private readonly GumpDecorationAssets _assets;
    /// <summary>실제 물리 프레임 해제 캐시.</summary>
    private readonly Dictionary<(uint, uint), IndexedImage> _images = [];
    /// <summary>실제 표면과 자산의 수명을 빌린다.</summary>
    public GumpDecorationPixelTarget(GumpPixelTarget target, GumpDecorationAssets assets) { ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(assets); _target = target; _assets = assets; }
    /// <summary>기존 생성/영역 경계를 보존하고 원색 출력만 연결한다.</summary>
    public GumpDecorationHooks Hooks(GumpDecorationHooks? hooks = null) { hooks ??= new(); hooks.Shape = Shape; return hooks; }
    /// <summary>캐시 SHP 물리 프레임과 상대 RECT를 그대로 읽는다.특수 프레임은 출력하지 않는다.</summary>
    private void Shape(uint shape, uint frame, ScreenPoint position)
    { var (database, block) = _assets.Shape(shape, frame); var source = database.Blocks()[block].Frames[(int)frame]; if (source.IsSpecial()) return; var key = (shape, frame); if (!_images.TryGetValue(key, out var image)) { image = database.Decode(block, (int)frame); _images.Add(key, image); } _target.Image(image, new(unchecked(position.X + source.Rect.Left), unchecked(position.Y + source.Rect.Top))); }
}
