// Rune 후처리 조회와 시간 helper가 읽는 현재 원본 전역과 실제 효과 경계를 둔다.
namespace Netstorm.Core.Original;

/// <summary>RawRunePostPop.h의 현재 전역이다.</summary>
public sealed class RunePostPopState
{
    /// <summary>원본 현재타입 번호다. 각 조회시 다시 읽는다.</summary>
    public uint RuneType { get; set; } = 166;
    /// <summary>원본 현재타입 번호다. 각 조회시 다시 읽는다.</summary>
    public uint DaisType { get; set; } = 165;
    /// <summary>원본 현재타입 번호다. 각 조회시 다시 읽는다.</summary>
    public uint AltarType { get; set; } = 164;
    /// <summary>altar/player 시간선택을 끄고 10초로 반환한다.</summary>
    public bool SuppressDuration { get; set; }
    /// <summary>patchPlayer+50/CDPlayer+50의 현재 bit1이다. 원래 ownerWORD가 index다.</summary>
    public uint[] PlayerFlags { get; } = new uint[9];
    /// <summary>같은 Client005412e8의 장기 주소다. 독립 구성은 현재 값을 쓴다.</summary>
    public UIntBox? SharedAltarType { get; set; }
    /// <summary>같은 Client005412ec의 장기 주소다. 독립 구성은 현재 값을 쓴다.</summary>
    public UIntBox? SharedDaisType { get; set; }
    /// <summary>같은 현재005412f0을 빌린다. 독립 구성은 현재 값을 쓴다.</summary>
    public UIntBox? SharedRuneType { get; set; }
    /// <summary>같은 현재00594fa4를 빌린다. 독립 구성은 현재 값을 쓴다.</summary>
    public BoolBox? SharedSuppressedDuration { get; set; }
    /// <summary>여러 의식 소비자가 원래 owner의 같은 번호를 계속 읽는다. 참조형 소비자에 임시 값을 반환하지 않는다.</summary>
    public uint CurrentRuneType() => SharedRuneType?.Value ?? RuneType;
    /// <summary>표시/소리/Player 갱신 콜백 뒤 현재 미션 상태를 다시 읽는다.</summary>
    public bool CurrentSuppressDuration() => SharedSuppressedDuration?.Value ?? SuppressDuration;
    /// <summary>보상/의식/참여자/AI의 장기 참조를 같은 실제 DWORD에 연결한다.</summary>
    public uint CurrentAltarType() => SharedAltarType?.Value ?? AltarType;
    /// <summary>finder는 현재 값을 복사하고 참조형 소비자는 원래 owner 수명을 그대로 빌린다.</summary>
    public uint CurrentDaisType() => SharedDaisType?.Value ?? DaisType;
}

/// <summary>RawRunePostPop.h의 예약/건설/공통후처리 효과 묶음이다. 도달하지 않은 효과는 빈 성공으로 대체하지 않는다.</summary>
public sealed class RunePostPopHooks
{
    /// <summary>최초flags1의 new0x28 성공/실패다.</summary>
    public Func<bool>? ReserveRegular { get; set; }
    /// <summary>Host.AddRegular(Rune,0x304,0.01F)의 로컬예약이다.</summary>
    public Action<ushort, uint, float>? AddRegular { get; set; }
    /// <summary>가까운같은owner신전이있으면기존Construction.Start(Rune,7/10/25)를호출한다.</summary>
    public Action<ushort, float>? StartConstruction { get; set; }
    /// <summary>연결된 실제 월드는 시간 helper가 도달한 위치에서 현재00594fa4를 읽는다. 독립 구성은 state를 사용한다.</summary>
    public Func<bool>? SuppressDuration { get; set; }
    /// <summary>같은 현재 Player 저장소의 owner별+50을 읽으며 독립 구성은 state 배열을 사용한다.</summary>
    public Func<uint, uint>? PlayerFlags { get; set; }
    /// <summary>효과 묶음의 얕은 사본을 만든다. 외부 재연결을 캡처된 장치와 구별한다.</summary>
    internal RunePostPopHooks Copy() => (RunePostPopHooks)MemberwiseClone();
}
