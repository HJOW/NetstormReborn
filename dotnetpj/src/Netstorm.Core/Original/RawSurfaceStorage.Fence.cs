// 썬 펜스의 전체 이웃·연결선·표식·재생성·수명을 같은 장기 물리와 Host에 연결한다.
// cpppj/src/client/RawSurfaceFence.cpp 의 BindFenceLifetimeDevices/FenceLifetime 장기 본문을 이식한다.
// 0.587에서 추가됐다. (R4-1 잔여 중 개별 삭제 몸체 Fence 우선)
namespace Netstorm.Core.Original;

/// <summary>장기 펜스의 실제 최초 이웃·정리·재생성 장치를 묶는다.</summary>
public sealed class RawSurfaceFenceLifetimeDevices
{
    /// <summary>동맹이면 같은 방향 이웃을, 아니면 비동맹 이웃을 찾는다. 0.587에서 추가됐다.</summary>
    public Func<int, uint, bool, int>? Neighbor { get; set; }

    /// <summary>실제 두 끝점 표식 삭제다. 0.587에서 추가됐다.</summary>
    public Action<int, int>? ClearMarkers { get; set; }

    /// <summary>현재 좌표/소유자/끝점의 연결선 정리다. 0.587에서 추가됐다.</summary>
    public Action<float[], uint, uint, int, int>? ClearLine { get; set; }

    /// <summary>현재 동맹/사거리/연결 재생성이다. 0.587에서 추가됐다.</summary>
    public Action<int>? Rebuild { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.587에서 추가됐다.</summary>
    internal RawSurfaceFenceLifetimeDevices Copy() => new()
    {
        Neighbor = Neighbor,
        ClearMarkers = ClearMarkers,
        ClearLine = ClearLine,
        Rebuild = Rebuild,
    };
}

/// <summary>World 없이 같은 장기 Pool로 펜스 수명을 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 이웃/정리/재생성 장치를 본체보다 오래 유지한다. 0.587에서 추가됐다.</summary>
    private RawSurfaceFenceLifetimeDevices _fenceLifetimeDevices = new();

    /// <summary>같은 장기 이웃/정리/두 Regular/부모를 공유하는 수명 본체다. 0.587에서 추가됐다.</summary>
    private RawFenceLifetime? _fenceLifetime;

    /// <summary>첫 실제 삭제 전에 실제 이웃·정리·재생성 장치만 연결한다. 0.587에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 펜스 장치 묶음이다.</param>
    public void BindFenceLifetimeDevices(RawSurfaceFenceLifetimeDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 뒤, 첫 본체/대여 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _fenceLifetime is not null
            || _fenceLifetimeDevices.Neighbor is not null
            || devices.Neighbor is null || devices.ClearMarkers is null || devices.ClearLine is null || devices.Rebuild is null)
        {
            throw new ArgumentException("장기 펜스의 실제 최초 장치 중복/누락", nameof(devices));
        }

        _fenceLifetimeDevices = devices.Copy();
    }

    /// <summary>같은 장기 이웃/정리/두 Regular/부모를 공유하는 수명 본체다. 0.587에서 추가됐다.</summary>
    public RawFenceLifetime FenceLifetime()
    {
        if (_fenceLifetime is null)
        {
            var hooks = new FenceLifetimeHooks
            {
                Neighbor = (sid, direction, allied) =>
                    (_fenceLifetimeDevices.Neighbor ?? throw new InvalidOperationException("장기 펜스의 실제 이웃 조회 미연결"))(sid, direction, allied),
                ClearMarkers = (sid, other) =>
                    (_fenceLifetimeDevices.ClearMarkers ?? throw new InvalidOperationException("장기 펜스 표식 실제 삭제 미연결"))(sid, other),
                ClearLine = (points, first, second, parent, other) =>
                    (_fenceLifetimeDevices.ClearLine ?? throw new InvalidOperationException("장기 펜스 연결선 실제 삭제 미연결"))(points, first, second, parent, other),
                Unpop = (sid, flags) => UnpopAsset(sid, flags),
                Repop = (sid, flags) => RepopAsset(sid, flags),
                Rebuild = sid =>
                    (_fenceLifetimeDevices.Rebuild ?? throw new InvalidOperationException("장기 펜스 실제 재생성 미연결"))(sid),
                ReserveRegular = () => AllocateResourceRegular(),
                AddRegular = (sid, @event, payload) => ConstructResourceRegular(sid, @event, payload),
                Damageable = (sid, flags) => DamageablePre().PreDestroy(sid, flags),
            };
            _fenceLifetime = new RawFenceLifetime(Pool, hooks);
        }

        return _fenceLifetime;
    }
}
