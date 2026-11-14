// 실제 공장이 원문 Take 부모에 현재 가상 해제·생성자·후처리 장치를 제공한다.
namespace Netstorm.Core.Original;

/// <summary>같은 Pool/현재 TYPE로 전체 Take 부모를 실행한다.</summary>
public sealed partial class RawSquidFactory
{
    /// <summary>DWORD 토큰을 SID로 좁히기 전에 실제 풀 물리 범위를 확인한다.</summary>
    private int TakeObject(uint number)
    {
        if (number >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(number), "Take의 실제 물리 SID 범위");
        return (int)number;
    }

    /// <summary>현재 TYPE와 독립적으로 네 실제 Form 생성자 VT인지 판본에 맞춰 읽는다.</summary>
    private bool IsTakeFormVtable(uint table)
    {
        // 네 확인된 클래스만 순서대로 비교한다. TYPE 별칭과 클래스의 쌍 일치를 강제하지 않는다.
        foreach (uint type in FormTypes) if (table == FormVtable(type)) return true;
        return false;
    }

    /// <summary>모든 !free 슬롯의 실제 VT+48→현재 생성자→반환 객체→저장 VT+2c를 실행한다. 풀 할당은 하지 않는다.</summary>
    public int Take(uint type, int sid)
    {
        using var call = BeginRuntimeCall();
        SquidFactoryRuntimeBindings? runtime = call.Runtime;
        var hooks = new RawSquidTakeHooks
        {
            Capacity = () => (uint)_pool.Capacity,
            Slot = number => (uint)TakeObject(number),
            State = number => _pool.Slot(TakeObject(number))[RawSidPool.StateOffset],
            Type = number => _pool.Slot(TakeObject(number))[RawSidPool.TypeOffset],
            Vtable = number => ReadVtable(_pool.Slot(TakeObject(number))),
            Detail = WriteTakeDetail,
            Assertion = (expression, _, line) => Assert(expression, line),
            Unpop = (number, table) =>
            {
                int current = TakeObject(number);
                if (IsTakeFormVtable(table)) { UnpopForm(current); return; }
                if (!SupportsConstructedVtable(_pool.Edition, table)) throw new InvalidOperationException("Take의 선택된 실제 VT+48 미연결");
                IRawUnpop unpop = runtime is not null ? runtime.Unpop! : _currentUnpop is { } getter ? getter() : _unpop ?? throw new InvalidOperationException("Take의 선택된 실제 공간 해제 미연결");
                if (!ReferenceEquals(unpop.Pool, _pool)) throw new InvalidOperationException("Take 공간 해제의 같은 Pool 계약 위반");
                unpop.Unpop(current, TypeRecord(_pool.Slot(current)[RawSidPool.TypeOffset]));
            },
            StoreState = (number, value) => _pool.RawSlot(TakeObject(number))[RawSidPool.StateOffset] = value,
            StoreType = (number, value) => _pool.RawSlot(TakeObject(number))[RawSidPool.TypeOffset] = value,
            Constructor = number => TypeRecord(number).ConstructorAddress,
            Construct = (address, number) => (uint)ConstructRaw(address, TakeObject(number), true),
            PostTake = (number, table) =>
            {
                if (IsTakeFormVtable(table)) return;
                if (!SupportsConstructedVtable(_pool.Edition, table)) throw new InvalidOperationException("Take의 선택된 실제 VT+2c 미연결");
                PostTake(TakeObject(number));
            },
        };
        return TakeObject(RawSquidTake.Take(_pool.Edition, type, unchecked((uint)sid), hooks));
    }
}
