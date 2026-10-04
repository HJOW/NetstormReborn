namespace Netstorm.Core.Simulation;

/// <summary>피해와 분리된 착탄·파괴 그림 기록. 이미 제거된 목표에도 위치와 시작 틱을 보존한다.</summary>
public sealed record CombatImpact(double X, double Y, long Tick, char Group);

/// <summary>원본 anim 클러스터를 세션 시계로 표시한다. 이 목록은 피해·난수·검사합에 영향을 주지 않는다.</summary>
public sealed partial class BattleSession
{
    /// <summary>폭발 그림 간격(초). 팬게임과 원본 그림 수를 참고한 값이며 녹화의 전체 주기는 추가 측정이 필요하다.</summary>
    public const double ImpactFrameSeconds = 0.04;

    /// <summary>가장 긴 35그림 폭발을 보관할 시간(초).</summary>
    private const double ImpactLifetimeSeconds = 35 * ImpactFrameSeconds;

    /// <summary>최근 착탄·폭발의 화면 기록.</summary>
    private readonly List<CombatImpact> _impacts = [];

    /// <summary>렌더러가 읽는 착탄·폭발 위치. 세션이 멈추면 그림도 같은 단계에 남는다.</summary>
    public IReadOnlyList<CombatImpact> Impacts => _impacts.AsReadOnly();

    /// <summary>원본 anim의 작은 폭발(D)·방어선 먼지(C)를 남기고 탄 종류에 맞는 명중음을 요청한다.</summary>
    private void RecordShotImpact(CombatShot shot)
    {
        if (shot.IsBeam) return;
        _impacts.Add(new CombatImpact(shot.EndX, shot.EndY, Tick, shot.BlockedByFenceId != 0 ? 'C' : 'D'));
        string? sound = shot.AttackerType?.ToLowerInvariant() switch
        {
            "sunarcher" => "sunDiscThrowerImpact.wav",
            "thundercannon" => "thunderCannonImpact.wav",
            "raincannon" => "iceCannonImpact2-800.WAV",
            _ => null,
        };
        if (shot.BlockedByFenceId == 0 && sound != null)
            Emit(SessionEventKind.CombatSound, shot.Owner, shot.AttackerId, sound);
    }
}
