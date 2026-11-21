// Dais 일반/전체 writer와 현재 버퍼/송신의 같은 선택 상태를 기존 파생 체인에 결합한다.
namespace Netstorm.Core.Original;

/// <summary>RawCampaignSession.cpp DaisTransmission의 실제 함수 값과 마지막 writer 분배다.</summary>
public static class RawDaisTransmission
{
    /// <summary>선택된 실제 파생 경계만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Dais 전송의 선택된 실제 경계 미연결");
    /// <summary>같은 장기 헤더/버퍼를 읽으며 다른 프로세스는 기존 함수 사본으로 넘긴다. 생성 시 외부 효과를 실행하지 않는다.</summary>
    public static ProcessFormTransmission Make(RawGameAuxState aux, Action<int> transmit, ProcessFormTransmission? other = null)
    {
        ArgumentNullException.ThrowIfNull(aux); ArgumentNullException.ThrowIfNull(transmit); ProcessFormTransmission fallback = other?.Clone() ?? new(); bool writingDais = false;
        // 실제 작성 위치에서 현재 두 헤더 BYTE와 같은 공통 버퍼를 고른다.
        int Write(DaisProcess process, bool changed, bool force) => process.WritePacket(aux.SquidTransmission.Buffer, changed, force, aux.DaisProcessHeader[1], aux.DaisProcessHeader[0]);
        return new()
        {
            WriteData = (process, changed) => { writingDais = process is DaisProcess; return process is DaisProcess dais ? Write(dais, changed, false) : Need(fallback.WriteData)(process, changed); },
            WriteFullData = (process, force) => { writingDais = process is DaisProcess; return process is DaisProcess dais ? Write(dais, false, force != 0) : Need(fallback.WriteFullData)(process, force); },
            Packet = size =>
            {
                if (!writingDais) return Need(fallback.Packet)(size); byte[] bytes = aux.SquidTransmission.Buffer;
                if (size < 0 || size > bytes.Length) throw new ArgumentOutOfRangeException(nameof(size), "DaisProcess의 실제 작성 버퍼 범위"); return bytes.AsSpan(0, size).ToArray();
            },
            Transmit = size => { if (writingDais) transmit(size); else Need(fallback.Transmit)(size); },
        };
    }
}
