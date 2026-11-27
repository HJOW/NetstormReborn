using Netstorm.Core.Original;

// 같은 월드 자산과 세션 조회를 일반 객체의 픽셀 출력에 묶는다.
namespace Netstorm.Client.Display;

/// <summary>수집한 일반 객체를 실제 가상표와 현재 상태로 화면에 그린다.</summary>
public sealed class SpriteWorldRenderer
{
    /// <summary>현재 월드의 실제 객체 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 월드의 현재 TYPE 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>TYPE 순서의 실제 셰이프 헤더다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;
    /// <summary>파일에서 읽은 TYPE 정의다.</summary>
    private readonly IReadOnlyList<RiftTypeDefinition> _definitions;
    /// <summary>TYPE 순서의 실제 프레임 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>실제 SHP 파일 저장소다.</summary>
    private readonly ShapeDatabase _database;
    /// <summary>현재 카메라·소유자·선택 전역이다.</summary>
    private readonly SpriteDrawState _state;
    /// <summary>공통 Client의 실제 현재 색표다.</summary>
    private readonly SpriteColorBinding _colors;
    /// <summary>같은 세션의 HP·공간·프로세스 조회다.</summary>
    private readonly SpriteDrawHooks _rules;
    /// <summary>실제 TYPE 순서의 표시 헤더 사본이다.</summary>
    private readonly SpriteTypeView[] _views;
    /// <summary>같은 풀의 가상 프레임 조회기다.</summary>
    private readonly SpriteVirtualFrames _virtualFrames;
    /// <summary>현재 표면에 출력하는 실제 다리 추가 그림이다.</summary>
    private readonly Action<int, ScreenPoint, byte[], int, int, ScreenRect>? _bridge;
    /// <summary>현재 표면에 출력하는 실제 메타 추가 그림이다.</summary>
    private readonly Action<int, ScreenPoint, byte[], int, int, ScreenRect>? _meta;
    /// <summary>레인 신전 상승음의 실제 재생 장치다.</summary>
    private readonly Action? _templeAscend;

    /// <summary>같은 월드의 자산·현재 상태·조회 장치를 빌려 출력기를 조립한다.</summary>
    /// <param name="pool">현재 월드 객체 풀이다.</param>
    /// <param name="types">현재 TYPE 표다.</param>
    /// <param name="shapes">TYPE 순서의 실제 셰이프 헤더다.</param>
    /// <param name="definitions">파일 자산 TYPE 정의다.</param>
    /// <param name="frames">TYPE별 실제 프레임 코드다.</param>
    /// <param name="database">실제 SHP 파일이다.</param>
    /// <param name="state">현재 세션의 표시 전역이다.</param>
    /// <param name="colors">같은 Client 공통 색표 조회다.</param>
    /// <param name="rules">같은 세션의 HP·공간·프로세스 조회다.</param>
    /// <param name="bridge">실제 다리 추가 그림이다.</param>
    /// <param name="meta">실제 메타 추가 그림이다.</param>
    /// <param name="templeAscend">실제 레인 신전 상승음 재생이다.</param>
    /// <remarks>cpppj RawCampaignFactoryDisplay의 native Draw 경로를 현재 표면에 묶는다.</remarks>
    public SpriteWorldRenderer(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<SquidDisplayShape> shapes,
        IReadOnlyList<RiftTypeDefinition> definitions, IReadOnlyList<RiftTypeFrames> frames, ShapeDatabase database,
        SpriteDrawState state, SpriteColorBinding colors, SpriteDrawHooks rules,
        Action<int, ScreenPoint, byte[], int, int, ScreenRect>? bridge = null,
        Action<int, ScreenPoint, byte[], int, int, ScreenRect>? meta = null, Action? templeAscend = null)
    {
        _pool = pool ?? throw new ArgumentNullException(nameof(pool));
        _types = types ?? throw new ArgumentNullException(nameof(types));
        _shapes = shapes ?? throw new ArgumentNullException(nameof(shapes));
        _definitions = definitions ?? throw new ArgumentNullException(nameof(definitions));
        _frames = frames ?? throw new ArgumentNullException(nameof(frames));
        _database = database ?? throw new ArgumentNullException(nameof(database));
        _state = state ?? throw new ArgumentNullException(nameof(state));
        _colors = colors ?? throw new ArgumentNullException(nameof(colors));
        _rules = rules ?? throw new ArgumentNullException(nameof(rules));
        if (types.Count != shapes.Count || types.Count != frames.Count) throw new ArgumentException("객체 표시 TYPE·셰이프·프레임 수 불일치");
        _views = SpriteAssetViews.Build(shapes, definitions);
        _virtualFrames = new SpriteVirtualFrames(pool, definitions, frames);
        _bridge = bridge;
        _meta = meta;
        _templeAscend = templeAscend;
    }

    /// <summary>현재 dirty 클립과 화면을 빌려 수집한 객체 하나를 출력한다.</summary>
    /// <param name="entry">SID 주소 토큰을 포함한 현재 수집 항목이다.</param>
    /// <param name="pixels">현재 8비트 화면이다.</param>
    /// <param name="pitch">화면 한 행의 바이트 간격이다.</param>
    /// <param name="height">화면의 행 수다.</param>
    /// <param name="clip">현재 dirty 클립이다.</param>
    public void Draw(RenderCollectionEntry entry, byte[] pixels, int pitch, int height, ScreenRect clip)
    {
        var target = new SpritePixelTarget(pixels, pitch, height, clip,
            SpriteAssetViews.BindShapes(_database, _definitions.Count, _state), _colors.Color);
        SpriteDrawHooks hooks = target.Hooks(_rules);
        Action<int, ScreenPoint>? bridge = _bridge is null ? null : (sid, point) => _bridge(sid, point, pixels, pitch, height, clip);
        Action<int, ScreenPoint>? meta = _meta is null ? null : (sid, point) => _meta(sid, point, pixels, pitch, height, clip);
        var additional = new SpriteVirtualAdditional(_pool, _frames, hooks.Shape!, bridge, meta,
            _types, _definitions, () => _state.LocalPlayer, _shapes, _templeAscend);
        hooks.VirtualFrame = _virtualFrames.Read;
        hooks.VirtualDraw = additional.Draw;
        new SpriteDraw(_pool, _types, _views, _state, hooks, 0).Draw(entry);
    }
}
