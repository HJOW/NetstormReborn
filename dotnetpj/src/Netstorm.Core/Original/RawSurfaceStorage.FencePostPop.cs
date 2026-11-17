// 썬 펜스의 실제 후처리·정기 처리를 같은 장기 풀·이웃·프레임에 연결한다.
// cpppj/src/client/RawSurfaceFence.cpp 의 FencePostPop/FenceRegular 장기 본문을 이식한다.
// 0.592에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 후처리·정기 처리)
namespace Netstorm.Core.Original;

/// <summary>장기 펜스 정기 처리의 실제 Damageable Handle 장치를 묶는다. 0.592에서 추가됐다.</summary>
public sealed class RawSurfaceFenceRegularDevices
{
    /// <summary>차단 bit20 또는 다른 사건은 실제 Damageable Handle로 넘긴다. 0.592에서 추가됐다.</summary>
    public RegularHandler? Damageable { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.592에서 추가됐다.</summary>
    internal RawSurfaceFenceRegularDevices Copy() => new()
    {
        Damageable = Damageable,
    };
}

/// <summary>World 없이 같은 장기 Pool로 펜스 후처리·정기 처리를 보유한다. 0.592에서 추가됐다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 이웃/연결선/장부를 공유하는 후처리 본체다. 0.592에서 추가됐다.</summary>
    private RawFencePostPop? _fencePostPop;

    /// <summary>같은 장기 이웃·프레임·지정기를 공유하는 정기 처리 본체다. 0.592에서 추가됐다.</summary>
    private RawFenceRegular? _fenceRegular;

    /// <summary>같은 장기 Damageable Handle 장치를 본체보다 오래 유지한다. 0.592에서 추가됐다.</summary>
    private RawSurfaceFenceRegularDevices _fenceRegularDevices = new();

    /// <summary>같은 장기 이웃/연결선/장부와 실제 하위 소비자를 사용하는 후처리 본체다. 0.592에서 추가됐다.</summary>
    public RawFencePostPop FencePostPop()
    {
        if (_fencePostPop is null)
        {
            var hooks = new FencePostPopHooks
            {
                Theme = sid => unchecked((uint)Types[(int)Pool.Slot(sid)[10]].Theme),
                Neighbor = (sid, direction) => FenceNeighbors().Neighbor(sid, direction),
                ReserveRegular = () => AllocateResourceRegular(),
                AddRegular = (sid, @event, payload) => ConstructResourceRegular(sid, @event, payload),
                ClearLine = points => FenceLine().Clear(points),
                Connections = sid => FenceNeighbors().Connections(sid),
                ValidateBase = (sid, flags, projected) => PostPopExecutor().ValidateBase(sid, flags, in projected),
                Damageable = (sid, flags) => ResourceCarrierPostPop().DamageablePostPop(sid, flags),
            };
            _fencePostPop = new RawFencePostPop(Pool, hooks);
        }

        return _fencePostPop;
    }

    /// <summary>첫 실제 정기 처리 전에 실제 Damageable Handle 장치만 연결한다. 0.592에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 정기 처리 장치 묶음이다.</param>
    public void BindFenceRegularDevices(RawSurfaceFenceRegularDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 뒤, 첫 본체/대여 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _fenceRegular is not null
            || _fenceRegularDevices.Damageable is not null || devices.Damageable is null)
        {
            throw new ArgumentException("장기 펜스 정기 처리의 실제 최초 장치 중복/누락", nameof(devices));
        }

        _fenceRegularDevices = devices.Copy();
    }

    /// <summary>같은 장기 이웃·프레임·지정기와 실제 연결 소비자를 사용하는 정기 처리 본체다. 0.592에서 추가됐다.</summary>
    public RawFenceRegular FenceRegular()
    {
        if (_fenceRegular is null)
        {
            var hooks = new FenceRegularHooks
            {
                Neighbor = (sid, direction) => FenceNeighbors().Neighbor(sid, direction),
                ClearMarkers = (sid, other) => FenceMarkerClear().Clear(sid, other),
                ClearLine = points => FenceLine().Clear(points),
                Connections = sid => FenceNeighbors().Connections(sid),
                ReserveRegular = () => AllocateResourceRegular(),
                AddRegular = (sid, @event, payload) => ConstructResourceRegular(sid, @event, payload),
                RefreshNeighbors = sid => FenceLifetime().Refresh(sid),
                Rebuild = sid => FenceRebuild().Rebuild(sid),
                Damageable = (_fenceRegularDevices.Damageable ?? throw new InvalidOperationException("장기 펜스 정기 처리 실제 Damageable 미연결")),
            };
            _fenceRegular = new RawFenceRegular(Pool, Frames, FrameExecutor(), hooks);
        }

        return _fenceRegular;
    }
}
