using Netstorm.Core.Original;

// 건설0x37의 관찰/버전/수신을 같은 실제 패킷 등록표에 설치한다.
namespace Netstorm.Net;

/// <summary>cpppj/src/o/RawPacketDispatch.cpp의 RegisterConstructionData 자유 함수를 제공한다.</summary>
public static class RawConstructionDataRegistration
{
    /// <summary>관찰기를 지우고 버전1/불일치0과 실제 건설 수신을 등록한다. 길이/송신자/연결 효과를 추가하지 않는다.</summary>
    public static void RegisterConstructionData(this RawPacketDispatch dispatch, RawConstructionData data)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(data);
        dispatch.Observe(0x37, null); dispatch.Version(0x37, 1, 0);
        dispatch.Register(0x37, (_, _, packet) => data.Receive(packet));
    }
}
