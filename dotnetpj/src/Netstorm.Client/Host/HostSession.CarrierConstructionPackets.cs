// 고정 Carrier/건설 F가 호출 시점의 실제 Session 본문을 직접 선택하는 실제 Host 부분이다.
// cpppj/src/client/NativeCarrierPacketReceivers.cpp 와 NativeConstructionPacketReceivers.cpp 의
// ReceiveFixedCarrierPacket/ReceiveFixedConstructionPacket 호출 자리를 이식한다.
// 각 본체는 월드·RU-5 후속에서 잇는다. 0.963에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client.Host;

/// <summary>고정F가 같은 Carrier/건설 본체를 직접 선택하는 실제 Host 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>저장된 고정 F값으로 같은 Carrier 본문만 호출한다. sender/현재 packet 종류/등록표로 재선택하지 않는다.</summary>
    /// <param name="entry">원문 불변 Carrier 수신 코드다.</param>
    /// <param name="sender">원문 송신 구독자 번호다. 본체 이식 전까지 보관만 한다.</param>
    /// <param name="packet">같은 현재 패킷 물리다. 본체 이식 전까지 읽지 않는다.</param>
    public void ReceiveFixedCarrierPacket(uint entry, uint sender, ReadOnlyMemory<byte> packet)
    {
        // 본체가 없어 World 수명을 건드리지 않으므로 수명 가드는 각 본체 이식 때 둔다.
        _ = sender; _ = packet;
        // 검토한 15행만 받으며 각 본체는 후속에서 잇는다.
        switch (entry)
        {
            case 0x0048f7f0:
            case 0x0048f8f0:
            case 0x004900e0:
            case 0x00428570:
            case 0x00426520:
            case 0x00444bc0:
            case 0x004267c0:
            case 0x0048bfe0:
            case 0x0041e860:
            case 0x00420bf0:
            case 0x0041e320:
            case 0x00448e90:
            case 0x00493f80:
            case 0x004205f0:
            case 0x004207a0:
                throw new InvalidOperationException($"고정 Carrier 수신 본체 미연결(0x{entry:x})");
            default: throw new ArgumentOutOfRangeException(nameof(entry), "미등록 고정 Carrier 실제 본문");
        }
    }

    /// <summary>저장된 고정 F값으로 같은 건설/연구 본문만 호출한다. sender/현재 packet 종류/등록표로 재선택하지 않는다.</summary>
    /// <param name="entry">원문 불변 건설/연구 수신 코드다.</param>
    /// <param name="sender">원문 송신 구독자 번호다. 본체 이식 전까지 보관만 한다.</param>
    /// <param name="packet">같은 현재 패킷 물리다. 본체 이식 전까지 읽지 않는다.</param>
    public void ReceiveFixedConstructionPacket(uint entry, uint sender, ReadOnlyMemory<byte> packet)
    {
        // 본체가 없어 World 수명을 건드리지 않으므로 수명 가드는 각 본체 이식 때 둔다.
        _ = sender; _ = packet;
        // 검토한 8행만 받으며 각 본체는 후속에서 잇는다.
        switch (entry)
        {
            case 0x00444af0:
            case 0x004441b0:
            case 0x004427b0:
            case 0x00453d70:
            case 0x004436f0:
            case 0x004dc050:
            case 0x004332c0:
            case 0x004d56f0:
                throw new InvalidOperationException($"고정 건설/연구 수신 본체 미연결(0x{entry:x})");
            default: throw new ArgumentOutOfRangeException(nameof(entry), "미등록 고정 건설/연구 실제 본문");
        }
    }
}
