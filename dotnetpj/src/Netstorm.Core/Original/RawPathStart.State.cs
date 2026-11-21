// 실제 명령 인자와 현재 타입/스냅/재귀 guard의 같은 owner를 보관한다.
namespace Netstorm.Core.Original;

/// <summary>현재 부모TYPE의 실제 이동/회전 속도와 기본 프레임이다.</summary>
/// <param name="Speed">현재 이동 속도다.</param>
/// <param name="TurningSpeed">현재 회전 속도다.</param>
/// <param name="DefaultFrame">통지 뒤 다시 읽는 기본 프레임이다.</param>
public readonly record struct RawPathStartType(float Speed, float TurningSpeed, int DefaultFrame);

/// <summary>RawPathStart.h의 원문8인자 값형 요청이다.</summary>
/// <param name="Parent">실제 부모SID다.</param>
/// <param name="Destination">원하는 float32 목적지다.</param>
/// <param name="ReplaceStart">연속한 시작 꼭짓점을 현재 위치로 바꾸는 DWORD다.</param>
/// <param name="Helper">실제 helper DWORD다.</param>
/// <param name="TailWord">원문 꼬리 DWORD를 WORD로 미리 줄이지 않는다.</param>
/// <param name="TailByte">원문 꼬리 DWORD를 BYTE로 미리 줄이지 않는다.</param>
/// <param name="Reserved">재귀 요청에 보존할 마지막 DWORD다.</param>
public readonly record struct RawPathStartRequest(int Parent, RawPathPoint Destination, uint ReplaceStart = 0, uint Helper = 0, uint TailWord = 0, uint TailByte = 0, uint Reserved = 0);

/// <summary>원문 참조 전역을 값 사본으로 바꾸지 않는 현재 시작 상태다.</summary>
public sealed class RawPathStartState
{
    /// <summary>같은 Host/Prepare 시각과 현재 권한이다.</summary>
    public SquidProcessState Clock { get; }
    /// <summary>현재 Dude TYPE의 실제 owner다.</summary>
    public UIntBox DudeTypeBox { get; }
    /// <summary>현재 텔레포트 TYPE의 실제 owner다.</summary>
    public UIntBox TeleportTypeBox { get; }
    /// <summary>현재 마나 볼트 TYPE의 실제 owner다.</summary>
    public UIntBox ManaBoltTypeBox { get; }
    /// <summary>현재 사제 TYPE의 실제 owner다.</summary>
    public UIntBox PriestTypeBox { get; }
    /// <summary>현재 간헐천 TYPE의 실제 owner다.</summary>
    public UIntBox GeyserTypeBox { get; }
    /// <summary>목적지 스냅의 실제 bool owner다.</summary>
    public BoolBox SnapPriestDestinationBox { get; }
    /// <summary>재귀 guard의 실제 bool owner이며 실패 분기에서 직접 바꾼다.</summary>
    public BoolBox RetryingBox { get; }
    /// <summary>현재 로컬 부착 Dude TYPE다.</summary>
    public uint DudeType { get => DudeTypeBox.Value; set => DudeTypeBox.Value = value; }
    /// <summary>현재 로컬 부착 텔레포트 TYPE다.</summary>
    public uint TeleportType { get => TeleportTypeBox.Value; set => TeleportTypeBox.Value = value; }
    /// <summary>현재 로컬 부착 마나 볼트 TYPE다.</summary>
    public uint ManaBoltType { get => ManaBoltTypeBox.Value; set => ManaBoltTypeBox.Value = value; }
    /// <summary>현재 사제 통지 TYPE다.</summary>
    public uint PriestType { get => PriestTypeBox.Value; set => PriestTypeBox.Value = value; }
    /// <summary>현재 목적지 대상 TYPE다.</summary>
    public uint GeyserType { get => GeyserTypeBox.Value; set => GeyserTypeBox.Value = value; }
    /// <summary>현재 사제 목적지의 거의올림-0.5 선택이다.</summary>
    public bool SnapPriestDestination { get => SnapPriestDestinationBox.Value; set => SnapPriestDestinationBox.Value = value; }
    /// <summary>현재 실패 재귀 guard다.</summary>
    public bool Retrying { get => RetryingBox.Value; set => RetryingBox.Value = value; }
    /// <summary>제공된 실제 owner를 빌리며 독립 입력만 원문 기본값 상자를 소유한다.</summary>
    public RawPathStartState(SquidProcessState clock, UIntBox? dudeType = null, UIntBox? teleportType = null, UIntBox? manaBoltType = null, UIntBox? priestType = null, UIntBox? geyserType = null, BoolBox? snapPriestDestination = null, BoolBox? retrying = null)
    {
        Clock = clock; DudeTypeBox = dudeType ?? new() { Value = 70 }; TeleportTypeBox = teleportType ?? new() { Value = 134 }; ManaBoltTypeBox = manaBoltType ?? new() { Value = 105 }; PriestTypeBox = priestType ?? new() { Value = 158 }; GeyserTypeBox = geyserType ?? new() { Value = 122 }; SnapPriestDestinationBox = snapPriestDestination ?? new(); RetryingBox = retrying ?? new();
    }
}

/// <summary>시작 부모가 빌리는 실제 타입 분배/생성/metadata/겹침 정리 함수 값이다.</summary>
public sealed record RawPathStartHooks
{
    /// <summary>현재 부모TYPE의 실제 이동 프로세스 선택이다.</summary>
    public Func<uint, uint>? ProcessType { get; init; }
    /// <summary>실제 같은 풀의 사제54/텔레포트23 생성이다.</summary>
    public Func<bool, RawPathProcess?>? Create { get; init; }
    /// <summary>현재 부모TYPE의 실제 속도/기본 프레임 조회다.</summary>
    public Func<int, RawPathStartType>? Metadata { get; init; }
    /// <summary>현재 부모 위치/TYPE/owner의 실제 겹침 정리다.</summary>
    public Action<RawPathPoint, uint, uint>? ClearBlocked { get; init; }
    /// <summary>비행/운반 등 실제 파생 클래스 생성이며 원문 선택 시 필수다.</summary>
    public Func<uint, RawPathProcess?>? CreateType { get; init; }
}
