// 경로 탐색이 World보다 오래 빌리는 전역·거리표·공유 소유자를 보관한다.
namespace Netstorm.Core.Original;

// 경로 탐색 저장소 모듈을 담는 네임스페이스다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.

// Path.cpp가 직접 읽고 바꾸는 전역이다. 명령을 만든 부모의 owner를 별도 이동 허가 정책으로 바꾸지 않는다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
public sealed class RawPathSearchState
{
    /// <summary>GraphAt 조회 허용 전역이며 번호 254는 Graph 없음이다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public bool GraphsEnabled { get; set; } = true;

    /// <summary>원본 kenGo 활성 전역이다. 같은 Graph·허용 지면에서 꺼졌으면 원본 assert 경로다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public bool SearchEnabled { get; set; } = true;

    /// <summary>역추적 후보 raw+12 &amp; 6 == 2에 30 비용을 더한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public bool AvoidFlagTwo { get; set; } = true;

    /// <summary>패치 PE 초기값 6이다. 5면 방향 전제 3번을 5로 바꾼다. CD에는 이 분기가 없다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public int PolicySelector { get; set; } = 6;

    /// <summary>대각선은 양옆 직선 방향의 허용 비트가 필요하다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint[] DirectionPrerequisite { get; } = [0u, 5u, 0u, 20u, 0u, 80u, 0u, 65u];

    /// <summary>원본 방문·동률 선택 우선순위다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public int[] DirectionOrder { get; } = [0, 2, 4, 6, 1, 3, 5, 7];

    /// <summary>surface가 아닌 예외 Graph 바이트 getter의 현재 타입 번호다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint FenceMarkType { get; set; } = 90u;

    /// <summary>surface가 아닌 예외 Graph 바이트 getter의 현재 타입 번호다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint WindArcherType { get; set; } = 140u;
}

// 원본 signed WORD 거리표와 두 바이트 큐의 미사용 흔적까지 보존한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
public sealed class RawPathSearchWork
{
    /// <summary>256×256 칸 수다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public const int CellCount = 65536;

    /// <summary>BFS 큐 항목 수다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public const int QueueEntries = 16384;

    /// <summary>256×256 칸의 원본 signed WORD BFS 거리표다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public short[] Distance { get; } = new short[CellCount];

    /// <summary>BFS 방문 순서대로 X·Y 바이트 좌표를 담는 16,384항목 큐다. 항목마다 X·Y 2바이트가 차례로 있다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public byte[] Queue { get; } = new byte[QueueEntries * 2];
}

// 원문 59514c의 거리·큐 소유자는 Session보다 오래 살며 실제 BFS 도달 여부를 구별한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
public sealed class RawPathSearchStorage
{
    /// <summary>기존 실제 거리·큐 배열을 공유하며 진단에서 새로 계산하지 않는다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public RawPathSearchWork? Work { get; set; }

    /// <summary>실제 탐색 셀의 거리표 쓰기에 도달한 뒤에만 원문 비 NULL로 관측한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public bool Initialized { get; set; }
}
