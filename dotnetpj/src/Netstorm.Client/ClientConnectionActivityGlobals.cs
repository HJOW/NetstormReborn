using Netstorm.Core.Original;
using Netstorm.Net;

// 활동 표와 그 사이 Zone DWORD를 같은 장기Client 수명에서 별개로 보유한다.
namespace Netstorm.Client;

/// <summary>22프레임/22벽시각과독립기한/Zone필드의실제소유자다.</summary>
public sealed class ClientConnectionActivityGlobals
{
    /// <summary>005c84e8부터 실제22프레임 DWORD이며 다른 필드는 clear에 포함하지 않는다.</summary>
    public uint[] Frames { get; } = new uint[22];
    /// <summary>005c8548부터 실제22벽시각 DWORD이며 signed reader도 같은 비트를 읽는다.</summary>
    public uint[] WallSeconds { get; } = new uint[22];
    /// <summary>005c8540의 별도 Zone 설치 stamp다.</summary>
    public UIntBox ActiveStamp { get; } = new();
    /// <summary>005c8544의 Zone+10 출력이며 wall 음수칸이 아니다.</summary>
    public UIntBox ActiveZoneWord { get; } = new();
    /// <summary>005c8620의 독립 BSS0 기한이다.</summary>
    public DoubleBox HeartbeatEnd { get; } = new();
    /// <summary>00531908의 실제 최초6이며 옵션 해석이1을 쓴다.</summary>
    public UIntBox AutoRestart { get; } = new() { Value = 6 };
    /// <summary>간극을 포함하지 않는 같은 두 실제표의 고정 뷰다.</summary>
    public ConnectionActivityTables Tables { get; }
    /// <summary>같은 실제표만 묶으며 시각/상태/Session을 읽거나 초기화하지 않는다.</summary>
    public ClientConnectionActivityGlobals() => Tables = new(Frames, WallSeconds);
}
