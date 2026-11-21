// 같은 Client 수명의 미션 설정/버전 응답 생성 필드를 소유한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawGameAuxState.h의 네트워크 공용 장기 저장소다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>32h의 실제7DWORD 초기값이다. Session/World 교체에서 다시 만들거나 초기화하지 않는다.</summary>
    public uint[] MissionSettings { get; } = [1, 0, 0, 0, 0, 1, 8];
    /// <summary>0052ed54/55의 현재 메시지/프로토콜 BYTE다. 보호10.78의 최초0c/1을 한 번 구성한다.</summary>
    public byte[] Packet0cHeader { get; } = [0x0c, 1];
    /// <summary>00544c24의 최초BSS WORD0이다. 생성 시의 선언 길이이며 송신 observer의 최종 길이와 구별한다.</summary>
    public ushort Packet0cInitialLength { get; set; }
}
