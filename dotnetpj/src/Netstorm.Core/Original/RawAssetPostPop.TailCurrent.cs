// 실제 가상 표가 선택한 전용 몸체와 바리케이드 현재 부모의 도달 위치를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>나머지 전용 후처리의 명시 분배와 현재 바리케이드 서버/로딩 관문이다.</summary>
public sealed partial class RawAssetPostPop
{
    /// <summary>이미 선택한 실제 전체 전용 몸체만 호출하고 누락을 공통 부모 성공으로 대신하지 않는다.</summary>
    private void DedicatedPostPop(int sid, uint flags, uint kind)
    {
        switch (kind)
        {
            case SunAviaryKind: AssetNeed(_hooks.SunAviaryPostPop)(sid, flags); break;
            case RainAviaryKind: AssetNeed(_hooks.RainAviaryPostPop)(sid, flags); break;
            case WindAviaryKind: AssetNeed(_hooks.WindAviaryPostPop)(sid, flags); break;
            case SunFenceKind: AssetNeed(_hooks.SunFencePostPop)(sid, flags); break;
            case NuggetKind: AssetNeed(_hooks.NuggetPostPop)(sid, flags); break;
            case GrowingIceKind: AssetNeed(_hooks.GrowingIcePostPop)(sid, flags); break;
            default: throw new InvalidOperationException("선택한 자산 전용 후처리 종류 오류");
        }
    }
    /// <summary>현재 서버/최초/비로딩만 통지 관문이며 extra를 추가 조건으로 쓰지 않는다.</summary>
    private void BlockerCurrent(int sid, uint flags)
    {
        if (_state.CurrentServer && (flags & 1) != 0 && _state.LoadingDepth == 0) AssetNeed(_hooks.StartBlocker)(sid);
        _damageable.DamageablePostPop(sid, flags);
    }
}
