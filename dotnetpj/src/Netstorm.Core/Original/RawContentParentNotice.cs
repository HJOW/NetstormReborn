using System.Buffers.Binary;

// 현재 부모 가상표에 따른 내용물 통지 분배기를 담는다.
namespace Netstorm.Core.Original;

/// <summary>실제 빈 부모 통지와 파생 통지를 구분한다. cpppj/src/o/RawContentParentNotice.cpp의 VT+24 분배에 대응한다.</summary>
public sealed partial class RawContentParentNotice
{
    /// <summary>통지 직전의 현재 부모 가상표를 읽는 같은 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>실제 파생 몸체를 실행할 소비자다. 없으면 선택된 파생 통지에서 명시 실패한다.</summary>
    private readonly Action<int, int, uint>? _derived;

    /// <summary>풀과 실제 파생 소비자를 빌린다. 생성 중 슬롯·게임 상태를 조회하거나 수정하지 않는다.</summary>
    public RawContentParentNotice(RawSidPool pool, Action<int, int, uint>? derived = null)
    {
        _pool = pool;
        _derived = derived;
    }

    /// <summary>현재 가상표의 원본 진입 주소를 반환한다. 알려지지 않은 가상표는 다른 타입의 빈 몸체로 대체하지 않는다.</summary>
    public uint NativeEntry(int parent)
    {
        uint table = BinaryPrimitives.ReadUInt32LittleEndian(_pool.Slot(parent));
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        ReadOnlySpan<uint> forms = patch ? [0x512110u, 0x512098u, 0x512188u, 0x512208u] : [0x505a08u, 0x505990u, 0x505a80u, 0x505b00u];
        // 네 내장 Form의 내용물 통지는 모두 같은 빈 ret8 몸체다.
        foreach (uint form in forms) if (table == form) return patch ? 0x41c100u : 0x401c20u;
        ReadOnlySpan<(uint Table, uint Entry)> entries = patch ? PatchEntries : CdEntries;
        // 해당 판본의 지원 생성자 가상표에서 실제 진입점을 찾는다.
        foreach (var candidate in entries) if (candidate.Table == table) return candidate.Entry;
        throw new InvalidOperationException($"내용물 부모의 미복원 VT: {table}");
    }

    /// <summary>빈 몸체는 인자를 읽지 않고 반환한다. 파생 몸체는 인자를 그대로 실제 소비자에 전달한다.</summary>
    public void Notice(int parent, int form, uint reason)
    {
        uint entry = NativeEntry(parent);
        if (entry == (_pool.Edition == OriginalEdition.Patch1078 ? 0x41c100u : 0x401c20u)) return;
        var call = _derived ?? throw new InvalidOperationException($"실제 내용물 부모 파생 통지 미연결: {entry}");
        call(parent, form, reason);
    }
}
