using Netstorm.Client.Display;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Net;

// 실제 현재 캡처·카메라·8비트 표면을 같은 Host의 파편16에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>RawCampaignParticles의 파편 외부 장치와 단독 미션의 원격 전용 방송을 소유한다.</summary>
public sealed class HostMissionShrapnel : IDisposable
{
    /// <summary>같은 공장·감시 쌍·전체 가상 삭제를 가진 실제 세션이다.</summary>
    private readonly HostSession _session;
    /// <summary>실제 clip/pitch를 읽는 같은 논리 표면이다.</summary>
    private readonly Screen _screen;
    /// <summary>실제 월드 카메라/viewport 설치 여부다.</summary>
    private readonly Func<bool> _viewBound;
    /// <summary>같은 현재 Player 기록을 읽는 원격 전용 분배기다.</summary>
    private readonly RawBattleBroadcast _broadcast;
    /// <summary>단독 일반 월드의 실제 NULL 연결 표다. 로컬 수신에 되먹이지 않는다.</summary>
    private readonly Action<byte[]>?[] _peers = new Action<byte[]>?[9];
    /// <summary>이미 반납한 장치의 재사용을 막는다.</summary>
    private bool _disposed;
    /// <summary>다른 표시와 같은 준비 위치에서 현재 시각/카메라를 공급한다.</summary>
    public ShrapnelFrameState FrameState { get; } = new();

    /// <summary>생성/입자 계산은 같은 실제 Host에 맡기고 현재 캡처·dirty·픽셀·음향만 공급한다.</summary>
    public HostMissionShrapnel(HostSession session, Screen screen, RawShapeToBuffer capture, DirtyRegions dirty, Func<bool> viewBound, HostMissionAudio audio)
    {
        ArgumentNullException.ThrowIfNull(session); ArgumentNullException.ThrowIfNull(screen); ArgumentNullException.ThrowIfNull(capture);
        ArgumentNullException.ThrowIfNull(dirty); ArgumentNullException.ThrowIfNull(viewBound); ArgumentNullException.ThrowIfNull(audio);
        _session = session; _screen = screen; _viewBound = viewBound;
        _broadcast = new(OriginalEdition.Patch1078, session.PlayerRecords, _peers, () => session.MissionPlayer.BattleOwner, () => session.Gameplay.Authority.Server);
        session.BuildShrapnelWorldDevices(FrameState, new()
        {
            Lifetime = new()
            {
                Reserve = bytes => bytes == 104 ? true : throw new InvalidOperationException("파편 실제 객체 확보량 오류"),
                Capture = sid => { if (!_viewBound()) throw new InvalidOperationException("파편 실제 카메라/월드viewport 미연결(R5)"); return capture.Capture(sid); },
            },
            Frame = new()
            {
                Reserve = bytes => bytes != 0 ? true : throw new InvalidOperationException("파편 실제 저장소 확보량0 오류"),
                Release = static _ => { }, // 실제 관리 입자/작업 배열은 원문 수명 본체가 반납한다.
                Invalidate = rect => dirty.Add(new(rect.Left, rect.Top, rect.Right, rect.Bottom)), Pixel = Draw,
            },
            Release = static _ => { }, // 실제 bitmap/입자/객체의 소유권은 본체/Kernel에 있다.
        });
        session.BuildDropIslandWorldDevices(packet =>
        {
            if (!session.BridgeState.Mission) throw new InvalidOperationException("현재 파편 장치의 MP 실제 원격 연결 표 미설치");
            _broadcast.Send(packet.ToArray());
        }, audio.PlayEffectAt, static (expression, file, line) => throw new InvalidOperationException($"섬 정리 원본 진단: {expression} {file}:{line:x}"));
    }
    /// <summary>현재 clip 밖은 생략하며 clip 안인데 물리 표면 밖이면 원문 장치 불일치를 거부한다.</summary>
    public void Draw(ShrapnelPixel pixel)
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (!_viewBound()) throw new InvalidOperationException("파편 실제 픽셀 표면/월드viewport 미연결(R5)");
        ScreenRect clip = _screen.Clip;
        if (pixel.X < clip.Left || pixel.X >= clip.Right || pixel.Y < clip.Top || pixel.Y >= clip.Bottom) return;
        if (pixel.X < 0 || pixel.X >= _screen.Pitch || pixel.Y < 0 || pixel.Y >= _screen.Height) throw new ArgumentOutOfRangeException(nameof(pixel), "파편 실제 clip/표면 경계 불일치");
        byte[] pixels = _screen.Lock(); try { pixels[pixel.Y * _screen.Pitch + pixel.X] = unchecked((byte)pixel.Color); } finally { _screen.Unlock(); }
    }
    /// <summary>같은 Host의 파편을 표시 목록/장치가 남아 있을 때 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed) return; _session.ReleaseShrapnelWorld(); _disposed = true;
    }
}
