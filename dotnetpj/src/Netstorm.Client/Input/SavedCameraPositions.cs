using Netstorm.Core.Original;

// 숫자 키의10칸은 Tab 최근 배치5칸과 독립인 장기 전역이다.
namespace Netstorm.Client.Input;

/// <summary>005caeb0의80BYTE 고정 데이터다. 키 처리와 명시 초기화는 별도다.</summary>
public sealed class SavedCameraPositions
{
    /// <summary>최초 BSS0이며 World/Renderer 교체로 초기화하지 않는다.</summary>
    public ConstructionNoticePoint[] Points { get; } = new ConstructionNoticePoint[10];
}
