// State와 중앙섬 배치가 직접 공유하는 실제 보조 전역이다.
namespace Netstorm.Core.Original;

/// <summary>일반 대여나 미션 교체가 값을 초기화하지 않는 Native 저장소다.</summary>
public sealed partial class RawGameAuxState
{
    /// <summary>00568afc/00568b00의 현재 중앙 X/Y float 두 개다.</summary>
    public float[] CentralPosition { get; } = new float[2];
    /// <summary>00595124의 현재 비전투 SP 기한 준비 DWORD다.</summary>
    public uint IncomePrimed { get; set; }
}
