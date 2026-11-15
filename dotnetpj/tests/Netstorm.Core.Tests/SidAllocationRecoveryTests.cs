using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 같은 현재 Pool의 전체 물리·삭제 기록·진단·가상 회수와 호출 수명을 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>cpppj SidAllocationRecoveryTests의 PATCH 원본47행과 실제 World/Host 결선을 옮긴다.</summary>
public sealed class SidAllocationRecoveryTests
{
    /// <summary>원본 관측의 실제 Pool 크기다.</summary>
    private const int Capacity = 23005;
    /// <summary>명시 가상 삭제 경계에 쓴 원본 관측의 테스트 가상표다.</summary>
    private const uint FakeTable = 0x10001000;

    /// <summary>진단 식·경로의 ASCII 바이트를 원본 관측과 같은 소문자16진수로 표시한다.</summary>
    private static string Hex(string value) => Convert.ToHexStringLower(Encoding.ASCII.GetBytes(value));

    /// <summary>두 삭제 기록의 client→other 순서를320BYTE로 유지해 독립 Adler32에 대조한다.</summary>
    private static uint LogAdler(RawSidPool pool)
    {
        byte[] bytes = new byte[320]; int offset = 0;
        // 각 배열의 최신 기록부터 원본 두 DWORD를 기록한다.
        foreach (bool client in new[] { true, false })
        {
            // SID와 TYPE를 패딩 없이 낮은 바이트부터 직렬화한다.
            foreach (SidDeletion record in pool.Deletions(client))
            {
                BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset), record.Sid);
                BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(offset + 4), record.Type); offset += 8;
            }
        }
        return X86Fixture.Adler32(bytes);
    }

    /// <summary>원본 도구와 같은 명시 입력을 실제 제품 Pool에 적용하는 장면이다. 기대값을 제품으로 생성하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>관측 모드 번호다.</summary>
        public readonly uint Mode;
        /// <summary>Allocate의 원문 비트 입력이다.</summary>
        public readonly SidAllocation Flags;
        /// <summary>현재 서버 owner다.</summary>
        public bool Server;
        /// <summary>현재 회수 권한 owner다.</summary>
        public bool Boss;
        /// <summary>전체 DWORD Bridge 타입이다.</summary>
        public uint Bridge = 82;
        /// <summary>현재 금지 DWORD다.</summary>
        public uint Disallow;
        /// <summary>실제 회수 진입 횟수다.</summary>
        public uint Kills;
        /// <summary>장치가 읽는 같은 실제 풀이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>원본 동기 호출 사건을 원문 순서로 보관한다.</summary>
        public readonly List<string> Events = [];

        /// <summary>기존 할당 뒤 명시 raw 오염과 현재 장치만 연결한다.</summary>
        public Scene(uint mode, uint flags, bool server, bool boss)
        {
            Mode = mode; Flags = (SidAllocation)flags; Server = server; Boss = boss;
            Pool = new(Capacity, server, () => Server);
            if (mode == 15) { Pool.Allocate(SidAllocation.Predictable); Pool.Allocate(SidAllocation.Predictable); }
            bool client = (flags & 2) != 0;
            if (mode == 1) Disallow = 1;
            if (mode == 3) Pool.RawSlot(Pool.FirstFree(client))[11] = 0;
            if (mode is 4 or >= 5 and <= 9 or 17) SetHead(Pool, client, Pool.Tail(client));
            if (mode == 6)
            {
                // 원본50회수 상한 뒤10개 후보가 남는 입력이다.
                for (int sid = 10; sid < 70; sid++) Candidate(sid, 82);
            }
            else if (mode is >= 5 and <= 9) { Candidate(10, 82); Candidate(11, mode == 7 ? (byte)70 : (byte)82); Candidate(13, 82); }
            if (mode == 17) Bridge = 0x10052;
            if (mode == 11) Pool.RawSlot(23001)[11] = 0;
            if (mode == 12) Pool.RawSlot(23002)[11] = 0;
            if (mode is 13 or 14) BinaryPrimitives.WriteUInt16LittleEndian(Pool.RawSlot(23002)[4..], (ushort)(mode == 13 ? 23004 : 23001));
            Pool.BindAllocation(Bindings());
        }

        /// <summary>실제 현재 전역과 원본 관측의 진단·삭제 동작을 연결한다.</summary>
        public SidAllocationBindings Bindings() => new()
        {
            Disallow = () => Disallow, Boss = () => Boss, BridgeType = () => Bridge,
            Assertion = (expression, file, line) =>
            {
                Events.Add($"A:{line}:{Hex(expression)}:{Hex(file)}");
                if (Mode == 1 && line == 0xe8) Disallow = 0;
                if (Mode == 11 && line == 0xf0) Pool.RawSlot(23001)[11] = 1;
            },
            Reclaim = sid =>
            {
                Kills++; Events.Add($"K:{sid}:0"); Pool.Release(sid);
                if (Kills == 1)
                {
                    if (Mode == 7) Bridge = 70;
                    else if (Mode == 8) Candidate(12, 82);
                    else if (Mode == 9) throw new InvalidOperationException("명시 삭제 장치 실패");
                }
            },
        };

        /// <summary>원본 Release가 받는 void/live 후보를 목록과 별개인 명시 raw 입력으로 쓴다.</summary>
        private void Candidate(int sid, byte type)
        {
            Span<byte> raw = Pool.RawSlot(sid); BinaryPrimitives.WriteUInt32LittleEndian(raw, FakeTable); raw[10] = type; raw[11] = 4;
        }
    }

    /// <summary>입력 하네스의 현재 자유 목록 머리 WORD를 지정한다.</summary>
    private static void SetHead(RawSidPool pool, bool client, int value) => BinaryPrimitives.WriteUInt16LittleEndian(pool.RawSlot(0)[(client ? 4 : 18)..], (ushort)value);

    /// <summary>반환·중단·전체Pool/기록·현재 전역·호출 순서를 독립 원본47행과 대조한다.</summary>
    [Fact]
    public void Native47CurrentDiagnosticsListsAndRelease()
    {
        int observations = 0;
        // 예외 중단의 부분 상태도 모든 원문 행에서 비교한다.
        X86Fixture.CheckRows(X86Fixture.Read("sid-allocation-recovery"), row =>
        {
            var s = new Scene(X86Fixture.UInt(row[0]), X86Fixture.UInt(row[1]), row[2] == "1", row[3] == "1");
            bool stopped = false; int result = 0;
            try { result = s.Pool.Allocate(s.Flags); } catch (Exception) { stopped = true; }
            Assert.Equal(row[4] != "0", stopped);
            uint[] actual = [(uint)result, s.Pool.FreeCount, s.Pool.PredictableCursor, (uint)s.Pool.FirstFree(true), (uint)s.Pool.Tail(true),
                (uint)s.Pool.FirstFree(false), (uint)s.Pool.Tail(false), X86Fixture.Adler32(s.Pool.Bytes), LogAdler(s.Pool)];
            Assert.Equal(row.Skip(5).Take(9).Select(X86Fixture.UInt), actual);
            Assert.Equal(X86Fixture.UInt(row[14]), s.Bridge); Assert.Equal(X86Fixture.UInt(row[15]), s.Disallow);
            Assert.Equal(row[16], s.Events.Count == 0 ? "-" : string.Join(';', s.Events)); observations++; return true;
        });
        Assert.Equal(47, observations);
    }

    /// <summary>선택된 live 후보의 회수 장치가 없을 때 선행 물리/카운터를 보존하고 그 위치에서 실패한다.</summary>
    [Fact]
    public void SelectedMissingReclaimKeepsPhysicalPrefix()
    {
        var s = new Scene(5, 2, true, true); SidAllocationBindings bindings = s.Bindings(); bindings.Reclaim = null; s.Pool.BindAllocation(bindings);
        byte[] before = s.Pool.Bytes.ToArray(); uint free = s.Pool.FreeCount;
        Assert.Throws<InvalidOperationException>(() => s.Pool.Allocate(s.Flags));
        Assert.Equal(before, s.Pool.Bytes.ToArray()); Assert.Equal(free, s.Pool.FreeCount);
    }

    /// <summary>현재 Host 회수 호출 동안 이전 Clear 증빙으로 대여를 반납할 수 없으며 복귀 뒤 같은 증빙을 사용할 수 있다.</summary>
    [Fact]
    public void ReclaimPinsBindingUntilSynchronousReturn()
    {
        var authority = new RawAuthorityState(); var pool = new RawSidPool(Capacity, true, () => authority.Server);
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        var destroy = new RawSquidDestroy(pool, new RawUnpop(pool, new(), new byte[65536]), types);
        var storage = new RawProcessHostStorage(pool, types, authority);
        var runtime = new ProcessHostRuntimeBindings { Destroy = destroy };
        using RawProcessHostStorage.Use use = storage.Bind(runtime); RawProcessHostStorage.ClearPass pass = use.ClearForState();
        int first = pool.Allocate(SidAllocation.Client); pool.RawSlot(first)[10] = 82; bool rejected = false; int calls = 0;
        runtime.Asset = new() { Emit = effect =>
        {
            Assert.Equal(SquidDestroyEffect.DestroyDependent, effect.Effect); Assert.Equal(first, effect.Sid); Assert.Equal(0u, effect.Flags);
            Assert.Equal(1ul, runtime.ActiveCalls); calls++;
            InvalidOperationException error = Assert.Throws<InvalidOperationException>(() => use.ReturnForState(pass)); rejected = error.Message.Contains("진행 중");
            pool.Release(effect.Sid);
        } };
        pool.BindAllocation(new() { Boss = () => authority.Boss, BridgeType = () => 82, Assertion = (_, _, _) => { }, Reclaim = storage.ReclaimForAllocation });
        SetHead(pool, true, pool.Tail(true)); Assert.Equal(14999, pool.Allocate(SidAllocation.Client));
        Assert.Equal(1, calls); Assert.True(rejected); Assert.Equal(0ul, runtime.ActiveCalls);
        _ = use.ReturnForState(pass); Assert.Equal(0ul, runtime.ActiveCalls);
    }

    /// <summary>실제 World의 Content 가상 삭제를 긴 Pool이 선택하고 반납 뒤 옛 대여를 호출하지 않는다.</summary>
    [Fact]
    public void ActualWorldReclaimsCurrentClassThroughLongPool()
    {
        OriginalTypeRecord[] types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray(); types[6].ConstructorAddress = OriginalConstructors.Address(6);
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        SquidDisplayShape[] shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray();
        var storage = new RawSurfaceStorage(OriginalEdition.Patch1078, types, frames, shapes); int assertions = 0;
        storage.BindDisplayDiagnostics(new(), (_, _, _) => assertions++);
        var world = new RawSurfaceWorld(OriginalEdition.Patch1078, storage.Types, 83, storage, frames); RawWorldCore core = world.Core();
        var content = new RawContentFormHost(core.Pool, core.Types, (_, _, _) => { });
        core.Forms.Bind(new RawContentLifetime(core.Pool, core.Types, content, core.Destroy, new(), new(), core.Forms.Hooks()));
        int first = core.Factory.CreateContained(2); uint before = core.Pool.FreeCount; SetHead(core.Pool, true, core.Pool.Tail(true));
        storage.Links.BridgeType = 0x10006;
        storage.BindDisplayDiagnostics(new(), (_, _, line) => { Assert.Equal(0x14au, line); throw new InvalidOperationException("전체DWORD 비교 뒤 예약 꼬리 진단"); });
        Assert.Throws<InvalidOperationException>(() => core.Pool.Allocate(SidAllocation.Client));
        Assert.Equal(0, core.Pool.Slot(first)[11] & 1); Assert.Equal(before, core.Pool.FreeCount);
        storage.Links.BridgeType = 6; storage.BindDisplayDiagnostics(new(), (_, _, _) => assertions++);
        int replacement = core.Factory.CreateContained(2); Assert.Equal(14999, replacement); Assert.Equal(5, first);
        Assert.Equal(before, core.Pool.FreeCount); Assert.Equal(first, core.Pool.FirstFree(true)); Assert.Equal(first, core.Pool.Tail(true));
        Assert.Equal(0, assertions); Assert.Equal(0u, core.Destroy.PreDepth); Assert.Equal(0u, core.Destroy.PostDepth); Assert.True(world.Destroy(replacement, 0x10));
        int residual = core.Factory.CreateContained(2); Assert.Equal(first, residual);
        world.ReturnProcessBindingsForState(world.ClearProcessesForState());
        SetHead(core.Pool, true, core.Pool.Tail(true)); Assert.Throws<InvalidOperationException>(() => core.Pool.Allocate(SidAllocation.Client));
    }

    /// <summary>장치 연결은 함수 사본을 보관하며 현재 owner를 읽고 진단 뒤 재확보한 물리에만 쓴다.</summary>
    [Fact]
    public void BindingCopiesCallbacksAndUsesCurrentPoolAfterDiagnostic()
    {
        var pool = new RawSidPool(Capacity); uint disallow = 1; int calls = 0; UIntBox cursor = pool.PredictableCursorBox;
        var binding = new SidAllocationBindings { Disallow = () => disallow, Assertion = (_, _, _) => { calls++; pool.Reset(32768); disallow = 0; } };
        ReadOnlySpan<byte> oldStorage = pool.Bytes; byte[] old = oldStorage.ToArray();
        pool.BindAllocation(binding); binding.Assertion = (_, _, _) => throw new InvalidOperationException("재대입한 외부 장치");
        Assert.Equal(5, pool.Allocate(SidAllocation.Client)); Assert.Equal(1, calls); Assert.Equal(32768, pool.Capacity); Assert.Same(cursor, pool.PredictableCursorBox);
        Assert.Equal(RawSidPool.VoidBit, pool.Slot(5)[11]); Assert.Equal((uint)(32768 - 6), pool.FreeCount); Assert.Equal(old, oldStorage.ToArray());
        Assert.Throws<ArgumentOutOfRangeException>(() => new RawSidPool(65536));
    }

    /// <summary>예측 카운터의 현재 전체 DWORD와 덧셈 넘침을 보존하며 Reset은 같은 owner만 갱신한다.</summary>
    [Fact]
    public void PredictableCursorSharesCurrentDwordAndResetKeepsOwner()
    {
        var pool = new RawSidPool(Capacity, server: false); UIntBox owner = pool.PredictableCursorBox;
        owner.Value = uint.MaxValue; Assert.Equal(23000, pool.Allocate(SidAllocation.Predictable)); Assert.Equal(0u, owner.Value);
        Assert.Equal(RawSidPool.VoidBit, pool.Slot(23000)[11]); pool.Reset();
        Assert.Same(owner, pool.PredictableCursorBox); Assert.Equal(1u, owner.Value); Assert.Equal(23002, pool.Allocate(SidAllocation.Predictable));
    }
}
