// 썬 펜스의 실제 표식·포스 필드 생성과 재생성 실제 생성 결선을 같은 장기 풀에 연결한다.
// cpppj/src/client/RawSurfaceFence.cpp 의 FenceMarkers/FenceShields 장기 본문을 이식한다.
// 0.591에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 생성)
namespace Netstorm.Core.Original;

/// <summary>World 없이 같은 장기 Pool/TYPE으로 펜스 생성을 보유한다. 0.591에서 추가됐다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 풀·TYPE·표식 역할·이웃 중심을 공유하는 표식 생성 본체다. 0.591에서 추가됐다.</summary>
    private RawFenceMarkerCreate? _fenceMarkerCreate;

    /// <summary>같은 장기 풀·프레임·포스 필드 역할을 공유하는 포스 필드 생성 본체다. 0.591에서 추가됐다.</summary>
    private RawFenceShieldCreate? _fenceShieldCreate;

    /// <summary>같은 장기 표식 타입·이웃 중심·실제 생성 장치를 공유하는 본체다. 0.591에서 추가됐다.</summary>
    public RawFenceMarkerCreate FenceMarkers()
    {
        if (_fenceMarkerCreate is null)
        {
            var hooks = new FenceCreateHooks
            {
                Create = (type, flags) => CreateAsset(type, flags),
                NotifySurface = sid => BuildingExecutor().NotifySurface(sid),
                SetOwner = (sid, player) => SetAssetOwner(sid, player),
                Pop = (sid, x, y, flags) => PopAsset(sid, x, y, flags),
            };
            _fenceMarkerCreate = new RawFenceMarkerCreate(
                Pool,
                GameAux().FenceMarkerTypeBox,
                hooks,
                sid => FenceNeighbors().Center(sid));
        }

        return _fenceMarkerCreate;
    }

    /// <summary>같은 장기 포스 필드 타입·프레임·실제 생성 장치를 공유하는 본체다. 0.591에서 추가됐다.</summary>
    public RawFenceShieldCreate FenceShields()
    {
        if (_fenceShieldCreate is null)
        {
            var hooks = new FenceCreateHooks
            {
                Create = (type, flags) => CreateAsset(type, flags),
                SetOwner = (sid, player) => SetAssetOwner(sid, player),
                Pop = (sid, x, y, flags) => PopAsset(sid, x, y, flags),
            };
            _fenceShieldCreate = new RawFenceShieldCreate(
                Pool,
                Frames,
                GameAux().FenceShieldTypeBox,
                hooks);
        }

        return _fenceShieldCreate;
    }

    /// <summary>
    /// 재생성 생성 장치 묶음을 같은 저장소의 실제 표식·포스 필드 생성 본체로 채운다.
    /// 재생성 본체 생성 전 한 번만 호출한다. 0.591에서 추가됐다. (R4-1 재생성 실제 생성 결선)
    /// </summary>
    public void BindFenceRebuildActualCreations()
    {
        // Native 전송 뒤·재생성 본체/대여 전에만 실제 본체를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _fenceRebuild is not null
            || _fenceRebuildDevices.Markers is not null || _fenceRebuildDevices.Shields is not null)
        {
            throw new ArgumentException("장기 펜스 재생성의 실제 생성 결선 중복/누락");
        }

        _fenceRebuildDevices = new RawSurfaceFenceRebuildDevices
        {
            Markers = (sid, other) => FenceMarkers().Create(sid, other),
            Shields = (points, first, second, parent, other) => FenceShields().Create(points, first, second, parent, other),
        };
    }
}
