using Netstorm.Core.Simulation;

// 원문 생성 부모가 같은 실제 풀과 현재 TYPE/가상 초기화를 소비하게 한다.
namespace Netstorm.Core.Original;

/// <summary>일반·내용물·Gump 생성이 같은 현재 부모를 사용한다.</summary>
public sealed partial class RawSquidFactory
{
    /// <summary>전체 DWORD를 좁히기 전에 실제 물리 SID 범위만 검사한다.</summary>
    private int CreationObject(uint sid)
    {
        if (sid >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(sid), "Create의 실제 물리 SID 범위");
        return (int)sid;
    }

    /// <summary>진단→실제 할당→현재 생성자→반환 객체 TYPE/저장VT+28을 실행하고 선행 효과를 보존한다.</summary>
    public int Create(uint type, SidAllocation flags = SidAllocation.Server)
    {
        var hooks = new RawSquidCreationHooks
        {
            FakeSurfaceType = CurrentFakeSurfaceType,
            Assertion = (expression, _, line) => Assert(expression, line),
            Allocate = value => (uint)_pool.Allocate((SidAllocation)value),
            Constructor = number => TypeRecord(number).ConstructorAddress,
            Construct = (address, number) => (uint)ConstructRaw(address, CreationObject(number), true),
            Vtable = number => ReadVtable(_pool.Slot(CreationObject(number))),
            StoreType = (number, value) => _pool.RawSlot(CreationObject(number))[RawSidPool.TypeOffset] = value,
            PostCreate = (number, table) => VirtualPostCreate(CreationObject(number), table),
        };
        return CreationObject(RawSquidCreation.Create(_pool.Edition, type, (uint)flags, hooks));
    }

    /// <summary>현재TYPE6의 실제 생성자와 반환 객체의 가상 초기화를 일반 Create로 실행한다.</summary>
    public int CreateContained(uint flags) => Create(6, (SidAllocation)flags);

    /// <summary>현재TYPE3의 실제 생성자와 반환 객체의 가상 초기화를 일반 Create로 실행한다.</summary>
    public int CreateGumpForm(uint flags = 2) => Create(RawGumpFormHost.GumpFormType, (SidAllocation)flags);
}
