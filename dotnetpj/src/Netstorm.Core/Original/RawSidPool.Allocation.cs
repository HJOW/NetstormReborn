using Netstorm.Core.Simulation;

// 현재 할당 진단·소진 회수를 같은 원본 풀의 물리 상태에 적용한다.
namespace Netstorm.Core.Original;

/// <summary>긴 Pool이 현재 전역을 빌리고 실제 삭제는 현재 World 대여에서 선택하는 장치다. 연결 자체는 게임값을 읽거나 쓰지 않는다.</summary>
public sealed class SidAllocationBindings
{
    /// <summary>005c8458의 현재 전체 금지 DWORD다. 독립 미연결은 원본 BSS0을 사용한다.</summary>
    public Func<uint>? Disallow { get; set; }
    /// <summary>소진 회수를 선택할 때 읽는 현재00540bc4 권한이다.</summary>
    public Func<bool>? Boss { get; set; }
    /// <summary>현재005411a0의 전체 DWORD다. 슬롯 TYPE 바이트와 비교할 때 축소하지 않는다.</summary>
    public Func<uint>? BridgeType { get; set; }
    /// <summary>선택된 원문 단언의 정상/예외 복귀를 전달한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
    /// <summary>현재 가상+10을 flags0으로 실행하는 실제 World/ProcessHost 삭제 경계다.</summary>
    public Action<int>? Reclaim { get; set; }
    /// <summary>CD 실제 소진 복원 때 연결할 객체 덤프 장치다. 기존 관리 CD 하네스에서는 선택하지 않는다.</summary>
    public Action? Dump { get; set; }
    /// <summary>CD 상세 문구를 현재 Client owner로 복사하는 장치다.</summary>
    public Action<string>? Detail { get; set; }
    /// <summary>CD 회수 뒤 실제 Tell 부모 장치다.</summary>
    public Action<string>? Tell { get; set; }
}

/// <summary>cpppj/src/o/SidPool.cpp/.h의 PATCH 현재 할당·진단·소진 회수 몸체다.</summary>
public sealed partial class RawSidPool
{
    /// <summary>현재 장기 전역 getter와 선택된 실제 가상 삭제의 함수 사본이다.</summary>
    private SidAllocationBindings _allocation = new();

    /// <summary>현재 장치만 값 사본으로 보관하며 풀·목록·카운터·원본 owner값을 초기화하지 않는다.</summary>
    public void BindAllocation(SidAllocationBindings bindings)
    {
        ArgumentNullException.ThrowIfNull(bindings);
        _allocation = new() { Disallow = bindings.Disallow, Boss = bindings.Boss, BridgeType = bindings.BridgeType, Assertion = bindings.Assertion,
            Reclaim = bindings.Reclaim, Dump = bindings.Dump, Detail = bindings.Detail, Tell = bindings.Tell };
    }

    /// <summary>선택된 단언을 호출 끝까지 보관하고 진단이 정상 복귀하면 같은 원문 위치에서 계속한다.</summary>
    private void AssertAllocation(string expression, uint line, string file = @"\Ns\O\Squid.cpp")
    {
        Action<string, string, uint> assertion = _allocation.Assertion ?? throw new InvalidOperationException("SID 할당의 선택된 원문 단언 미연결");
        assertion(expression, file, line);
    }

    /// <summary>DWORD를 WORD로 좁히기 전에 현재 물리 범위만 확인한다. 진단이나 삭제가 재확보한 현재 풀을 매번 읽는다.</summary>
    private int AllocationOffset(uint sid) => sid < Capacity ? (int)sid * Stride : throw new ArgumentOutOfRangeException(nameof(sid), sid, "SID 할당의 실제 물리 범위");

    /// <summary>현재 슬롯의 상태 BYTE다. 예약/free 여부에 별도 관문을 추가하지 않는다.</summary>
    private byte AllocationState(uint sid) => _bytes[AllocationOffset(sid) + StateOffset];

    /// <summary>PATCH004af1d0의 금지 진단·현재 권한·최대50 회수·현재 머리 갱신·슬롯 초기화를 원문 순서로 실행한다.</summary>
    public int Allocate(SidAllocation flags = SidAllocation.Server)
    {
        if (Edition == OriginalEdition.Cd1072) return AllocateLegacyCd(flags);
        if ((_allocation.Disallow?.Invoke() ?? 0) != 0) AssertAllocation("disallowSquidAllocation == 0", 0xe8);
        uint sid;
        if ((flags & SidAllocation.Predictable) != 0)
        {
            int head = PredictableFirst * Stride;
            if ((_bytes[head + StateOffset] & FreeBit) == 0) AssertAllocation("predictableHead->isFree()", 0xf0);
            sid = unchecked((uint)PredictableFirst + PredictableCursor);
            if (unchecked((int)sid) >= Capacity) AssertAllocation("sid < numSquids", 0xf2);
            if ((AllocationState(sid) & FreeBit) == 0) AssertAllocation("me->isFree()", 0xf4);
            if (IsServer)
            {
                if ((uint)ReadNext(AllocationOffset(sid)) != unchecked(sid + 1)) AssertAllocation("me->getNext() == sid+1", 0xf7);
                uint next = (uint)ReadNext(AllocationOffset(sid));
                if ((AllocationState(next) & FreeBit) == 0) AssertAllocation("checkMe->isFree()", 0xfa);
                ushort selected = (ushort)ReadNext(AllocationOffset(sid));
                if (selected == PredictableFirst) AssertAllocation("n != getSid()", 0x123, @"c:\ns\o\squid.h");
                WriteNext(head, selected);
            }
        }
        else
        {
            bool client = (flags & SidAllocation.Client) != 0;
            if (!client && !IsServer) AssertAllocation("server", 0x10a);
            int list = client ? 0 : 1;
            if (ReadNext(Heads[list]) == _tails[list])
            {
                Func<bool> boss = _allocation.Boss ?? throw new InvalidOperationException("SID 소진 복구의 현재 boss 미연결");
                if (boss())
                {
                    uint end = (uint)Capacity, candidate = 2, remaining = 50;
                    // 최초 iterator 끝은 보관하고 첫 현재 live 슬롯만 원문처럼 찾는다.
                    while (candidate < end && (AllocationState(candidate) & FreeBit) != 0) candidate++;
                    // 삭제 뒤 현재 capacity/종류/물리를 읽으며 free 슬롯만 건너뛴다.
                    while (candidate >= 1 && candidate < Capacity && remaining != 0)
                    {
                        Func<uint> bridge = _allocation.BridgeType ?? throw new InvalidOperationException("SID 소진 복구의 현재 Bridge TYPE 미연결");
                        if ((uint)_bytes[AllocationOffset(candidate) + TypeOffset] == bridge())
                        {
                            Action<int> reclaim = _allocation.Reclaim ?? throw new InvalidOperationException("SID 소진 복구의 현재 가상 삭제 미연결");
                            reclaim((ushort)candidate); remaining--;
                        }
                        candidate++;
                        // 원문의 최초 끝과 콜백 뒤 새 배열을 함께 사용한다.
                        while (candidate < end && (AllocationState(candidate) & FreeBit) != 0) candidate++;
                    }
                }
                if (!client && !IsServer) AssertAllocation("server", 0x139);
            }
            sid = (uint)ReadNext(Heads[list]);
            if ((AllocationState(sid) & FreeBit) == 0) AssertAllocation("p->isFree()", 0x149);
            if (sid == _tails[list]) AssertAllocation("p != *freeTail", 0x14a);
            ushort next = (ushort)ReadNext(AllocationOffset(sid));
            if (next == 0) AssertAllocation("n != getSid()", 0x123, @"c:\ns\o\squid.h");
            WriteNext(Heads[list], next);
        }
        int offset = AllocationOffset(sid);
        Array.Clear(_bytes, offset, Stride); _bytes[offset + StateOffset] = VoidBit;
        if ((flags & SidAllocation.Predictable) != 0) PredictableCursor = unchecked(PredictableCursor + 1);
        FreeCount = unchecked(FreeCount - 1);
        return (ushort)sid;
    }
}
