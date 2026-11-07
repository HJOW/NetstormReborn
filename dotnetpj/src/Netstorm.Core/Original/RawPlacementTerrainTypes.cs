// 배치 지형이 공유하는 입력·셀·프레임 표 형식을 복원한다.
namespace Netstorm.Core.Original;

// 배치 지형 형식을 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.

// 사제의 패턴 없는 Canon 순회에 필요한 프레임 표와 기본 프레임 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class PriestPlainCanonType
{
    /// <summary>타입의 실제 프레임 코드 표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public RiftTypeFrames Frames { get; set; } = new RiftTypeFrames(Array.Empty<RiftFrameCode>());

    /// <summary>타입의 기본 프레임 번호다. -1이면 유효 칸을 찾지 못한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public int DefaultFrame { get; set; }
}

// 일반 타입·패턴 배치 검사가 격자 칸마다 읽는 정보다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public readonly record struct CanonPlacementCell(
    int Frame,
    int Label,
    byte Side,
    float X,
    float Y,
    float SnappedX,
    float SnappedY,
    SquidSearchArea Area)
{
    /// <summary>현재 복호기 프레임과 패턴의 셀 라벨이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public int Frame { get; init; } = Frame;

    /// <summary>패턴의 셀 라벨이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public int Label { get; init; } = Label;

    /// <summary>조회 가능한 실제 프레임 코드의 방향 문자다. 없으면 0이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public byte Side { get; init; } = Side;

    /// <summary>원래 가로 좌표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public float X { get; init; } = X;

    /// <summary>원래 세로 좌표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public float Y { get; init; } = Y;

    /// <summary>지형 원점에 쓰는 절삭한 가로 좌표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public float SnappedX { get; init; } = SnappedX;

    /// <summary>지형 원점에 쓰는 절삭한 세로 좌표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public float SnappedY { get; init; } = SnappedY;

    /// <summary>현재 타입 발자국으로 계산한 탐색기의 네 정수 인자다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public SquidSearchArea Area { get; init; } = Area;
}

// 사제 배치 검사에 전달하는 입력 값이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public readonly record struct PriestPlacementQuery(
    uint Type,
    float X,
    float Y,
    uint Flags,
    uint Owner,
    uint Mode)
{
    /// <summary>타입 표에서 객체 종류를 찾는 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint Type { get; init; } = Type;

    /// <summary>가로 좌표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public float X { get; init; } = X;

    /// <summary>세로 좌표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public float Y { get; init; } = Y;

    /// <summary>원본 검사 인자의 플래그다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint Flags { get; init; } = Flags;

    /// <summary>소유자다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint Owner { get; init; } = Owner;

    /// <summary>모드다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint Mode { get; init; } = Mode;
}
