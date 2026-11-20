// 최근 배치와 저장 카메라는 같은 Point 값형을 사용하되 서로 다른 배열을 소유한다.
namespace Netstorm.Core.Original;

/// <summary>단정도 x/y의 원문8BYTE 값이며 NaN/무효값의 비트도 복사한다.</summary>
/// <param name="X">현재 원문 X float다.</param>
/// <param name="Y">현재 원문 Y float다.</param>
public record struct ConstructionNoticePoint(float X, float Y);

/// <summary>00558ea4의40BYTE 최근 배치와 별도00558e78 Tab 순번이다.</summary>
public sealed class ConstructionHistoryState
{
    /// <summary>최신 배치부터 다섯 좌표며 명시 초기화 전에는 BSS0이다.</summary>
    public ConstructionNoticePoint[] Points { get; } = new ConstructionNoticePoint[5];
    /// <summary>현재 Tab 순번이며 Session/Renderer 교체로 되감지 않는다.</summary>
    public uint Index { get; set; }
}
