// 같은 Root의 실제 Frame을 기록 장치가 Client 종료까지 빌린다.
namespace Netstorm.Client.Host;

/// <summary>ClientMain의 장기 패킷 기록 소유자와 실제 현재 프레임 결선이다.</summary>
public sealed partial class ClientHostGame
{
    /// <summary>첫 조회 전에는 null이며 파일/요청 상태를 Session보다 오래 보관하는 실제 기록 장치다.</summary>
    private ClientPacketRecord? _packetRecord;
    /// <summary>첫 조회에서 같은 실제 프레임의 단일 owner를 만들고 전송/State가 공유한다. 조회에서 파일을 열지 않는다.</summary>
    public ClientPacketRecord PacketRecordOwner => _packetRecord ??= new ClientPacketRecord(_clientClock.Frame);
}
