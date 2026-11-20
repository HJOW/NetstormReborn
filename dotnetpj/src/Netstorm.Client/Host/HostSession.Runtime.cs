// 실제 현재 Runtime의 실행 대여와 전체 월드 반납 관문을 세션에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>장기 Gameplay의 현재 owner를 선택하는 세션 부분이다.</summary>
public sealed partial class HostSession
{
    /// <summary>일반/Editor/Meta의 실제 전체 owner가 공개한 같은 현재 배송 대여다.</summary>
    private Func<IDisposable>? _runtimePin;
    /// <summary>실제 outer owner 공개 뒤 현재 pin 장치만 연결하며 조회/초기화는 하지 않는다.</summary>
    internal void BindRuntimePin(Func<IDisposable> pin) { ArgumentNullException.ThrowIfNull(pin); _runtimePin = pin; }
    /// <summary>현재 실제 전체 owner를 선택된 하위 호출 끝까지 보유한다.</summary>
    public IDisposable PinRuntime() => (_runtimePin ?? throw new InvalidOperationException("세션의 실제 Runtime 대여 미공개"))();
    /// <summary>배송 중 실제 Pool/Player/장치를 명시 반납하는 전체 정리를 거부한다.</summary>
    private void EnsureRuntimeCanRelease() { if (Gameplay.RuntimeCallActive(this)) throw new InvalidOperationException("실제 Runtime 배송 종료 전 월드 반납"); }
}
