// 장기 저장소의 실제 Path 파생 writer를 현재 원문 헤더·버퍼·진단에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj RawSurfaceStorage.SerializeProcessForm의 Path 분기다. Native 기본 writer와 기존 세션 체인이 같은 본체를 사용한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 실제 경로의 Native 본문만 쓴다. Form/Parent/count 선행 자격 검사나 자체 송신을 추가하지 않는다.</summary>
    public int SerializePathProcess(RawPathProcess process, Memory<byte> buffer)
    {
        if (!ReferenceEquals(process.Pool, Pool)) throw new ArgumentException("장기 Path writer의 실제 Pool 불일치");
        return new RawPathData(Pool.Edition).WriteCurrent(process.Runtime, (ushort)process.Form(), (ushort)process.Parent(), GameAux().PathPacketHeader, buffer, new()
        {
            ParentGenus = parent => Types[Pool.Slot(parent)[10]].Flags2,
            Assertion = AssertDisplay, Trace = text => TraceGraph(GameAux().PathTraceLevel, text),
        });
    }
}
