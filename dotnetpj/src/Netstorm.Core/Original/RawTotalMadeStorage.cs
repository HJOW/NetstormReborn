// Totalmade의 실제 세 256DWORD만 보관하고 부분 reset을 공유한다.
namespace Netstorm.Core.Original;

// 집계 저장소 모듈을 담는 네임스페이스다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.

// Totalmade의 실제 세 256DWORD만 보관한다. 목록·현재 소유자·로딩/삭제 정책과 수명을 분리한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
public sealed class RawTotalMadeStorage
{
    /// <summary>원본 rep stosd가 초기화하는 DWORD 수다. 256번째 물리 칸은 보존한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public const int InitializedEntries = 255;

    /// <summary>실제 표 한 개의 물리 칸 수다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public const int TableSize = 256;

    /// <summary>현재 로컬 수다. 최초 BSS 0이다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint[] LocalCounts { get; } = new uint[TableSize];

    /// <summary>누적 로컬 생산 수이며 일반 삭제에서 줄지 않는다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint[] LocalSecondaryCounts { get; } = new uint[TableSize];

    /// <summary>현재 전역 수이며 모든 타입의 256번째 물리 칸도 보관한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint[] GlobalCounts { get; } = new uint[TableSize];

    /// <summary>Session·장부·TYPE 없이 같은 현재 세 배열에 원문 부분 reset만 실행한다. 앞 255칸만 지우고 256번째 칸과 다른 상태에는 접근하지 않는다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    /// <param name="storage">초기화할 실제 저장소다.</param>
    public static void ResetCounts(RawTotalMadeStorage storage)
    {
        ArgumentNullException.ThrowIfNull(storage);
        // 앞 255칸만 0으로 지운다
        for (int index = 0; index < InitializedEntries; ++index)
        {
            storage.LocalCounts[index] = 0u;
            storage.LocalSecondaryCounts[index] = 0u;
            storage.GlobalCounts[index] = 0u;
        }
    }
}
