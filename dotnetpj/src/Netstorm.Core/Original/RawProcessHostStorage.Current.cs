// 장기 저장소와 정리 증빙이 짧은 Runtime을 역소유하지 않도록 같은 현재 대상만 잠근다.
namespace Netstorm.Core.Original;

/// <summary>원문 current_.lock과 ClearPass의 weak_ptr 수명을 관리 코드로 연결한다.</summary>
public sealed partial class RawProcessHostStorage
{
    /// <summary>현재 짧은 대여의 비소유 선택 슬롯이다. Use와 실행 중 snapshot만 Runtime을 강하게 소유한다.</summary>
    private WeakReference<ProcessHostRuntimeBindings>? _current;
    /// <summary>호출 위치에서 살아 있는 같은 Runtime만 반환한다. 호출자는 사용 끝까지 이 snapshot을 보유한다.</summary>
    private ProcessHostRuntimeBindings? CurrentRuntime() => _current is not null && _current.TryGetTarget(out ProcessHostRuntimeBindings? runtime) ? runtime : null;

    /// <summary>정상 Clear의 정체성만 기록하며 발급 Runtime을 살아 있게 만들지 않는다.</summary>
    public sealed partial class ClearPass
    {
        /// <summary>발급 위치의 같은 Runtime을 약하게 기록하는 증빙이다.</summary>
        private readonly WeakReference<ProcessHostRuntimeBindings> _runtime;
        /// <summary>반납 검증 위치에서 살아 있는 같은 발급 대상만 잠근다.</summary>
        internal ProcessHostRuntimeBindings? Runtime => _runtime.TryGetTarget(out ProcessHostRuntimeBindings? runtime) ? runtime : null;
    }
}
