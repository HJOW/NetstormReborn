// 실제 현재 Runtime의 실행 대여와 전체 월드 반납 관문을 세션에 연결한다.
using Netstorm.Core.Original;

namespace Netstorm.Client.Host;

/// <summary>장기 Gameplay의 현재 owner를 선택하는 세션 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>일반/Editor/Meta의 실제 전체 owner가 공개한 같은 현재 배송 대여다.</summary>
    private Func<IDisposable>? _runtimePin;
    /// <summary>직접생성한후처리부모의약한장부다. 이전그래프/최소AI부모의수명을늘리지않는다.</summary>
    private readonly List<WeakReference<RawPostPop>> _runtimePostPops = [];
    /// <summary>실제새부모에현재함수사본을전달하고이후공개/반납갱신대상으로기록한다.</summary>
    private RawPostPop TrackPostPopRuntime(RawPostPop body)
    {
        ArgumentNullException.ThrowIfNull(body); _runtimePostPops.Add(new(body));
        Func<IDisposable>? pin = _runtimePin; body.BindRuntimePin(pin is null ? null : () => pin()); return body;
    }
    /// <summary>실제 outer owner 공개 뒤 현재 pin 장치만 연결하며 조회/초기화는 하지 않는다.</summary>
    internal void BindRuntimePin(Func<IDisposable>? pin)
    {
        _runtimePin = pin;
        Func<object?>? current = pin is null ? null : () => pin();
        if (_playerProcessRuntime is { } processes) processes.Pin = current;
        // 현재살아있는실제부모만갱신하며이전호출이이미보유한대여는건드리지않는다.
        for (int index = _runtimePostPops.Count - 1; index >= 0; index--)
        {
            if (_runtimePostPops[index].TryGetTarget(out RawPostPop? body)) body.BindRuntimePin(current);
            else _runtimePostPops.RemoveAt(index);
        }
    }
    /// <summary>현재 실제 전체 owner를 선택된 하위 호출 끝까지 보유한다.</summary>
    public IDisposable PinRuntime() => (_runtimePin ?? throw new InvalidOperationException("세션의 실제 Runtime 대여 미공개"))();
    /// <summary>배송 중 실제 Pool/Player/장치를 명시 반납하는 전체 정리를 거부한다.</summary>
    private void EnsureRuntimeCanRelease()
    {
        if (Gameplay.RuntimeCallActive(this) || _playerProcessRuntime is { ActiveCalls: not 0 })
            throw new InvalidOperationException("실제 Runtime 배송 종료 전 월드 반납");
    }
}
