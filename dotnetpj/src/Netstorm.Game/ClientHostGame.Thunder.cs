using Netstorm.Core.Original;

// 같은 Game의 실제 Surface/Screen/Kernel을 번개 부착과 모드·최종 종료 정리에 제공한다.
namespace Netstorm.Client.Host;

/// <summary>장면/날씨와 원문 명령이 함께 사용할 실제 Thunder의 Game 부분이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>등록 PID만 장기 보유하며 실제 프로세스는 같은 Kernel이 소유한다.</summary>
    private HostThunderFlashes? _nativeThunder;

    /// <summary>State 자원 owner 준비 뒤 같은 현재 장치만 연결한다. 프로세스·곡·시각·RNG를 실행하지 않는다.</summary>
    private void BindNativeThunderOwner()
    {
        if (_nativeThunder is { } current)
        {
            if (!ReferenceEquals(current.Surface, _surfaceGlobals) || !ReferenceEquals(current.Screen, _driver.Screen)
                || !ReferenceEquals(current.Kernel, _kernel) || !ReferenceEquals(current.Clock, _clientClock))
                throw new InvalidOperationException("Game 번개의 실제 장기 장치 교체 불일치");
            return;
        }
        _nativeThunder = new(_surfaceGlobals, _driver.Screen, _kernel, _clientClock);
    }

    /// <summary>현재 실제 기술 부모를 조회해 번개를 요청한다. 감독/명령 호출자 설치는 별도다.</summary>
    public void RequestNativeThunderFlash(int brightness = 0)
        => (_nativeThunder ?? throw new InvalidOperationException("실제 Game 번개 owner 미설치")).Request(brightness);

    /// <summary>현재 등록된 실제 번개를 Screen/Host 반납 및 표시 모드 변경보다 먼저 취소한다.</summary>
    private void ClearNativeThunderFlashes() => _nativeThunder?.Clear();
}
