// 썬 펜스의 실제 연결선·표식 정리를 같은 장기 풀·해시·TYPE·프레임·역할에 연결한다.
// cpppj/src/client/RawSurfaceFence.cpp 의 FenceLine/FenceMarkerClear 장기 본문을 이식한다.
// 0.589에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 정리 효과)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 Pool/Hash/TYPE으로 펜스 정리 효과를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 풀·해시·TYPE·프레임·포스 필드 역할을 공유하는 연결선 본체다. 0.589에서 추가됐다.</summary>
    private RawFenceLine? _fenceLine;

    /// <summary>같은 장기 풀·해시·TYPE·표식 역할·이웃 중심을 공유하는 표식 정리 본체다. 0.589에서 추가됐다.</summary>
    private RawFenceMarkerClear? _fenceMarkerClear;

    /// <summary>같은 장기 연결선 타입·프레임·가상 삭제를 공유하는 본체다. 0.589에서 추가됐다.</summary>
    public RawFenceLine FenceLine()
    {
        if (_fenceLine is null)
        {
            _fenceLine = new RawFenceLine(
                Pool,
                Hash,
                Types,
                Frames,
                GameAux().FenceShieldTypeBox,
                (sid, flags) => DestroyAsset(sid, flags));
        }

        return _fenceLine;
    }

    /// <summary>같은 장기 표식 타입·이웃 중심·가상 삭제를 공유하는 본체다. 0.589에서 추가됐다.</summary>
    public RawFenceMarkerClear FenceMarkerClear()
    {
        if (_fenceMarkerClear is null)
        {
            _fenceMarkerClear = new RawFenceMarkerClear(
                Pool,
                Hash,
                Types,
                GameAux().FenceMarkerTypeBox,
                (sid, flags) => DestroyAsset(sid, flags),
                sid => FenceNeighbors().Center(sid));
        }

        return _fenceMarkerClear;
    }
}
