// 같은 Client 수명의 미션 설정/버전 응답 생성 필드를 소유한다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/o/RawGameAuxState.h의 네트워크 공용 장기 저장소다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>0052edce/cf·00544c50의 같은 장기 Path 헤더다. World 교체/Reset에서 새로 만들지 않는다.</summary>
    public RawPathDataHeader PathPacketHeader { get; } = new();
    /// <summary>00540c68의 현재 경로 로그 수준이다. 최초99이며 월드/세션 교체나 일반 Reset으로 바꾸지 않는다.</summary>
    public int PathTraceLevel { get; set; } = 99;
    /// <summary>00541158의 최초58이며 신규 DaisProcess 부착에서 읽는 같은 장기 DWORD다.</summary>
    public UIntBox DaisProcessTypeBox { get; } = new() { Value = 58 };
    /// <summary>00532690/91의 현재 메시지6a/프로토콜1 BYTE다. 일반 준비/World 교체에서 초기화하지 않는다.</summary>
    public byte[] DaisProcessHeader { get; } = [0x6a, 1];
    /// <summary>0054d4d8의 최초BSS0이며 객체 수신과 검증 시작/종료가 같은 현재DWORD를 쓴다.</summary>
    public UIntBox VerifySidBox { get; } = new();
    /// <summary>0054d4dc의 별도BSS0이며 검증 간격의 현재 순회 위치다.</summary>
    public UIntBox PreviousVerifySidBox { get; } = new();
    /// <summary>0054d4e4의 최초BSS0이며 Take 전 TYPE와 정상 수신 종료0을 공유한다.</summary>
    public UIntBox SupportTypeWasBox { get; } = new();
    /// <summary>005451f8의 최초BSS0인 NUL 종료 기술 사용 목록이다. 월드 교체로 재생성하지 않는다.</summary>
    public byte[] TypeUse { get; } = new byte[256];
    /// <summary>32h의 실제7DWORD 초기값이다. Session/World 교체에서 다시 만들거나 초기화하지 않는다.</summary>
    public uint[] MissionSettings { get; } = [1, 0, 0, 0, 0, 1, 8];
    /// <summary>005326d8의 최초3이며 개발자 scatter가 같은 장기 행을 갱신한다.</summary>
    public int DeveloperScatterRow { get; set; } = 3;
    /// <summary>0054288c의 최초02000065이며 개발자 cr이 현재 입력 큐 사건으로 읽는다.</summary>
    public uint DeveloperCombatResizeCommand { get; set; } = 0x02000065;
    /// <summary>0052ed54/55의 현재 메시지/프로토콜 BYTE다. 보호10.78의 최초0c/1을 한 번 구성한다.</summary>
    public byte[] Packet0cHeader { get; } = [0x0c, 1];
    /// <summary>00544c24의 최초BSS WORD0이다. 생성 시의 선언 길이이며 송신 observer의 최종 길이와 구별한다.</summary>
    public ushort Packet0cInitialLength { get; set; }
}
