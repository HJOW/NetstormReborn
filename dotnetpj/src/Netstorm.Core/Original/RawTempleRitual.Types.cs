// 신전 의식이 읽는 현재 원본 전역과 실제 하위 효과 경계를 둔다.
namespace Netstorm.Core.Original;

/// <summary>원본 단정도 저장 x/y 두 개인 새 좌표 자료형이다.</summary>
public readonly record struct TempleRitualPoint(float X, float Y);

/// <summary>RawTempleRitual.h의 의식 helper가 효과 뒤에도 다시 읽는 현재 원본 전역이다.</summary>
public sealed class TempleRitualState
{
    /// <summary>후속 모드에서 룬을 생성할 권한이다. 00540bc4/CD00540a2c다.</summary>
    public bool Boss { get; set; } = true;
    /// <summary>현재 사제 타입 번호다. 005412d0/CD0051cbbc다.</summary>
    public uint PriestType { get; set; } = 158;
    /// <summary>현재 룬 타입 번호다. 005412f0/CD0051cbdc다.</summary>
    public uint RuneType { get; set; } = 166;
    /// <summary>SHP 정수 핫스폿을 월드 좌표로 바꾸는 x 배율이다. 0059a92c/CD00565c44다.</summary>
    public float PixelScaleX { get; set; } = 16;
    /// <summary>SHP 정수 핫스폿을 월드 좌표로 바꾸는 y 배율이다. 0059a930/CD00565c48이다.</summary>
    public float PixelScaleY { get; set; } = 11;
    /// <summary>RunePostPop/주변 정리와 같은 현재005412f0의 장기 owner다. 독립 구성은 현재 값을 쓴다.</summary>
    public UIntBox? SharedRuneType { get; set; }
    /// <summary>같은 Client의 현재 사제 TYPE 주소다. 독립 구성은 현재 값을 쓴다.</summary>
    public UIntBox? SharedPriestType { get; set; }
    /// <summary>같은 Client의 현재 권한 주소다. 독립 구성은 현재 값을 쓴다.</summary>
    public BoolBox? SharedBoss { get; set; }
    /// <summary>일반 finder의 각 현재 후보에서 사제 번호를 읽는다. 참조형 소비자에 임시 값을 반환하지 않는다.</summary>
    public uint PriestTypeCurrent() => SharedPriestType?.Value ?? PriestType;
    /// <summary>사제 후속 효과가 돌아온 뒤 Rune 생성 가지의 현재 권한을 읽는다.</summary>
    public bool BossCurrent() => SharedBoss?.Value ?? Boss;
    /// <summary>함수가 번호를 저장하는 위치는 기존 본문에 유지하며 현재 룬 번호를 읽는다.</summary>
    public uint RuneTypeCurrent() => SharedRuneType?.Value ?? RuneType;
}

/// <summary>RawTempleRitual.h의 실제 Factory/공통 HP/owner/Pop/위치 음향/공유 예약의 필수 하위 연결이다.</summary>
public sealed class TempleRitualHooks
{
    /// <summary>Factory.Create(현재 runeType,flags0)이며 실제 슬롯 번호를 반환한다.</summary>
    public Func<uint, uint, int>? Create { get; set; }
    /// <summary>기존 공통 setter이며 Rune HP0을 쓴다.</summary>
    public Action<int, int>? SetHitPoints { get; set; }
    /// <summary>원본 가상+0x74를 현재 신전 owner BYTE로 요청한다.</summary>
    public Action<int, uint>? SetOwner { get; set; }
    /// <summary>원본 가상+0x90을 현재 StageAnchor 위치와flags0으로 요청한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>현재 사제 좌표에서 단계별 for*.wav를flags0으로 요청한다.</summary>
    public Action<float, float, string, uint>? PlayAt { get; set; }
    /// <summary>위치 음향 뒤 원본 new0x28 성공 여부를 공급한다.</summary>
    public Func<bool>? ReserveShared { get; set; }
    /// <summary>같은 Host.AddSharedRegular(신전,0x307,단계별지연)이다.</summary>
    public Action<int, uint, float>? AddShared { get; set; }
    /// <summary>Stage가 저장한 프레임 뒤 현재 대상의 가상+80을 한 번 조회한다. Count 자체의 직접 방향 계산과 구별한다.</summary>
    public Func<int, uint>? Count { get; set; }
    /// <summary>PATCH isVoid 진단이 정상 복귀하면 plain extra 쓰기/소유자/Pop을 계속한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다. 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal TempleRitualHooks Copy() => (TempleRitualHooks)MemberwiseClone();
}
