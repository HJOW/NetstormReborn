// 전체 제품의 같은 현재 시각·권한·타입 번호를 공유·독립 저장소에서 읽는다.
namespace Netstorm.Core.Original;

// 프로세스 상태 모듈을 담는 네임스페이스다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.

// 부모 객체의 이벤트 처리기다. 원본 base는 payload를 그대로 돌려준다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.
public delegate float RegularHandler(
    /// <summary>부모 객체 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    int parent,
    /// <summary>사건 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    uint ev,
    /// <summary>개수다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    uint count,
    /// <summary>전달 값이다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    float payload);

// 프로세스 실행기가 읽고 쓰는 현재 시각·권한·타입 번호 묶음이다. 빌린 주소가 있으면 같은 물리를 직접 읽고 쓰며, 없으면 독립 값을 쓴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.
public sealed class SquidProcessState
{
    /// <summary>독립 입력의 게임 시각이다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private double _now;

    /// <summary>독립 입력의 권한 플래그다. 꺼져 있으면 서버 번호 form은 flag 0x10 없이 지우지 않는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private bool _boss = true;

    /// <summary>독립 입력의 서버 전역이다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private bool _server = true;

    /// <summary>독립 입력의 현재 간헐천 타입이다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _pollType = PollTypeDefault;

    /// <summary>독립 입력의 현재 일반 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _regularType = RegularTypeDefault;

    /// <summary>독립 입력의 현재 공유 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _sharedRegularType = SharedRegularTypeDefault;

    /// <summary>독립 입력의 상한 비트다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _processTypeEnd = ProcessTypeEndDefault;

    /// <summary>독립 입력의 원문 최초 ProcessForm DWORD다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _processFormType = ProcessFormTypeDefault;

    /// <summary>독립 입력의 발사 애니메이션 프로세스 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _fireAnimationType = FireAnimationTypeDefault;

    /// <summary>독립 입력의 회전 애니메이션 프로세스 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    private uint _rotateAnimationType = RotateAnimationTypeDefault;

    /// <summary>현재 간헐천 타입의 독립 초기값이다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint PollTypeDefault = 122u;

    /// <summary>독립 PATCH 입력의 현재 일반 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint RegularTypeDefault = 46u;

    /// <summary>독립 PATCH 입력의 현재 공유 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint SharedRegularTypeDefault = 61u;

    /// <summary>독립 PATCH 입력의 signed 상한 비트다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint ProcessTypeEndDefault = 63u;

    /// <summary>독립 입력의 원문 최초 ProcessForm DWORD다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint ProcessFormTypeDefault = 2u;

    /// <summary>독립 입력의 발사 애니메이션 프로세스 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint FireAnimationTypeDefault = 35u;

    /// <summary>독립 입력의 회전 애니메이션 프로세스 번호다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public const uint RotateAnimationTypeDefault = 36u;

    /// <summary>패치 Kernel 등록이 전진시키는 타입 순환 값이다. CD에는 없다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public uint TypeRotation { get; set; }

    /// <summary>독립 SharedRegular 생성의 현재 basePostPop 입력이며 실제 월드는 아래 대여 getter를 사용한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public uint PostPopDepth { get; set; }

    /// <summary>독립 진단 정상 복귀·실패를 명시하며 기본 깊이 0에서는 요구하지 않는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>실행기보다 오래 사는 실제 두 bool이다. 미연결이면 기존 값형 입력만 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public RawAuthorityState? SharedAuthority { get; set; }

    /// <summary>실제 Client FrameTime.game의 현재 주소다. 장치 소유자는 Host보다 오래 유지한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public DoubleBox? SharedNow { get; set; }

    /// <summary>명시 장기 TYPE owner가 있을 때만 연결하며 Session 지역 슬롯은 빌리지 않는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? SharedPollType { get; set; }

    /// <summary>실제 Client의 같은 장기 세 DWORD를 빌린다. null이면 독립 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? CurrentRegularType { get; set; }

    /// <summary>실제 Client의 같은 장기 세 DWORD를 빌린다. null이면 독립 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? CurrentSharedRegularType { get; set; }

    /// <summary>실제 Client의 같은 장기 세 DWORD를 빌린다. null이면 독립 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? CurrentProcessTypeEnd { get; set; }

    /// <summary>실제 Client GameAux의 같은 장기 번호 주소를 빌린다. null이면 독립 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? CurrentProcessFormType { get; set; }

    /// <summary>실제 Client의 장기 주소를 빌린다. null이면 독립 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? CurrentFireAnimationType { get; set; }

    /// <summary>실제 Client의 장기 주소를 빌린다. null이면 독립 값을 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public UIntBox? CurrentRotateAnimationType { get; set; }

    /// <summary>전체 제품의 같은 현재 시각을 읽고 독립 입력만 fallback을 사용한다. 빌린 주소가 있으면 직접 쓰고 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public double Now
    {
        get => SharedNow is not null ? SharedNow.Value : _now;
        set
        {
            if (SharedNow is not null)
            {
                SharedNow.Value = value;
            }
            else
            {
                _now = value;
            }
        }
    }

    /// <summary>Kill의 원문 소비 위치에서 현재 권한을 직접 읽거나 쓴다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public bool Boss
    {
        get => SharedAuthority is not null ? SharedAuthority.Boss : _boss;
        set
        {
            if (SharedAuthority is not null)
            {
                SharedAuthority.Boss = value;
            }
            else
            {
                _boss = value;
            }
        }
    }

    /// <summary>Form 통지·서버 상태 출력이 현재 주소를 직접 사용한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public bool Server
    {
        get => SharedAuthority is not null ? SharedAuthority.Server : _server;
        set
        {
            if (SharedAuthority is not null)
            {
                SharedAuthority.Server = value;
            }
            else
            {
                _server = value;
            }
        }
    }

    /// <summary>Regular의 현재 간헐천 타입을 같은 장기 DWORD에서 조회한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    public uint PollType
    {
        get => SharedPollType is not null ? SharedPollType.Value : _pollType;
        set
        {
            if (SharedPollType is not null)
            {
                SharedPollType.Value = value;
            }
            else
            {
                _pollType = value;
            }
        }
    }

    /// <summary>getter 호출마다 현재 전체 번호를 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 일반 번호다.</returns>
    public uint RegularType() => CurrentRegularType is not null ? CurrentRegularType.Value : _regularType;

    /// <summary>생성·조회가 같은 공유 번호를 사용한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 공유 번호다.</returns>
    public uint SharedRegularType() => CurrentSharedRegularType is not null ? CurrentSharedRegularType.Value : _sharedRegularType;

    /// <summary>상한을 읽기 전에 signed 변환·정규화를 하지 않는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 상한 비트다.</returns>
    public uint ProcessTypeEnd() => CurrentProcessTypeEnd is not null ? CurrentProcessTypeEnd.Value : _processTypeEnd;

    /// <summary>각 생성·Take 위치마다 현재 전체 번호를 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 ProcessForm 번호다.</returns>
    public uint ProcessFormType() => CurrentProcessFormType is not null ? CurrentProcessFormType.Value : _processFormType;

    /// <summary>일반 생성·조회와 전체 수신이 같은 현재 번호를 사용한다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 발사 번호다.</returns>
    public uint FireAnimationType() => CurrentFireAnimationType is not null ? CurrentFireAnimationType.Value : _fireAnimationType;

    /// <summary>발사와 별개인 회전 DWORD를 복사 없이 읽는다. 2026-11-07 F2d 프로세스 상태 이식에서 추가됐다.</summary>
    /// <returns>현재 회전 번호다.</returns>
    public uint RotateAnimationType() => CurrentRotateAnimationType is not null ? CurrentRotateAnimationType.Value : _rotateAnimationType;
}
