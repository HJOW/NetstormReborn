// 썬 펜스의 실제 재생성과 수명 실제 이웃 결선을 같은 장기 풀·이웃·정리에 연결한다.
// cpppj/src/client/RawSurfaceFence.cpp 의 FenceRebuild 장기 본문을 이식한다.
// 0.590에서 추가됐다. (R4-1 잔여 중 펜스 수명 실제 이웃 결선)
namespace Netstorm.Core.Original;

/// <summary>장기 펜스 재생성의 실제 표식·포스 필드 생성 장치를 묶는다. 0.590에서 추가됐다.</summary>
public sealed class RawSurfaceFenceRebuildDevices
{
    /// <summary>00456460의 내부 표식 생성이다. 0.590에서 추가됐다.</summary>
    public Action<int, int>? Markers { get; set; }

    /// <summary>00455b20(mode1)의 현재 좌표/owner/끝점 SID 생성이다. 0.590에서 추가됐다.</summary>
    public Action<float[], uint, uint, int, int>? Shields { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.590에서 추가됐다.</summary>
    internal RawSurfaceFenceRebuildDevices Copy() => new()
    {
        Markers = Markers,
        Shields = Shields,
    };
}

/// <summary>World 없이 같은 장기 Pool로 펜스 재생성을 보유한다. 0.590에서 추가됐다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 장기 표식·포스 필드 생성 장치를 본체보다 오래 유지한다. 0.590에서 추가됐다.</summary>
    private RawSurfaceFenceRebuildDevices _fenceRebuildDevices = new();

    /// <summary>같은 장기 이웃·사거리를 공유하는 재생성 본체다. 0.590에서 추가됐다.</summary>
    private RawFenceRebuild? _fenceRebuild;

    /// <summary>첫 실제 재생성 전에 실제 표식·포스 필드 생성 장치만 연결한다. 0.590에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 재생성 장치 묶음이다.</param>
    public void BindFenceRebuildDevices(RawSurfaceFenceRebuildDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 뒤, 첫 본체/대여 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _fenceRebuild is not null
            || _fenceRebuildDevices.Markers is not null || _fenceRebuildDevices.Shields is not null
            || devices.Markers is null || devices.Shields is null)
        {
            throw new ArgumentException("장기 펜스 재생성의 실제 최초 장치 중복/누락", nameof(devices));
        }

        _fenceRebuildDevices = devices.Copy();
    }

    /// <summary>같은 장기 이웃·사거리와 실제 생성 장치를 사용하는 재생성 본체다. 0.590에서 추가됐다.</summary>
    public RawFenceRebuild FenceRebuild()
    {
        if (_fenceRebuild is null)
        {
            var hooks = new FenceRebuildHooks
            {
                Neighbor = (sid, direction) =>
                    FenceNeighbors().Neighbor(sid, direction, true),
                WithinRange = (sid, endpoints) =>
                    FenceNeighbors().WithinRange(sid, endpoints),
                Markers = (sid, other) =>
                    (_fenceRebuildDevices.Markers ?? throw new InvalidOperationException("장기 펜스 표식 실제 생성 미연결"))(sid, other),
                Shields = (points, first, second, parent, other) =>
                    (_fenceRebuildDevices.Shields ?? throw new InvalidOperationException("장기 펜스 포스 필드 실제 생성 미연결"))(points, first, second, parent, other),
            };
            _fenceRebuild = new RawFenceRebuild(Pool, hooks);
        }

        return _fenceRebuild;
    }

    /// <summary>
    /// 수명 장치 묶음을 같은 저장소의 실제 이웃·정리·재생성 본체로 채운다.
    /// 동맹 읽기·표식/포스 필드 생성 장치가 먼저 연결되어 있어야 하며, 수명 본체 생성 전 한 번만 호출한다.
    /// 0.590에서 추가됐다. (R4-1 펜스 수명 실제 이웃 결선)
    /// </summary>
    public void BindFenceLifetimeActuals()
    {
        // Native 전송 뒤, 동맹·재생성 생성 장치 다음·수명 본체/대여 전에만 실제 본체를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || FenceLifetimeBound()
            || _fenceAlliances is null
            || _fenceRebuildDevices.Markers is null || _fenceRebuildDevices.Shields is null)
        {
            throw new ArgumentException("장기 펜스 수명의 실제 본체 결선 중복/누락");
        }

        _fenceLifetimeDevices = new RawSurfaceFenceLifetimeDevices
        {
            Neighbor = (sid, direction, allied) => FenceNeighbors().Neighbor(sid, direction, allied),
            ClearMarkers = (sid, other) => FenceMarkerClear().Clear(sid, other),
            ClearLine = (points, first, second, parent, other) => FenceLine().Clear(points),
            Rebuild = sid => FenceRebuild().Rebuild(sid),
        };
    }

    /// <summary>수명 장치 묶음이 이미 채워졌는지 확인한다. 0.590에서 추가됐다.</summary>
    private bool FenceLifetimeBound() =>
        _fenceLifetime is not null
        || _fenceLifetimeDevices.Neighbor is not null || _fenceLifetimeDevices.ClearMarkers is not null
        || _fenceLifetimeDevices.ClearLine is not null || _fenceLifetimeDevices.Rebuild is not null;
}
