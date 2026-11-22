using Netstorm.Core.Original;

// State가 고른 실제 가상 삭제를 같은 현재 Session의 기존 분배기에 연결한다.
namespace Netstorm.Client.Host;

/// <summary>가상 삭제 본체를 복제하지 않고 같은 Pool의 실제 설치된 분배기만 빌린다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 월드의 기존 가상+10 분배기다. 미설치 선택을 일반 삭제로 대체하지 않는다.</summary>
    private RawSquidFormDispatch? _stateDeletion;

    /// <summary>원본 가상 삭제를 수행할 같은 실제 월드 분배기를 설치하며 현재 값/Pool을 바꾸지 않는다.</summary>
    public void BindStateDeletion(RawSquidFormDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        EnsureRuntimeCanRelease();
        if (_playerRecordsReleased || ProcessBindingsReturned()) throw new InvalidOperationException("State 반납 뒤 가상 삭제 분배 설치 금지");
        if (!ReferenceEquals(dispatch.Pool, Pool)) throw new ArgumentException("State 가상 삭제의 실제 Pool 불일치", nameof(dispatch));
        if (_stateDeletion is not null && !ReferenceEquals(_stateDeletion, dispatch)) throw new InvalidOperationException("현재 State 가상 삭제 분배 중복 설치");
        _stateDeletion = dispatch;
    }

    /// <summary>호출자가 같은 전체 Runtime을 pin한 선택 위치에서만 기존 가상 삭제를 실행한다.</summary>
    internal bool DestroyForState(int sid, uint flags)
    {
        if (_playerRecordsReleased || ProcessBindingsReturned()) throw new InvalidOperationException("State 반납 뒤 실제 가상 삭제 금지");
        return (_stateDeletion ?? throw new InvalidOperationException("State의 실제 Native World 가상 삭제 분배 미연결")).Destroy(sid, flags);
    }

    /// <summary>파편/건설도 State와 같은 실제 전체 가상10 분배를 사용한다. 미설치를 일반 Pre/Post로 대체하지 않는다.</summary>
    public void DestroyWorldObject(int sid, uint flags) => _ = DestroyForState(sid, flags);

    /// <summary>같은 Host의 소진/실패 정리도 현재 전체 가상 분배로 보낸다. State 전용 설치와 구별한다.</summary>
    public void BindWorldDeletion(RawSquidFormDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        EnsureRuntimeCanRelease();
        ProcessHostRuntimeBindings runtime = _playerProcessRuntime ?? throw new InvalidOperationException("월드 전체 삭제의 실제 프로세스 Runtime 미연결");
        if (!ReferenceEquals(dispatch.DestroyBody, runtime.Destroy)) throw new ArgumentException("월드 전체 삭제의 실제 Host 삭제 본체/깊이 불일치", nameof(dispatch));
        BindStateDeletion(dispatch);
        runtime.Asset = dispatch.Hooks();
        if (_worldDependencies is { } dependencies) dependencies.BindVirtualDestroy(DestroyWorldObject, BeforeWorldDependencyFind);
    }
}
