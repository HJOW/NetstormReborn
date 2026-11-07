// World 실행기보다 오래 보관할 두 실제 배열과 표 초기화를 공유한다.
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

// 그래프 저장소 모듈을 담는 네임스페이스다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.

// World 실행기보다 오래 보관할 두 실제 배열이다. 원문의 별도 heap 두 개를 호스트의 한 연속 Native 객체로 가장하지 않는다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.
public sealed class RawGraphStorage
{
    /// <summary>00562c18의 255행 6바이트이며 reserved와 틈 3행을 포함한다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public GraphRecord[] Records { get; } = new GraphRecord[SurfaceGraph.TableSize];

    /// <summary>00562c20의 4096DWORD이며 이전 flood의 비활성 흔적도 남긴다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    public uint[] Stack { get; } = new uint[SurfaceGraph.FloodSize];

    /// <summary>같은 실제 월드가 첫 저장 객체를 설치하기 전에 호출하는 원본 표 초기화다. 앞 251개 두 WORD만 지우고 reserved·나머지 행은 보존한다. 시작의 최초 할당·전체 초기화나 stack의 lazy 확보는 포함하지 않는다. 2026-11-07 F2d 저장소 물리 자료형 이식에서 추가됐다.</summary>
    /// <param name="storage">초기화할 실제 저장소다.</param>
    public static void ResetTable(RawGraphStorage storage)
    {
        ArgumentNullException.ThrowIfNull(storage);
        // 앞 251개의 두 WORD만 지우고 reserved·나머지 행은 보존한다
        for (int index = 0; index < SurfaceGraph.Count; ++index)
        {
            storage.Records[index].Surfaces = 0;
            storage.Records[index].InUse = 0;
        }

        storage.Records[0].InUse = 1;
        // 예약 WORD는 하위 바이트만 1로 바꾸고 윗바이트는 보존한다
        storage.Records[0].Reserved = unchecked((short)(ushort)(((ushort)storage.Records[0].Reserved & 0xFF00u) | 1u));
        storage.Records[SurfaceGraph.Invalid].Surfaces = 0x7DFD;
        storage.Records[SurfaceGraph.Invalid].InUse = 0x7DFD;
    }
}
