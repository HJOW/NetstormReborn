// Native 기본 Host가 실제 파생 writer와 현재 공통 예약을 소비하도록 연결한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/client/RawSurfaceStorage.cpp의 WriteDefaultForm/SerializeProcessForm 중 이식된 파생을 연결한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>현재 같은2048BYTE 버퍼에 작성하며 반환 길이를 바꾸거나 자체 송신하지 않는다.</summary>
    public int WriteDefaultForm(SquidProcess process, bool parentChanged, uint force) =>
        SerializeProcessForm(process, GameAux().SquidTransmission.Buffer, parentChanged, force);

    /// <summary>실제 파생 클래스로 선택하고 현재 헤더/식별자/미작성 tail을 보존한다. 미이식 파생은 실패한다.</summary>
    public int SerializeProcessForm(SquidProcess process, Memory<byte> buffer, bool parentChanged, uint force)
    {
        if (process is DaisProcess dais)
        {
            byte[] header = GameAux().DaisProcessHeader;
            return dais.WritePacket(buffer.Span, parentChanged, force != 0, header[1], header[0]);
        }
        if (process is ConstructionProcess construction)
        {
            byte[] header = GameAux().ConstructionHeader;
            return RawConstructionData.WriteProcess(construction, buffer.Span, parentChanged, force != 0, new() { Protocol = header[1], Message = header[0] });
        }
        if (process is GunProcess gun)
        {
            byte[] header = GameAux().GunProcessHeader;
            return gun.WritePacket(buffer.Span, parentChanged, force != 0, header[1], header[0]);
        }
        if (process is FlyerProcess flyer)
        {
            byte[] header = GameAux().FlyerProcessHeader;
            return flyer.WritePacket(buffer.Span, parentChanged, force != 0, header[1], header[0]);
        }
        if (process is SunAviaryProcess aviary)
        {
            byte[] header = GameAux().SunAviaryHeader;
            return aviary.WritePacket(buffer.Span, parentChanged, force != 0, header[1], header[0]);
        }
        if (process is RawPathProcess path) return SerializePathProcess(path, buffer);
        throw new InvalidOperationException("선택된 ProcessForm의 실제 파생 writer 미연결");
    }

    /// <summary>원문 prefix span에 해당하는 현재 바이트 사본만 반환한다. Packet API의 기존 배열 반환 계약을 유지한다.</summary>
    private byte[] CurrentProcessFormPacket(int length)
    {
        byte[] buffer = GameAux().SquidTransmission.Buffer;
        if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "장기 Form의 실제 작성 크기 범위");
        return buffer.AsSpan(0, length).ToArray();
    }
}
