// 한 월드가 빌리는 현재 삭제·사건·전송 처리기와 직렬화·송신 경계를 공유한다.
namespace Netstorm.Core.Original;

// 프로세스 호스트 묶음 모듈을 담는 네임스페이스다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.

// 파생 WriteData→공유 버퍼 송신 경계다. 버퍼 자체는 직렬화기가 소유한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.
public sealed class ProcessFormTransmission
{
    /// <summary>전송할 Form 데이터를 버퍼에 작성하는 콜백이다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public Func<SquidProcess, bool, int>? WriteData { get; set; }

    /// <summary>작성한 패킷이나 Form 데이터를 송신하는 콜백이다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public Action<int>? Transmit { get; set; }

    /// <summary>마지막 writer가 작성한 같은 버퍼를 빌린다. 대상 지정 전송은 방송 콜백을 실행하지 않고 이 바이트만 복사한다. 원문 span 반환을 배열 복사로 옮긴다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public Func<int, byte[]>? Packet { get; set; }

    /// <summary>ProcessForm+54의 실제 (buffer,부모변경0,force인자)를 전달한다. 평상시 부착 writer와 구별한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public Func<SquidProcess, uint, int>? WriteFullData { get; set; }

    /// <summary>현재 전송 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    /// <returns>복사된 전송 묶음이다.</returns>
    public ProcessFormTransmission Clone() => new()
    {
        WriteData = WriteData,
        Transmit = Transmit,
        Packet = Packet,
        WriteFullData = WriteFullData,
    };
}

// 한 월드가 빌리는 현재 삭제·사건·전송 처리기이며 Host 자체의 장기 수명과 구별한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.
public sealed class ProcessHostRuntimeBindings
{
    /// <summary>현재 월드의 실제 공통 삭제 몸체다. Pool 정체성은 대여 시 검증한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public RawSquidDestroy? Destroy { get; set; }

    /// <summary>현재 부모의 가상 Regular 사건이며 이전 월드 콜백을 Host에 남기지 않는다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public RegularHandler? Regular { get; set; }

    /// <summary>같은 현재 가상 분배기의 자산 선택·삭제·공간 효과다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public SquidDestroyHooks? Asset { get; set; }

    /// <summary>현재 실제 Form 직렬화와 송신 경계다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public ProcessFormTransmission? Transmit { get; set; }

    /// <summary>선택된 Host 효과의 대여 수이며 진행 중 반납을 거부한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public ulong ActiveCalls { get; set; }

    /// <summary>실제 현재 월드의 공통 Pop 깊이를 생성 위치에서 읽는다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public Func<uint>? PostPopDepth { get; set; }

    /// <summary>공유 Regular의 검사 무관 진단을 같은 Client로 전달한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>현재 월드의 같은 Pool 공장이다. 장기 Host 자체에 이 주소를 저장하지 않는다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    public RawSquidFactory? Factory { get; set; }

    /// <summary>현재 런타임 묶음의 얕은 사본을 만든다. 개수 값은 복사하고 참조·대리자는 공유한다. 2026-11-07 F2d 호스트 묶음 이식에서 추가됐다.</summary>
    /// <returns>복사된 런타임 묶음이다.</returns>
    public ProcessHostRuntimeBindings Clone() => new()
    {
        Destroy = Destroy,
        Regular = Regular,
        Asset = Asset,
        Transmit = Transmit,
        ActiveCalls = ActiveCalls,
        PostPopDepth = PostPopDepth,
        Assertion = Assertion,
        Factory = Factory,
    };
}
