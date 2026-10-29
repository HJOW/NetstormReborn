using Netstorm.Client.Display;

namespace Netstorm.Client.Gump; // 실제 배경 출력 분기·현재 자산·클립·팔레트 연결 영역이다.

/// <summary>배경 디버그와 flat 경로의 현재 1픽셀 테두리 색 전역이다.
/// 근거: cpppj/src/client/GumpBackgroundDraw.h.</summary>
public sealed class GumpBackgroundDrawState
{
    /// <summary>켜졌으면 배경 대신 현재 디버그 색의 체크 fill을 출력한다.</summary>
    public bool Debug { get; set; }
    /// <summary>디버그 fill 직전에 DWORD로 증가하는 색.</summary>
    public uint DebugColor { get; set; }
    /// <summary>flat 경로의 왼쪽/위쪽 선 색.</summary>
    public uint NearColor { get; set; }
    /// <summary>flat 경로의 오른쪽/아래쪽 선 색.</summary>
    public uint FarColor { get; set; }
}

/// <summary>현재 타입/프레임을 화면 기준점에 출력하는 요청. Mapped=false이면 현재 색 표를 무시한다.</summary>
/// <param name="Type">현재 타입.</param>
/// <param name="Frame">현재 프레임.</param>
/// <param name="Position">화면 기준점.</param>
/// <param name="Mapped">현재 명암 소스 치환표 사용.</param>
public readonly record struct GumpBackgroundShapeRequest(uint Type, uint Frame, ScreenPoint Position, bool Mapped);

/// <summary>같은 현재 노드/표면에 자산·클립·소스 명암표·픽셀을 연결한다.</summary>
public sealed class GumpBackgroundDrawHooks
{
    /// <summary>현재 배경 파생 노드.</summary>
    public Func<uint, GumpBackgroundNode?>? Node { get; set; }
    /// <summary>매 질감 pass의 현재 타입/프레임 추가 헤더. 생성 캐시를 쓰지 않는다.</summary>
    public Func<uint, uint, GumpBackgroundFrame>? Frame { get; set; }
    /// <summary>같은 논리/VFX 클립을 함께 쓰는 경계.</summary>
    public Action<ScreenRect>? Clip { get; set; }
    /// <summary>현재 소스 치환표에1 밝음/2 어두움을 설치한다.</summary>
    public Action<uint>? Shade { get; set; }
    /// <summary>현재 원색 또는 현재 소스 표로 실제 셰이프를 출력한다.</summary>
    public Action<GumpBackgroundShapeRequest>? Shape { get; set; }
    /// <summary>영역·DWORD 색·체크 여부로 fill한다.</summary>
    public Action<ScreenRect, uint, bool>? Fill { get; set; }
    /// <summary>양 끝 포함 현재 색 선.</summary>
    public Action<GumpLine>? Line { get; set; }
}

/// <summary>현재 위치/배율/색/클립을 원본 순서로 읽는 배경 가상/직접 Draw다.
/// 근거: cpppj/src/client/GumpBackgroundDraw.cpp, 00465c60/CD00494e40 및00465880/CD00494a20.</summary>
public sealed class GumpBackgroundDraw
{
    /// <summary>같은 노드의 현재 위치/배율/캐시.</summary>
    private readonly GumpCollection _collection;
    /// <summary>공유 현재 논리/VFX 클립.</summary>
    private readonly GumpDrawState _clip;
    /// <summary>현재 디버그/선 색.</summary>
    private readonly GumpBackgroundDrawState _state;
    /// <summary>실제 자산과 픽셀 경계.</summary>
    private readonly GumpBackgroundDrawHooks _hooks;

    /// <summary>실제 도달한 분기의 빠진 경계만 명시 오류로 거부한다.</summary>
    private static T Require<T>(T? value) where T : Delegate => value ?? throw new ArgumentException("배경 Draw 출력/자산 경계 필요");

    /// <summary>기본 노드 조회만 필수다. flat 경로에는 프레임/타입 경계가 필요 없다.</summary>
    public GumpBackgroundDraw(GumpCollection collection, GumpDrawState clip, GumpBackgroundDrawState state, GumpBackgroundDrawHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(collection); ArgumentNullException.ThrowIfNull(clip); ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        Require(hooks.Node); _collection = collection; _clip = clip; _state = state; _hooks = hooks;
    }

    /// <summary>파일 팔레트에서 마지막 계산한 흰색/회색 번호를 읽고 디버그 상태는 보존한다.</summary>
    public static void SetGumpBackgroundColors(GumpBackgroundDrawState state, PaletteColorTable palette)
    {
        state.NearColor = palette.Basic[7]; state.FarColor = palette.Basic[8];
    }

    /// <summary>Bounds 조회 뒤 현재 debug 또는 노드의 flags/type/frame을 읽는다. 디버그 증가는 fill보다 먼저다.</summary>
    public void Draw(uint token)
    {
        ScreenRect rect = _collection.Bounds(token);
        if (_state.Debug) { uint color = _state.DebugColor; _state.DebugColor = unchecked(color + 1); Require(_hooks.Fill)(rect, color, true); return; }
        GumpBackgroundNode node = Require(_hooks.Node)(token) ?? throw new InvalidOperationException("배경 Draw 토큰/저장소 없음");
        Background(rect, node.BackgroundFlags, node.Type, node.Frame);
    }

    /// <summary>체크→단색→질감 순으로 선택한다. flat의 선은 왼쪽→위→오른쪽→아래 순서며 pressed로 색을 바꾸지 않는다.</summary>
    public void Background(ScreenRect rect, uint flags, uint type, uint frame)
    {
        if ((flags & GumpBackgroundFlags.Checker) != 0) Require(_hooks.Fill)(rect, flags & 255, true);
        else if ((flags & GumpBackgroundFlags.Solid) != 0) Require(_hooks.Fill)(rect, flags & 255, false);
        else if ((flags & GumpBackgroundFlags.Texture) != 0)
        {
            Texture(rect, flags, type, frame); ScreenRect saved = _clip.Clip;
            uint near = (flags & GumpBackgroundFlags.Pressed) != 0 ? GumpBackgroundFlags.Dark : GumpBackgroundFlags.Bright;
            uint far = (flags & GumpBackgroundFlags.Pressed) != 0 ? GumpBackgroundFlags.Bright : GumpBackgroundFlags.Dark;
            if ((flags & GumpBackgroundFlags.Left) != 0) Texture(ScreenGeometry.ClipScreenRect(new(rect.Left, rect.Top, unchecked(rect.Left + 2), rect.Bottom), _clip.Clip), flags | near, type, frame);
            if ((flags & GumpBackgroundFlags.Right) != 0) Texture(ScreenGeometry.ClipScreenRect(new(unchecked(rect.Right - 2), rect.Top, rect.Right, rect.Bottom), saved), flags | far, type, frame);
            if ((flags & GumpBackgroundFlags.Top) != 0) Texture(ScreenGeometry.ClipScreenRect(new(rect.Left, rect.Top, rect.Right, unchecked(rect.Top + 2)), saved), flags | near, type, frame);
            if ((flags & GumpBackgroundFlags.Bottom) != 0) Texture(ScreenGeometry.ClipScreenRect(new(rect.Left, unchecked(rect.Bottom - 2), rect.Right, rect.Bottom), saved), flags | far, type, frame);
            return;
        }
        if ((flags & GumpBackgroundFlags.Left) != 0) Require(_hooks.Line)(new(new(rect.Left, rect.Top), new(rect.Left, unchecked(rect.Bottom - 1)), _state.NearColor));
        if ((flags & GumpBackgroundFlags.Top) != 0) Require(_hooks.Line)(new(new(rect.Left, rect.Top), new(unchecked(rect.Right - 1), rect.Top), _state.NearColor));
        if ((flags & GumpBackgroundFlags.Right) != 0) Require(_hooks.Line)(new(new(unchecked(rect.Right - 1), rect.Top), new(unchecked(rect.Right - 1), unchecked(rect.Bottom - 1)), _state.FarColor));
        if ((flags & GumpBackgroundFlags.Bottom) != 0) Require(_hooks.Line)(new(new(rect.Left, unchecked(rect.Bottom - 1)), new(unchecked(rect.Right - 1), unchecked(rect.Bottom - 1)), _state.FarColor));
    }

    /// <summary>프레임 조회 뒤의 현재 클립을 저장한다. C++ RAII를 try/finally로 옮겨 초기 clip 실패/출력 예외에도 복구하며 명암표/픽셀은 보존한다.</summary>
    public void Texture(ScreenRect rect, uint flags, uint type, uint frame)
    {
        GumpBackgroundFrame metrics = Require(_hooks.Frame)(type, frame); ScreenRect saved = _clip.Clip;
        GumpTexturePass plan = GumpBackground.PlanGumpTexture(rect, saved, metrics.Width, metrics.Height, flags); if (!plan.ValidTile) return;
        Action<ScreenRect> set = Require(_hooks.Clip);
        try { set(plan.Clip); } catch { set(saved); throw; }
        try
        {
            if (plan.Shade != 0) Require(_hooks.Shade)(plan.Shade);
            // 빈 클립도 원본의 모든 여분 타일 요청을 전달한다. 원색은 설치된 표를 무시한다.
            foreach (ScreenPoint origin in plan.Origins) Require(_hooks.Shape)(new(type, frame, origin, plan.Shade != 0));
        }
        finally { set(saved); }
    }

    /// <summary>배경 토큰만 이 가상 Draw로 분배하며 기존 돌 버튼/다른 파생 처리기를 캡처해 보존한다.</summary>
    public static void BindGumpBackgroundDraw(GumpDrawHooks hooks, GumpBackgroundDraw draw, Func<uint, bool> isBackground)
    {
        ArgumentNullException.ThrowIfNull(isBackground); Action<uint>? previous = hooks.Draw;
        hooks.Draw = token => { if (isBackground(token)) draw.Draw(token); else Require(previous)(token); };
    }
}

/// <summary>원본 타입·실제 SHP 블록·논리 프레임 수·추가 레이어 플래그의 명시 연결이다.</summary>
/// <param name="Type">원본 타입 번호.</param>
/// <param name="Block">실제 SHP 블록.</param>
/// <param name="FrameCount">원본 클러스터 논리 수.</param>
/// <param name="Flags">원본 flags1.</param>
public readonly record struct GumpBackgroundAssetBinding(uint Type, int Block, int FrameCount, uint Flags);

/// <summary>실제 SHP 헤더를 생성과 매 pass에 공급한다. GameAssets 전체 소유 구조의 연결은 후속 자산 조립에서 추가한다.
/// 근거: cpppj/src/client/GumpBackgroundDraw.cpp 의 GumpBackgroundAssets 명시 바인딩 경로.</summary>
public sealed class GumpBackgroundAssets
{
    /// <summary>타입 하나의 안정된 메타데이터와 모든 물리 헤더.</summary>
    private sealed class View
    {
        /// <summary>호스트 타입 번호를 주소 토큰 대신 쓰는 안정된 객체.</summary>
        public required GumpBackgroundType Type { get; init; }
        /// <summary>실제 데이터베이스 블록 번호.</summary>
        public int Block { get; init; }
        /// <summary>원본의 논리 클러스터 수.</summary>
        public int Count { get; init; }
        /// <summary>추가 레이어 허용을 포함한 원본 flags1.</summary>
        public uint Flags { get; init; }
        /// <summary>추가/공유 레이어를 모두 포함하는 안정된 헤더 객체.</summary>
        public List<GumpBackgroundFrame> Frames { get; } = [];
    }
    /// <summary>메타데이터와 픽셀을 읽는 실제 데이터베이스.</summary>
    private readonly ShapeDatabase _shapes;
    /// <summary>번호에 대응하는 안정된 타입 저장소.</summary>
    private readonly Dictionary<uint, View> _views = [];

    /// <summary>같은 SHP에 명시 타입 연결을 적용한다. 블록/논리 수/중복을 검사하고 모든 signed WORD 메타를 보존한다.</summary>
    public GumpBackgroundAssets(ShapeDatabase shapes, IEnumerable<GumpBackgroundAssetBinding> bindings)
    {
        ArgumentNullException.ThrowIfNull(shapes); _shapes = shapes;
        // 입력 순서의 타입 번호/블록을 그대로 등록한다. 물리 프레임 수를 논리 수로 바꾸지 않는다.
        foreach (GumpBackgroundAssetBinding b in bindings)
        {
            if (b.Type == 0 || _views.ContainsKey(b.Type) || b.Block < 0 || b.Block >= shapes.Blocks().Count || b.FrameCount < 1 || b.FrameCount > shapes.Blocks()[b.Block].Frames.Count)
                throw new ArgumentException("배경 자산 바인딩 오류");
            var view = new View { Type = new() { Token = b.Type, Shape = b.Type }, Block = b.Block, Count = b.FrameCount, Flags = b.Flags };
            // 추가/공유 프레임을 포함해 원본의 모든 실제 헤더를 순서대로 보관한다.
            for (int frame = 0; frame < shapes.Blocks()[b.Block].Frames.Count; frame++)
            {
                SquidFrameMetrics m = shapes.SquidMetrics(b.Block, frame); view.Frames.Add(new() { Width = m.Width, Height = m.Height });
            }
            _views.Add(b.Type, view);
        }
    }

    /// <summary>없는 타입/내장 무자산 타입은 호스트 예외로 거부하며 같은 객체를 반환한다.</summary>
    public GumpBackgroundType Type(uint type) => _views.TryGetValue(type, out View? view) ? view.Type : throw new ArgumentOutOfRangeException(nameof(type), "배경 자산 타입 없음");

    /// <summary>abstract0 및 signed frame 범위/추가 레이어/실제 물리 표를 모두 확인한다.</summary>
    public GumpBackgroundFrame Frame(uint type, uint abstractLayer, uint frame)
    {
        if (abstractLayer != 0) throw new ArgumentException("배경 자산 abstract 인자는0이어야 함"); Type(type); View v = _views[type];
        ulong maximum = (ulong)v.Count * ((v.Flags & 0x440000) != 0 ? 2U : 1U);
        if (frame >= maximum || frame >= (uint)v.Frames.Count) throw new ArgumentOutOfRangeException(nameof(frame), "배경 SHP 프레임 범위 오류"); return v.Frames[(int)frame];
    }

    /// <summary>현재 프레임을 매 출력 재검사하고 실제 DB/블록을 반환한다. 생성 노드의 이전 shape는 쓰지 않는다.</summary>
    public (ShapeDatabase Database, int Block) Shape(uint type, uint frame) { Frame(type, 0, frame); return (_shapes, _views[type].Block); }

    /// <summary>같은 안정된 타입/헤더를 배경 생성과 매 pass 조회에 함께 연결한다.</summary>
    public static void BindGumpBackgroundAssets(GumpBackgroundCreateHooks create, GumpBackgroundDrawHooks draw, GumpBackgroundAssets assets)
    {
        create.Type = assets.Type; create.Frame = assets.Frame; draw.Frame = (type, frame) => assets.Frame(type, 0, frame);
    }
}

/// <summary>배경 자산/명암표/같은 현재 VFX 소스 색표를 실제 GumpPixelTarget에 연결한다.</summary>
public sealed class GumpBackgroundPixelTarget
{
    /// <summary>현재 클립의 실제8비트 표면.</summary>
    private readonly GumpPixelTarget _target;
    /// <summary>현재 타입/프레임의 실제 데이터베이스.</summary>
    private readonly GumpBackgroundAssets _assets;
    /// <summary>파일 팔레트의 밝음/어두움 표.</summary>
    private readonly GumpShadeMaps _maps;
    /// <summary>글자와 공유 가능한 현재 VFX 소스 치환표. 참조를 바꾸지 않고 내용만 복사한다.</summary>
    private readonly byte[] _colors;
    /// <summary>같은 DB/블록/프레임의 압축 해제 그림 캐시.</summary>
    private readonly Dictionary<(ShapeDatabase, int, uint), IndexedImage> _images = [];

    /// <summary>실제 표면/자산/파일 명암표와256바이트 현재 색표를 빌린다.</summary>
    public GumpBackgroundPixelTarget(GumpPixelTarget target, GumpBackgroundAssets assets, GumpShadeMaps maps, byte[] colors)
    {
        ArgumentNullException.ThrowIfNull(target); ArgumentNullException.ThrowIfNull(assets); ArgumentNullException.ThrowIfNull(maps); ArgumentNullException.ThrowIfNull(colors);
        if (colors.Length != 256) throw new ArgumentException("배경 VFX 색표 크기 오류"); _target = target; _assets = assets; _maps = maps; _colors = colors;
    }

    /// <summary>노드/프레임 경계를 보존하고 같은 현재 표면의 clip/shade/shape/fill/line만 연결한다.</summary>
    public GumpBackgroundDrawHooks Hooks(GumpDrawState state, GumpBackgroundDrawHooks? hooks = null)
    {
        hooks ??= new(); hooks.Clip = clip => GumpDraw.SetGumpDrawClip(state, clip);
        hooks.Shade = shade => { if (shade != 1 && shade != 2) throw new ArgumentException("배경 명암 표 선택 오류"); (shade == 1 ? _maps.Bright : _maps.Dark).CopyTo(_colors, 0); };
        hooks.Shape = Shape; hooks.Fill = _target.Fill; hooks.Line = line => _target.Lines([line]); return hooks;
    }

    /// <summary>특수 프레임은 쓰지 않고 실제 프레임 상대영역을 화면 기준점에 더한다. 명암은 대상 색이 아닌 소스 색을 치환한다.</summary>
    private void Shape(GumpBackgroundShapeRequest request)
    {
        (ShapeDatabase database, int block) = _assets.Shape(request.Type, request.Frame); ShapeFrame frame = database.Blocks()[block].Frames[(int)request.Frame]; if (frame.IsSpecial()) return;
        var key = (database, block, request.Frame);
        if (!_images.TryGetValue(key, out IndexedImage? image)) { image = database.Decode(block, (int)request.Frame); _images.Add(key, image); }
        _target.Image(image, new(unchecked(request.Position.X + frame.Rect.Left), unchecked(request.Position.Y + frame.Rect.Top)), request.Mapped ? _colors : null);
    }
}
