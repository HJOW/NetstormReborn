// 실제 공간 해제가 현재 TYPE/진단과 공통 부모의 도달 장치를 제공한다.
namespace Netstorm.Core.Original;

/// <summary>두 판본의 현재 공간 부모를 같은 실제 Pool/Hash/점유/표시에 연결한다.</summary>
public sealed partial class RawUnpop
{
    /// <summary>같은 Client의 현재 TYPE/checking과 원문 진단 장치다.</summary>
    private SquidUnpopDiagnostics _diagnostics = new();
    /// <summary>선택한 실제 공통 종속 분리다. 미연결을 빈 성공으로 대체하지 않는다.</summary>
    private Action<int>? _containedUnpop;

    /// <summary>진단 함수를 값 복사하고 TYPE/checking은 같은 장기 소유자로 빌린다.</summary>
    public void BindDiagnostics(SquidUnpopDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics); _diagnostics = diagnostics.Copy();
    }

    /// <summary>새 호출이 선택할 실제 종속 분리 함수를 연결하거나 반납한다.</summary>
    public void BindContainedUnpop(Action<int>? unpop) => _containedUnpop = unpop;

    /// <summary>타입/공간/표시를 미리 검증하지 않고 원문 부모가 도달하는 순서대로 현재 장치를 선택한다.</summary>
    private void UnpopCurrent(int sid, OriginalTypeRecord type, uint flags)
    {
        ArgumentNullException.ThrowIfNull(type);
        var hooks = new SquidUnpopParentHooks
        {
            Type = number =>
            {
                IReadOnlyList<OriginalTypeRecord> selected = _diagnostics.Types;
                if (selected.Count == 0) return new(() => type);
                if (number >= selected.Count) throw new ArgumentOutOfRangeException(nameof(number), "Unpop 현재 TYPE 범위");
                return new(() => selected[number]);
            },
            Assertion = _diagnostics.Assertion,
            MiniMap = (number, _) => _minimap?.Enqueue(number),
            Side = (x, y) => (_building ?? throw new InvalidOperationException("Unpop 실제 건물 옆면 미연결")).UnpopSide(x, y),
            Contained = _containedUnpop,
            Update = (number, value) =>
            {
                ReadOnlySpan<byte> raw = _pool.RawSlot(number);
                if (!OriginalUnpopDisplay.Supports(_pool.Edition, Read(raw, 0, 4), value)) throw new InvalidOperationException("Unpop의 현재 파생 표시 갱신 미복원");
                _display?.Update(raw, value);
            },
        };
        RawSquidUnpopParent.Run(_pool, _hash, _spots, sid, flags, new RawCheckingFlag(_diagnostics.Checking), hooks);
    }
}
