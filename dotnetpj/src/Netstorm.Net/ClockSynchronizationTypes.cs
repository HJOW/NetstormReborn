using Netstorm.Core.Original;

// 같은 장기 Client 표본/시각 보정을 서버와 Commander가 직접 공유한다.
namespace Netstorm.Net;

/// <summary>원문 BSS 표본과 진단값이며 서로 다른 초기화 범위를 유지한다.</summary>
public sealed record ClockSynchronizationData
{
    /// <summary>005c8bf0의 현재 단방향 지연 밀리초다.</summary>
    public UIntBox LatencyMilliseconds { get; init; } = new();
    /// <summary>005c8bf8의 현재 송신 횟수다.</summary>
    public UIntBox Sent { get; init; } = new();
    /// <summary>005c8bfc의 현재 수신 횟수다.</summary>
    public UIntBox Received { get; init; } = new();
    /// <summary>005c8c08의 실제 벽시각 기한이며0은 예약 없음이다.</summary>
    public DoubleBox Next { get; init; } = new();
    /// <summary>005c8c10의70×32 실제 바이트다. 헤더/미작성4바이트도 표본 안에 보존한다.</summary>
    public Memory<byte> Samples { get; init; } = new byte[2240];
}

/// <summary>현재 시계와 이미 선택된 실제 연결의 가상 송신만 제공한다.</summary>
public sealed record ClockSynchronizationHooks
{
    /// <summary>00460cd0의 현재 게임 시각이다.</summary>
    public Func<double>? GameSeconds { get; init; }
    /// <summary>00460d70의 현재 벽시각이다.</summary>
    public Func<double>? WallSeconds { get; init; }
    /// <summary>고정 연결 token의 실제 가상+4다. Player/번호 관문을 다시 적용하지 않는다.</summary>
    public Action<uint, Memory<byte>>? Send { get; init; }
    /// <summary>수신 this의 실제 가상+4/+8이며 주소 snapshot은 선택된 호출 동안만 빌린다.</summary>
    public Action<PacketConnection, Memory<byte>, ReadOnlyMemory<byte>?>? Reply { get; init; }
}
