using Netstorm.Core.Original;

// 원문 커서의 나머지 수명/측정/고정 배열을 같은 주소 세대에 보관한다.
namespace Netstorm.Client.Input;

/// <summary>내려놓기 상태와 겹치지 않는 RawCursorLifecycle.h의 현재 전역이다.</summary>
public sealed class CursorLifecycleState
{
    /// <summary>55a280의 TYPE 포인터를 현재188TYPE의 논리 번호로 표현한다.</summary>
    public uint AbstractType { get; set; }
    /// <summary>55a2a0의 별도 방향 인자다.</summary>
    public uint DirectionArgument { get; set; }
    /// <summary>55a278의 현재 Gump이며 마나 프로세스55a274와 다르다.</summary>
    public uint CurrentGump { get; set; }
    /// <summary>55a1d8의 동일120BYTE/30DWORD 배열이다. WORD 배열로 줄이지 않는다.</summary>
    public uint[] Previews { get; } = new uint[30];
    /// <summary>55a1b8의 현재 signed 개수다.</summary>
    public int PreviewCount { get; set; }
    /// <summary>경고 문맥에 넣는 현재 글꼴 번호다.</summary>
    public uint WarningFont { get; set; }
    /// <summary>비용 문맥에 넣는 현재 글꼴 번호다.</summary>
    public uint CostFont { get; set; }
    /// <summary>원문 최초92인 비용 그림 TYPE다.</summary>
    public uint CostType { get; set; } = 92;
    /// <summary>명시 독립 raw 입력의 현재 진단값이며 실제 Client는 Checking을 제공한다.</summary>
    public uint TeleportBlocked { get; set; }
    /// <summary>실제 Client의 현재 진단 DWORD를 소비 때마다 조회한다.</summary>
    public RawCheckingFlag? Checking { get; set; }
    /// <summary>경고 측정의 동일8DWORD 문맥이다. +8만 글꼴로 바뀐다.</summary>
    public uint[] WarningContext { get; } = [2, 0, 0, 0, 0, 0, 1, 1];
    /// <summary>비용 측정의 동일8DWORD 문맥이다.</summary>
    public uint[] CostContext { get; } = [2, 3, 0, 0, 0, 0, 1, 1];
    /// <summary>다섯 경고의 현재 개별 signed 폭이다.</summary>
    public int[] WarningWidths { get; } = new int[5];
    /// <summary>다섯 경고의 현재 개별 signed 높이다.</summary>
    public int[] WarningHeights { get; } = new int[5];
    /// <summary>현재 최대 경고 폭이다.</summary>
    public int WarningWidth { get; set; }
    /// <summary>현재 최대 경고 높이다.</summary>
    public int WarningHeight { get; set; }
    /// <summary>현재 비용 글자의 측정 폭이다.</summary>
    public int CostTextWidth { get; set; }
    /// <summary>비용 글자/그림 중 최종 signed 높이다.</summary>
    public int CostHeight { get; set; }
    /// <summary>비용 그림3의 signed WORD hotspot X다.</summary>
    public int CostHotspotX { get; set; }
    /// <summary>비용 그림3의 signed WORD hotspot Y다.</summary>
    public int CostHotspotY { get; set; }
    /// <summary>비용 그림3의 별도 signed 높이다.</summary>
    public int ImageHeight { get; set; }
    /// <summary>같은 SHP 저장소 블록의 호스트 식별값이다.</summary>
    public uint CostImage { get; set; }
    /// <summary>55a1bc에 대응하는 현재 절삭 비용 문자열이다.</summary>
    public string CostText { get; set; } = "";
    /// <summary>선택한 실제 진단 getter 또는 독립 입력의 현재 값을 읽는다.</summary>
    public uint ReadChecking() => Checking is { } current ? current.Read() : TeleportBlocked;

    /// <summary>고정 객체/배열을 교체하지 않고 struct{}의 최초 값으로 되돌린다.</summary>
    public void Reset()
    {
        AbstractType = DirectionArgument = CurrentGump = WarningFont = CostFont = TeleportBlocked = CostImage = 0; CostType = 92; PreviewCount = 0;
        WarningWidth = WarningHeight = CostTextWidth = CostHeight = CostHotspotX = CostHotspotY = ImageHeight = 0; CostText = ""; Checking = null;
        Array.Clear(Previews); Array.Clear(WarningWidths); Array.Clear(WarningHeights); Array.Clear(WarningContext); Array.Clear(CostContext);
        WarningContext[0] = CostContext[0] = 2; CostContext[1] = 3; WarningContext[6] = WarningContext[7] = CostContext[6] = CostContext[7] = 1;
    }
}

/// <summary>비용 그림의 식별값과 원문 네 signed WORD다.</summary>
/// <param name="Token">같은 SHP 블록 토큰이다.</param>
/// <param name="Width">원문+24의 signed 폭이다.</param>
/// <param name="Height">원문+26의 signed 높이다.</param>
/// <param name="HotspotX">원문+28의 signed X다.</param>
/// <param name="HotspotY">원문+30의 signed Y다.</param>
public readonly record struct CursorCostImage(uint Token, short Width, short Height, short HotspotX, short HotspotY);
