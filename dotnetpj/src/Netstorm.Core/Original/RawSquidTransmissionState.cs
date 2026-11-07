// 공통 전송의 버퍼·헤더·동기화 상태를 세션·월드가 공유한다.
namespace Netstorm.Core.Original;

// 전송 상태 모듈을 담는 네임스페이스다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.

/// <summary>
/// 가변 패킷의 압축 헤더 값이다
/// (cpppj <c>o/RawSquidTransmission.h</c>의 <c>SquidCompressedPacketHeader</c>).
/// </summary>
/// <param name="Message">현재 메시지 바이트다.</param>
/// <param name="Protocol">현재 프로토콜 바이트다.</param>
/// <param name="Length">선언 길이 DWORD다. Word로 미리 자르지 않는다.</param>
public readonly record struct SquidCompressedPacketHeader(byte Message, byte Protocol, uint Length);

/// <summary>
/// 공통 전송의 버퍼·헤더·동기화 상태를 담는다. 초기값과 BSS 0을 보존하며 월드 대여·반납 때 초기화하지 않는다
/// (cpppj <c>o/RawSquidTransmission.h</c>의 <c>RawSquidTransmissionState</c>).
/// </summary>
/// <remarks>2026-11-07 F2d Host 전송 사전 구현에서 추가했다. 멀티플레이 사전 구현이며 검사는 3차 로컬 구현 때 잇는다.</remarks>
public sealed class RawSquidTransmissionState
{
    /// <summary>공통 송신 버퍼이며 반환 크기 뒤를 지우지 않는다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public byte[] Buffer { get; set; } = new byte[2048];

    /// <summary>기존 감시·내용물·삭제 writer의 현재 헤더다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public FormPacketHeaders Forms { get; set; } = new();

    /// <summary>삭제 writer의 한 번 소비 표식이다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public FormDeletionStamp Deletion { get; set; } = new();

    /// <summary>고정 선언 길이이며 35바이트 반환과 별개다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public FormPacketHeader Update { get; set; } = new(0x2c, 1, 27);

    /// <summary>선언 길이를 Word로 미리 자르지 않는다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public SquidCompressedPacketHeader Compressed { get; set; } = new(0x7a, 1, 0u);

    /// <summary>초기값이며 Config의 현재 값을 보존한다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public int Compress { get; set; } = 1;

    /// <summary>초기 BSS 0이며 재동기화 부모의 임시 허용 관문이다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public uint ClientSidAllowed { get; set; }

    /// <summary>같은 owner 모드 켜짐 값이다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public uint SameOwnerEnabled { get; set; }

    /// <summary>마지막 owner 값이다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public uint LastOwner { get; set; }

    /// <summary>누적 압축 횟수다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public uint CompressedCount { get; set; }

    /// <summary>누적 압축 바이트 수다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public uint CompressedBytes { get; set; }

    /// <summary>현재 동기화 대상이며 시계 동기화 token과 별개다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public uint SyncTarget { get; set; }

    /// <summary>공유 Regular 수신 이미지 헤더다. 2026-11-07 F2d Host 전송 사전 구현에서 추가됐다.</summary>
    public FormPacketHeader SharedRegular { get; set; } = new(0x7e, 1, 28);
}
