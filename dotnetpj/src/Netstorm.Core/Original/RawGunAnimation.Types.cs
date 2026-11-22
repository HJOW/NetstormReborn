// 총 애니메이션의 현재 전역과 실제 월드 효과 인자를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>실제 Missile 생성자가 받는 열두 인자의 값형 요청이다.</summary>
public sealed class GunProjectileRequest
{
    /// <summary>발사 부모 자산SID다.</summary>
    public ushort Firing { get; set; }
    /// <summary>피해 표적의SID다.</summary>
    public ushort Target { get; set; }
    /// <summary>원문 기본 타일/초 속도다.</summary>
    public float Speed { get; set; } = 20;
    /// <summary>비행 표시 자산의 원문TYPE다.</summary>
    public uint DisplayType { get; set; } = 80;
    /// <summary>충돌 효과의 원문TYPE다.</summary>
    public uint EffectType { get; set; } = 11;
    /// <summary>추적한 표적 raw 위치다.</summary>
    public float[] TargetPosition { get; set; } = new float[2];
    /// <summary>실제 교차 레인 배리어 코너 경계의 반환좌표다.</summary>
    public float[] TargetCenter { get; set; } = new float[2];
    /// <summary>최초0인 반사 순번이다.</summary>
    public uint Ricochet { get; set; }
    /// <summary>이 공통 요청이 실제로 넘기는 최초0 좌표이며 후속 파생의 invalid 좌표와 구별한다.</summary>
    public float[] FiringPosition { get; set; } = new float[2];
}
/// <summary>원문 현재 시각/썬 캐논 역할과 발사 효과 번호다.</summary>
public sealed class GunAnimationState
{
    /// <summary>현재 게임시각으로 상위 Frame이 갱신한다.</summary>
    public double Now { get; set; }
    /// <summary>독립 입력의 최초128 역할이다.</summary>
    public uint SunCannonType { get; set; } = 128;
    /// <summary>장기 Client의 현재00541258을 빌린다.</summary>
    public UIntBox? CurrentSunCannonType { get; set; }
    /// <summary>현재 비행 표시80 역할이다.</summary>
    public uint DisplayType { get; set; } = 80;
    /// <summary>현재 충돌11 역할이다.</summary>
    public uint EffectType { get; set; } = 11;
    /// <summary>소비위치마다 현재역할을 읽는다.</summary>
    public uint ReadSunCannonType() => CurrentSunCannonType?.Value ?? SunCannonType;
}
/// <summary>큰 월드 효과는 선택되는 지점에서만 실제 공급을 요구한다.</summary>
public sealed class GunAnimationHooks
{
    /// <summary>현재 자산 발자국 중심이다.</summary>
    public Func<ushort, float[]>? Center { get; set; }
    /// <summary>교차 레인 배리어 코너 선택이다.</summary>
    public Func<ushort, ushort, float[]>? TargetPoint { get; set; }
    /// <summary>프레임변경 전에 실행하는 실제 공간해제다.</summary>
    public Action<ushort, uint>? Unpop { get; set; }
    /// <summary>프레임변경 뒤0x40으로 실행하는 실제 공간재등록이다.</summary>
    public Action<ushort, uint>? Repop { get; set; }
    /// <summary>회전소리 번호의 구간난수다.</summary>
    public Func<int, int, int>? RandomInteger { get; set; }
    /// <summary>현재부모 위치의 WAV 재생이다.</summary>
    public Action<ushort, string>? Sound { get; set; }
    /// <summary>실제 Missile 생성/Pop/Kernel 부착 경계다.</summary>
    public Action<GunProjectileRequest>? CreateProjectile { get; set; }
    /// <summary>현재 GunProcess의 실제 event1 통지다.</summary>
    public Action<ushort, uint, uint, uint>? Notify { get; set; }
    /// <summary>통지뒤 현재 animation을 flags10으로 삭제한다.</summary>
    public Action<ushort, uint>? Kill { get; set; }
    /// <summary>Source 함수값 복사를 유지하며 외부슬롯교체를 분리한다.</summary>
    internal GunAnimationHooks Copy() => (GunAnimationHooks)MemberwiseClone();
}
