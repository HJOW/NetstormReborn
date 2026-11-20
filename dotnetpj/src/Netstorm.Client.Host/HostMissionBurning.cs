using System.Runtime.Versioning;
using Netstorm.Client.Display;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 제품호스트의 실제 자산/팔레트/소리와 같은 세션 연소를 조립한다.
namespace Netstorm.Client.Host;

/// <summary>원본 연소의 외부 장치만 소유한다. Host/Form/Kernel은 같은 HostSession이 소유한다.</summary>
public sealed class HostMissionBurning : IDisposable
{
    /// <summary>같은 실제 부모체인과 장기HP/난수 owner다.</summary>
    private readonly HostSession _session;
    /// <summary>파생보다 오래 살아야 하는 실제 색수/인접메모리 소유자다.</summary>
    private readonly ParticlePaletteMemory _memory;
    /// <summary>공유색수와 실제snapshot을 다른 소비자가 복제하지 않게 보관한다.</summary>
    private readonly BurningDrawPalette _palette = new();
    /// <summary>다음 첫색 생성이 병합할 실제 색띠다.</summary>
    private readonly List<ParticleGradient> _gradients = [];
    /// <summary>현재 프레임 게이트가 읽는 호스트 프레임 번호다.</summary>
    private uint _frame;
    /// <summary>R5의 실제 카메라/월드viewport를 연결했는지 구별한다.</summary>
    private bool _viewBound;
    /// <summary>자원 반납 뒤 현재장치 재사용을 막는다.</summary>
    private bool _disposed;

    /// <summary>같은 현재 시각/카메라/바람 입력이다. 아직 없는 월드뷰는0사각형 성공으로 가장하지 않는다.</summary>
    public BurningFrameState FrameState { get; } = new();
    /// <summary>전체Renderer가 받을 때까지 원문순서로 보존할 실제 변경영역이다.</summary>
    public DirtyRegions Dirty { get; }
    /// <summary>같은 실제SHP/현재raw 캡처를 원본대조와 후속 소비자에 빌린다.</summary>
    public RawShapeToBuffer Capture { get; }
    /// <summary>같은 실제 캡처·dirty 큐와 공통 표시 목록을 공유하는 건설 장치다.</summary>
    public HostMissionConstruction Construction { get; }

    /// <summary>같은 실제 타입/프레임·COL·SHP·소리장치를 세션의 단일연소 owner에 연결한다.</summary>
    [SupportedOSPlatform("windows")]
    public HostMissionBurning(HostSession session, HostStartupScene startup, Screen screen,
        IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeDefinition> definitions,
        IReadOnlyList<SquidDisplayShape> shapes, byte[] shp, HostMissionAudio audio)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(screen);
        ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(definitions); ArgumentNullException.ThrowIfNull(shapes);
        ArgumentNullException.ThrowIfNull(shp); ArgumentNullException.ThrowIfNull(audio);
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 연소 팔레트 메모리는 Windows 전용이다");
        _memory = new(); _session = session; Dirty = new(screen.Width, screen.Height);
        var tokens = new uint[types.Count];
        // cpppj 실제 장치 없는 보조표와 같은 일반12종의 등록이다.
        foreach (int type in new[] { 82, 155, 158, 164, 135, 133, 156, 73, 170, 146, 144, 126 }) tokens[type] = 1;
        // 전용 표는 일반 항등/색띠로 대신하지 않는다. 실제 Client 공통색표 owner가 후속 연결된다.
        foreach (int type in new[] { 82, 155, 111, 141, 137, 94, 97 }) tokens[type] = 2;
        var colors = OwnerColorMapBuilder.BuildStandardOwnerColorMaps(0, session.BridgeState.BridgeType);
        var factory = new RawFactoryDraw(session.Pool, session.WorldFrames);
        var database = new Netstorm.Client.Display.ShapeDatabase(shp);
        var captureState = new ShapeToBufferState
        {
            AddTypeHotspot = true, OwnerTypeMaps = tokens,
            OwnerColors = (type, owner) =>
            {
                int color = session.BridgeState.OwnerColors[checked((int)owner)];
                if (color < 0 || color > 8) throw new ArgumentOutOfRangeException(nameof(owner), "연소 현재 소유자 색 범위");
                if (tokens[type] != 1) throw new InvalidOperationException("연소 캡처의 실제 전용 소유자 색표 owner 미연결(R5)");
                return colors.Tables[color];
            },
            MainFrame = sid =>
            {
                ReadOnlySpan<byte> raw = session.Pool.Slot(sid);
                if (factory.Handles(sid)) return factory.MainFrame(sid);
                if (!RawPlainUnitCapture.Matches(raw)) throw new InvalidOperationException("연소 다른 파생의 실제 주그림 캡처 미연결(R5)");
                return definitions[raw[10] - 70].SpecialFrames.BaseFrame;
            },
            Additional = (sid, image, x, y) =>
            {
                if (factory.Handles(sid))
                {
                    factory.Additional(sid, x, y, (px, py, type, frame) =>
                        VfxDraw.DrawShape(image.Pixels, image.Width, image.Height, database, checked((int)type) - 70, frame,
                            px, py, new(0, 0, image.Width - 1, image.Height - 1)));
                    return;
                }
                if (!RawPlainUnitCapture.Matches(session.Pool.Slot(sid))) throw new InvalidOperationException("연소 다른 파생의 실제 추가그림 캡처 미연결(R5)");
                // 이13개 VT의+a4=41c100은 원문 빈ret8이다.
            },
        };
        Capture = new(session.Pool, types, shapes, database, captureState);
        BurningFrameHooks frame = BurningFrameCapture.Make(Capture, session.Pool, new() { StopSound = audio.StopEffect });
        Func<int, ConstructionVisualImage> actualCapture = frame.Capture!;
        frame.Capture = sid =>
        {
            if (!_viewBound) throw new InvalidOperationException("연소 실제 카메라/월드viewport 미연결(R5)");
            return actualCapture(sid);
        };
        var hooks = new BurningSystemHooks
        {
            Visual = new()
            {
                // 실제 입자배열/공유색표 소유자가 저장소를 확보한다. 원문 요청 종류도 검증한다.
                Reserve = bytes => bytes is 3600 or 8 ? true : throw new InvalidOperationException("연소 실제 확보량 오류"),
                CreatePalette = static () => 4,
                Gradient = (from, to, count, width) =>
                {
                    var logical = new uint[256];
                    // 원본 게임COL RGBX의 실제 번호 순서를 유지한다.
                    for (int index = 0; index < logical.Length; index++)
                    {
                        PaletteColor color = startup.Palette.Color((byte)index);
                        logical[index] = color.Red | ((uint)color.Green << 8) | ((uint)color.Blue << 16);
                    }
                    _gradients.Add(ParticleGradients.Build(logical, from, to, checked((int)count), width));
                },
                BuildPalette = token =>
                {
                    if (token != 4 || _gradients.Count != 1) throw new InvalidOperationException("연소 첫색표 토큰/색띠 오류");
                    _memory.Assign(ParticleGradients.Merge(_gradients.ToArray())); _palette.Count = _memory.Count;
                    _palette.Readable = _memory.Snapshot(); _gradients.Clear();
                },
            },
            Frame = frame, Invalidate = rect => Dirty.Add(new(rect.Left, rect.Top, rect.Right, rect.Bottom)),
            Clip = () =>
            {
                if (!_viewBound) throw new InvalidOperationException("연소 실제 출력clip 미연결(R5)");
                ScreenRect clip = screen.Clip; return new(clip.Left, clip.Top, clip.Right, clip.Bottom);
            },
            Pixel = (x, y, color) =>
            {
                if ((uint)x >= (uint)screen.Pitch || (uint)y >= (uint)screen.Height) throw new ArgumentOutOfRangeException(nameof(x), "연소 실제 표면 좌표 범위");
                byte[] pixels = screen.Lock(); try { pixels[y * screen.Pitch + x] = color; } finally { screen.Unlock(); }
            },
            Release = static _ => { }, // 실제관리배열/등록/Kernel이 소유버퍼와객체를 반납한다.
        };
        session.BuildBurningWorldDevices(FrameState, _palette, () => _frame, hooks, audio.BulfHurt);
        Construction = new(session, startup, screen, Capture, Dirty, () => _viewBound);
    }
    /// <summary>실제 프레임 준비 위치에서 현재 원문시각/프레임번호를 공급한다. 커널 실행은 수행하지 않는다.</summary>
    public void PrepareFrame(double now, uint frame)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (!double.IsFinite(now)) throw new ArgumentOutOfRangeException(nameof(now), "연소 현재 게임시각 오류");
        FrameState.Now = now; Construction.FrameState.Now = now; _frame = frame;
    }
    /// <summary>R5가 실제 카메라/월드viewport를 준비한 뒤 호출한다. 전체화면 크기를 임의 월드뷰로 쓰지 않는다.</summary>
    public void SetView(float cameraX, float cameraY, DisplayRect viewport, bool inset = false)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        FrameState.CameraX = cameraX; FrameState.CameraY = cameraY; FrameState.Viewport = viewport; FrameState.InsetViewport = inset; _viewBound = true;
        Construction.FrameState.CameraX = cameraX; Construction.FrameState.CameraY = cameraY;
    }
    /// <summary>같은 Host의 연소를 먼저 없앤 뒤 외부 실제팔레트 메모리를 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        Construction.Dispose(); _session.ReleaseBurningWorld(); if (OperatingSystem.IsWindows()) _memory.Dispose(); _disposed = true;
    }
}
