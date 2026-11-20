using System.Runtime.Versioning;
using Netstorm.Client.Display;
using Netstorm.Core.Display;
using Netstorm.Core.Original;

// 같은 실제 SHP 캡처·COL 팔레트와8비트 화면에 건설 표시를 제출한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignParticles.cpp ConnectConstruction의 외부 장치와 공유 색 메모리 소유자다.</summary>
public sealed class HostMissionConstruction : IDisposable
{
    /// <summary>같은Host/Form/Kernel과 등록소의 실제 세션이다.</summary>
    private readonly HostSession _session;
    /// <summary>현재 잠금·행 간격·dirty clip의 실제 화면이다.</summary>
    private readonly Screen _screen;
    /// <summary>R5의 실제 카메라/월드뷰 연결을 조회한다.</summary>
    private readonly Func<bool> _viewBound;
    /// <summary>실제11색과 인접DWORD를 소유하며 프로세스보다 오래 산다.</summary>
    private readonly ParticlePaletteMemory _memory;
    /// <summary>생성과 프레임이 같은 토큰3으로 읽는 실제 색 표다.</summary>
    private readonly ConstructionFramePalette _palette = new();
    /// <summary>첫 생성 때 병합할 두 원본 색 띠다.</summary>
    private readonly List<ParticleGradient> _gradients = [];
    /// <summary>외부 장치 반납 뒤 재사용을 거부한다.</summary>
    private bool _disposed;
    /// <summary>실제 프레임 시각과 카메라 원점을 제품 호스트가 갱신한다.</summary>
    public ConstructionFrameState FrameState { get; } = new();

    /// <summary>이미 연결한 공통 목록과 캡처를 빌리고 실제COL의 두 색 띠·화면 출력을 조립한다.</summary>
    [SupportedOSPlatform("windows")]
    public HostMissionConstruction(HostSession session, HostStartupScene startup, Screen screen,
        RawShapeToBuffer capture, DirtyRegions dirty, Func<bool> viewBound)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(startup); ArgumentNullException.ThrowIfNull(screen);
        ArgumentNullException.ThrowIfNull(capture); ArgumentNullException.ThrowIfNull(dirty); ArgumentNullException.ThrowIfNull(viewBound);
        if (!OperatingSystem.IsWindows()) throw new PlatformNotSupportedException("현재 건설 팔레트 메모리는 Windows 전용이다");
        _session = session; _screen = screen; _viewBound = viewBound; _memory = new();
        var visual = new ConstructionVisualHooks
        {
            Reserve = static _ => true, // 관리버퍼/실제 색 할당의 실패는 예외로 전달한다.
            CreatePalette = static () => 3,
            Gradient = (count, from, to, width) =>
            {
                var logical = new uint[256];
                // 같은 실제COL의 RGBX 순서로 색 번호를 찾는다.
                for (int index = 0; index < logical.Length; index++)
                {
                    PaletteColor color = startup.Palette.Color((byte)index);
                    logical[index] = color.Red | ((uint)color.Green << 8) | ((uint)color.Blue << 16);
                }
                _gradients.Add(ParticleGradients.Build(logical, from, to, checked((int)count), width));
            },
            BuildPalette = token =>
            {
                if (token != 3 || _gradients.Count != 2) throw new InvalidOperationException("건설 실제 공유 색 띠 연결 오류");
                _memory.Assign(ParticleGradients.Merge(_gradients.ToArray()));
                _palette.Token = token; _palette.Count = _memory.Count; _palette.Entries = _memory.Snapshot().ToArray(); _gradients.Clear();
            },
            Capture = sid =>
            {
                if (!_viewBound()) throw new InvalidOperationException("건설 실제 카메라/월드viewport 미연결(R5)");
                return capture.Capture(sid);
            },
            ReleaseParticles = static () => { }, // 원본 본체의 관리배열 교체가 이전 버퍼를 반납한다.
        };
        session.BuildConstructionWorldDevices(FrameState, _palette, visual, new()
        {
            Invalidate = rect => dirty.Add(new(rect.Left, rect.Top, rect.Right, rect.Bottom)), Draw = Draw,
        });
    }

    /// <summary>원본 blit 요청을 현재 화면의 끝을 제외하는clip에 제출하고255 투명 픽셀을 보존한다.</summary>
    public void Draw(ConstructionFrameDraw draw)
    {
        ObjectDisposedException.ThrowIf(_disposed, this); ArgumentNullException.ThrowIfNull(draw);
        if (!_viewBound()) throw new InvalidOperationException("건설 실제 그리기 표면/월드viewport 미연결(R5)");
        ScreenRect clip = _screen.Clip; byte[] pixels = _screen.Lock();
        try
        {
            // 원본 입력 사각형의 행을 현재 출력clip 안에서만 복사한다.
            for (int row = draw.Clip.Top; row < draw.Clip.Bottom; row++)
            {
                long y = (long)draw.Y + row;
                if (y < clip.Top || y >= clip.Bottom || y < 0 || y >= _screen.Height) continue;
                // 출력 범위 밖과255 투명색은 기존 화면을 그대로 보존한다.
                for (int column = draw.Clip.Left; column < draw.Clip.Right; column++)
                {
                    long x = (long)draw.X + column;
                    if (x < clip.Left || x >= clip.Right || x < 0 || x >= _screen.Pitch) continue;
                    byte color = draw.Pixels.Span[checked(row * draw.Pitch + column)];
                    if (color != 255) pixels[checked((int)y * _screen.Pitch + (int)x)] = color;
                }
            }
        }
        finally { _screen.Unlock(); }
    }

    /// <summary>같은 실제 건설 프로세스를 먼저 없앤 뒤 빌려준 색 메모리를 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _session.ReleaseConstructionWorld(); if (OperatingSystem.IsWindows()) _memory.Dispose(); _disposed = true;
    }
}
