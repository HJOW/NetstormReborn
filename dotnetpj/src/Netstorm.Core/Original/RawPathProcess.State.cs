// 경로가 같은 실제 프로세스 시각/현재 역할/진단/효과 함수를 빌린다.
namespace Netstorm.Core.Original;

/// <summary>RawPathProcess.h의 mutable 경로 전역과 같은 시계 참조다.</summary>
public sealed class RawPathProcessState
{
    /// <summary>같은 Host가 사용하는 현재 게임 시각/권한이다.</summary>
    public SquidProcessState Clock { get; }
    /// <summary>Step이 갱신하는 같은 관찰 전역이다.</summary>
    public RawPathObservation Observation { get; } = new();
    /// <summary>현재 텔레포트 경로 역할 번호다.</summary>
    public uint TeleportProcess { get; set; } = 23;
    /// <summary>현재 사제 경로 역할 번호다.</summary>
    public uint PriestProcess { get; set; } = 54;
    /// <summary>현재 사제 부모TYPE다.</summary>
    public uint PriestType { get; set; } = 158;
    /// <summary>현재 비행 경로 역할 번호다.</summary>
    public uint FlyProcess { get; set; } = 39;
    /// <summary>00427da0의 지면 검사 허용TYPE이며 CD는 첫4개만 사용한다.</summary>
    public uint[] GroundTypes { get; } = [158, 137, 156, 135, 187];
    /// <summary>독립 입력의 원문 겹침 부류 마스크다.</summary>
    public uint BlockingGenus { get; set; } = 0x10444200;
    /// <summary>실제 장기 마스크 owner가 있으면 그 현재 DWORD를 읽는다.</summary>
    public UIntBox? SharedBlockingGenus { get; set; }
    /// <summary>현재 웜 경로 역할 번호다.</summary>
    public uint WormProcess { get; set; } = 40;
    /// <summary>현재 썬 워커 부모TYPE다.</summary>
    public uint SunWalkerType { get; set; } = 133;
    /// <summary>현재 고갈된 운반자 부모TYPE다.</summary>
    public uint EmptyType { get; set; } = 187;
    /// <summary>현재 벌룬 경로 역할 번호다.</summary>
    public uint BalloonProcess { get; set; } = 42;
    /// <summary>화염 검사에서 읽는 현재 벌룬TYPE다.</summary>
    public uint BalloonType { get; set; } = 111;
    /// <summary>현재 Dude 경로 역할 번호다.</summary>
    public uint DudeProcess { get; set; } = 24;
    /// <summary>현재 Dude 부모TYPE다.</summary>
    public uint DudeType { get; set; } = 70;
    /// <summary>현재 워커 경로 역할 번호다.</summary>
    public uint WalkerProcess { get; set; } = 38;
    /// <summary>현재 윈드 워커 부모TYPE다.</summary>
    public uint WindWalkerType { get; set; } = 137;
    /// <summary>현재 레인 워커 부모TYPE다.</summary>
    public uint RainWalkerType { get; set; } = 156;
    /// <summary>같은 세션에서 윈드 보행이 직접 갱신하는 DWORD다.</summary>
    public uint WindWalkerCounter;
    /// <summary>현재 벌프 경로 역할 번호다.</summary>
    public uint BulfProcess { get; set; } = 41;
    /// <summary>자기 삭제 뒤에도 원래 owner를 읽는 현재 진단 뷰다.</summary>
    public RawCheckingFlag Checking { get; set; } = new();
    /// <summary>장기 마스크 또는 독립 마스크의 현재 값을 선택한다.</summary>
    public uint ReadBlockingGenus() => SharedBlockingGenus?.Value ?? BlockingGenus;
    /// <summary>시계/권한을 복사하지 않고 같은 실제 Host 상태를 참조한다.</summary>
    public RawPathProcessState(SquidProcessState clock) => Clock = clock ?? throw new ArgumentNullException(nameof(clock));
}

/// <summary>경로가 빌리는 실제 가상/탐색/수명/소리 함수 값이다.</summary>
public sealed record RawPathProcessHooks
{
    /// <summary>같은 Pool/hash/spot의 실제 Unpop이다.</summary>
    public Action<int, uint>? Unpop { get; init; }
    /// <summary>같은 Pool/hash/spot의 실제 Repop이다.</summary>
    public Action<int, uint>? Repop { get; init; }
    /// <summary>현재 부모의 실제 x/y/flags Pop이다.</summary>
    public Action<int, RawPathPoint, uint>? Pop { get; init; }
    /// <summary>부모+38의 실제10/11/12 반환이다.</summary>
    public Func<int, uint>? Halt { get; init; }
    /// <summary>부모+3c의 실제 도착이며 자신을 삭제할 수 있다.</summary>
    public Func<int, uint, uint, uint>? Arrival { get; init; }
    /// <summary>현재 raw 부모 좌표/현재TYPE 소리의 실제 요청이다.</summary>
    public Action<int, float, float, uint>? StepSound { get; init; }
    /// <summary>같은 현재TYPE 자산의 stepSound 번호다.</summary>
    public Func<int, uint>? SoundId { get; init; }
    /// <summary>같은 Runtime에 쓰는 실제 BFS이며 선택 시 누락을 진단한다.</summary>
    public Action<RawPathRuntime, int, RawPathPoint, RawPathPoint, uint>? Search { get; init; }
    /// <summary>운반 대상 변경 뒤 기존 실제 재탐색이다.</summary>
    public Action<RawPathRuntime, int>? Retarget { get; init; }
    /// <summary>현재 부모/좌표/TYPE/owner의 실제 겹침 정리다.</summary>
    public Action<int, RawPathPoint, uint, uint>? ClearBlocked { get; init; }
    /// <summary>공간 접두 이후 실제 예약/의식/대상 정리다.</summary>
    public Action<RawPathRuntime, int, uint>? Cleanup { get; init; }
    /// <summary>CD의 외부 관찰 시계이며 게임 시각과 구별한다.</summary>
    public Func<ulong>? ObservationStamp { get; init; }
    /// <summary>현재 벌룬 좌표의 화염 위치 소리다.</summary>
    public Action<float, float>? BalloonFlame { get; init; }
    /// <summary>같은 게임 공용 반열린 난수다.</summary>
    public Func<int, int, int>? BulfRandom { get; init; }
    /// <summary>현재 벌프 좌표/파일명의 실제 위치 소리다.</summary>
    public Action<float, float, string>? BulfSound { get; init; }
    /// <summary>자기 삭제 뒤에도 유효한 실제 원문 진단이다.</summary>
    public Action<string, string, uint>? Assertion { get; init; }
}
