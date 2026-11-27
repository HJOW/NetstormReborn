using Netstorm.Client.Gump;
using Netstorm.Core.Original;

namespace Netstorm.Client.Host; // 장기 물리의 GumpForm과 현재 실제 UI 등록표를 연결한다.

/// <summary>같은 공통 트리의 Form 삭제와 부모 소실 통지 대여를 관리한다.</summary>
public sealed class HostGumpFormRuntime : IDisposable
{
    /// <summary>실제 현재 GumpForm을 소유하는 장기 물리다.</summary>
    private readonly RawSurfaceStorage _storage;
    /// <summary>같은 실제 등록표와 수명을 가진 트리다.</summary>
    private readonly GumpButtonTree _tree;
    /// <summary>물리가 약하게 빌리는 현재 UI 통지와 수명이다.</summary>
    private readonly RawSurfaceGumpBindings _binding;
    /// <summary>다른 Form의 기존 삭제 경계다.</summary>
    private readonly Action<uint, uint>? _previousDelete;
    /// <summary>자기 삭제 분배만 반납할 때 식별하는 함수다.</summary>
    private readonly Action<uint, uint> _delete;
    /// <summary>통지와 삭제 분배를 더 이상 사용하지 않는 상태다.</summary>
    private bool _disposed;

    /// <summary>실제 등록표 통지와 Form 삭제를 연결하되 파생 노드 처리는 현재 소유자에게 맡긴다.</summary>
    public HostGumpFormRuntime(RawSurfaceStorage storage, GumpButtonTree tree, Func<uint, int, bool> releaseDerived, Action changed)
    {
        ArgumentNullException.ThrowIfNull(storage); ArgumentNullException.ThrowIfNull(tree);
        ArgumentNullException.ThrowIfNull(releaseDerived); ArgumentNullException.ThrowIfNull(changed);
        if (!storage.UsesNativeTransmissionDevices() || storage.Pool.Edition != tree.Create.Collection.Edition)
            throw new ArgumentException("Gump UI의 실제 Native 물리와 트리 판본 필요", nameof(storage));
        _storage = storage; _tree = tree; _previousDelete = tree.LifetimeHooks.DeleteForm;
        var weak = new WeakReference<HostGumpFormRuntime>(this);
        Action<uint, uint>? previous = _previousDelete;
        _delete = (form, flags) =>
        {
            if (!weak.TryGetTarget(out var owner) || owner._disposed)
            {
                (previous ?? throw new InvalidOperationException("현재 Gump Form 삭제 대여 반납됨"))(form, flags);
                return;
            }
            if (form > ushort.MaxValue) throw new ArgumentOutOfRangeException(nameof(form), "Gump Form SID 범위 오류");
            RawGumpFormHost forms = owner._storage.GumpForms();
            if (forms.IsGumpForm((int)form)) forms.Destroy((int)form, flags);
            else (owner._previousDelete ?? throw new InvalidOperationException("다른 UI Form 삭제 미연결"))(form, flags);
        };
        GumpFormNotice notice = GumpFormBinding.MakeGumpFormNotice(tree.Create, (token, parent) =>
        {
            if (!releaseDerived(token, parent)) GumpFormBinding.ReleaseGumpObject(tree.Lifetime, token, parent);
            changed();
        });
        _binding = new() { Notice = notice, Pin = () => tree };
        storage.BindGumpUiRuntime(_binding);
        tree.LifetimeHooks.DeleteForm = _delete;
    }

    /// <summary>현재 UI 노드의 Form을 모두 정리한 뒤 자기 대여와 삭제 분배를 반납한다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        _storage.ReleaseGumpUiRuntime(_binding);
        _disposed = true;
        if (_tree.LifetimeHooks.DeleteForm == _delete) _tree.LifetimeHooks.DeleteForm = _previousDelete;
    }
}
