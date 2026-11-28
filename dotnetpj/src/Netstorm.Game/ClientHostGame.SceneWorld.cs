using Netstorm.Client.Audio;

// 실제 장기 월드의 현재 조회를 장면 감독의 필수 경계에 제공한다.
namespace Netstorm.Client.Host;

/// <summary>현재 Surface/Player와 기존 CS를 빌린다. 감독 Start/Frame을 실행하거나 새 미션을 만들지 않는다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>미션 교체 뒤에도 같은 현재 Pool/TYPE/목록과 Game의 기존 CS를 읽는 장기 조회다.</summary>
    private SceneMusicWorldQueries? _sceneMusicWorld;

    /// <summary>실제 SceneMusicHooks가 빌릴 희생/대기실 조회다. 초기 결선이 없는 상태를 임의 false로 대체하지 않는다.</summary>
    public SceneMusicWorldQueries SceneMusicWorld => _sceneMusicWorld ?? throw new InvalidOperationException("실제 장면 음악 월드 조회 미연결");

    /// <summary>같은 State/Player 정체성 검증이 끝난 생성자 위치에서 실제 Surface와 기존 CS만 빌린다.</summary>
    private void BindSceneMusicWorld()
    {
        if (_sceneMusicWorld is not null) throw new InvalidOperationException("실제 장면 음악 월드 조회 중복 설치");
        _sceneMusicWorld = SceneMusicWorldQueries.ForCurrentSurface(_surfaceGlobals, _challenge);
    }
}
