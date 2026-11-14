using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 독립 x86 보호막 자료와 현재 선택 장치·raw 부작용을 같은 실제 부모에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>최초 예약 두 번·일반 finder·Regular의 원문 수명과 현재 입력 계약이다.</summary>
public sealed class RawForcefieldLifecycleTests
{
    /// <summary>원본 입력의 같은 대상 보호막 SID다.</summary>
    private const int Target = 50;
    /// <summary>원본의 정확한DWORD 필드 폭으로 합성 raw 입력을 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>현재 TYPE 표의 기본188개 칸을 만든다.</summary>
    private static OriginalTypeRecord[] Types() => Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

    /// <summary>원본 PATCH1208입력의 예약/진행/삭제 순서·반환 비트·전체50BYTE를 대조한다.</summary>
    [Fact]
    public void OriginalPatch1208MatchesEventsReturnAndRaw()
    {
        string[][] all = X86Fixture.Read("forcefieldpostpop"); Assert.Equal(3624, all.Length);
        string[][] rows = all.Where(row => row[0] == "originals").ToArray(); Assert.Equal(1208, rows.Length);
        var pool = new RawSidPool(24000, false); var hash = new RawSquidHash(); OriginalTypeRecord[] types = Types();
        // 원본 후보61까지 실제 client 할당을 유지하며 매 행의 stale 후보도 같은 풀에 남긴다.
        for (int sid = 5; sid <= 61; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(16, row.Length); hash.Reset(); var events = new List<string>();
            // 현재 TYPE 표의 물리 칸을 같은 배열에 초기화한다.
            for (int index = 0; index < types.Length; index++) types[index] = new OriginalTypeRecord();
            Span<byte> raw = pool.RawSlot(Target); raw.Clear(); Put(raw, 0, ForcefieldLifecycle.PatchVtable); raw[10] = 167; raw[34] = 1;
            raw[40] = unchecked((byte)X86Fixture.UInt(row[12])); Put(raw, 14, X86Fixture.UInt(row[8])); Put(raw, 18, X86Fixture.UInt(row[9])); types[167].FootX = types[167].FootY = 1;
            int layout = X86Fixture.Int(row[11]); float x = X86Fixture.FloatBits(row[8]), y = X86Fixture.FloatBits(row[9]);
            // 후보의 상태/매장/소유자/발자국은 제품 finder가 직접 판단한다.
            void Add(int sid, byte type, byte state, byte extra, float ax, float ay, int width, byte owner, int level)
            {
                Span<byte> item = pool.RawSlot(sid); item.Clear(); Put(item, 0, 0x11010000); item[10] = type; item[11] = state; item[40] = extra; item[34] = owner;
                Put(item, 14, BitConverter.SingleToUInt32Bits(ax)); Put(item, 18, BitConverter.SingleToUInt32Bits(ay)); types[type].FootX = width; types[type].FootY = 1;
                ref ushort head = ref hash.Bucket(level, ax, ay); BinaryPrimitives.WriteUInt16LittleEndian(item[4..], head); head = (ushort)sid;
            }
            if (layout != 0)
            {
                float ax = (float)Math.Min(255.5, (double)x + (layout == 7 ? 1 : layout == 8 ? 2 : 0));
                Add(60, (byte)(layout is 2 or 9 ? 159 : 158), (byte)(layout == 4 ? 2 : layout == 5 ? 4 : 0), (byte)(layout == 3 ? 8 : 0), ax, y, layout == 7 ? 2 : 1, (byte)(layout == 6 ? 8 : 1), 1);
                if (layout == 9) Add(61, 158, 0, 0, x, y, 1, 2, 3);
            }
            string returned = "-";
            if (row[1] == "Prefix")
            {
                int allocations = 0;
                var prefix = new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks
                {
                    ReserveRegular = () => { events.Add("A:40"); return X86Fixture.UInt(row[allocations++ == 0 ? 3 : 4]) != 0; },
                    AddRegular = (sid, ev, payload) => events.Add($"R:{sid}:{ev}:{BitConverter.SingleToUInt32Bits(payload)}"),
                });
                uint flags = X86Fixture.UInt(row[2]); prefix.Prefix(Target, flags);
                var book = new RawPostPopState { Suppressed = true, Depth = 1 }; var body = new RawPostPop(pool, types, book);
                events.Add($"B:50:{flags}"); body.PostPopBase(Target, flags); Assert.Equal(0u, book.Depth);
            }
            else
            {
                var state = new ForcefieldRegularState { PriestType = X86Fixture.UInt(row[10]) };
                var regular = new RawForcefieldRegular(pool, hash, types, state, new ForcefieldRegularHooks
                {
                    Advance = (sid, delta, flags) => events.Add($"A:{sid}:{delta}:{flags}"),
                    Destroy = (sid, flags) => events.Add($"D:{sid}:{flags}"),
                });
                returned = BitConverter.SingleToUInt32Bits(regular.Handle(Target, X86Fixture.UInt(row[5]), X86Fixture.UInt(row[6]), X86Fixture.FloatBits(row[7]))).ToString("x8");
            }
            Assert.Equal(row[13], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(row[14], returned);
            Assert.Equal(X86Fixture.Bytes(row[15]), pool.Slot(Target).ToArray()); return true;
        });
    }

    /// <summary>PATCH 미선택 경로는 raw를 읽지 않고 첫 확보 실패 후 두 번째 확보를 계속한다.</summary>
    [Fact]
    public void PrefixRequiresOnlySelectedEffectsAndCopiesHooks()
    {
        var pool = new RawSidPool(24000, false);
        var empty = new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks()); empty.Prefix(65535, 0);
        Assert.Throws<InvalidOperationException>(() => empty.Prefix(Target, 1));
        int reserves = 0, adds = 0; var hooks = new ForcefieldPostPopHooks
        {
            ReserveRegular = () => ++reserves == 2,
            AddRegular = (sid, ev, payload) => { Assert.Equal(Target, sid); Assert.Equal(2u, ev); Assert.Equal(0x3c23d70au, BitConverter.SingleToUInt32Bits(payload)); adds++; pool.RawSlot(Target)[10] = 255; },
        };
        var body = new RawForcefieldPostPop(pool, hooks); hooks.ReserveRegular = null; hooks.AddRegular = null;
        body.Prefix(Target, 1); Assert.Equal(2, reserves); Assert.Equal(1, adds); Assert.Equal(255, pool.RawSlot(Target)[10]);
        Assert.Throws<ArgumentException>(() => body.Validate(Target));
    }

    /// <summary>타입 finder에는 raw float와 현재 전체DWORD가 전달되며 실패 앞단의 부작용도 남는다.</summary>
    [Fact]
    public void TypedFinderUsesCurrentDwordAndPreservesCompletedEffect()
    {
        var pool = new RawSidPool(24000, false); var hash = new RawSquidHash(); var types = Types();
        Span<byte> raw = pool.RawSlot(Target); Put(raw, 14, BitConverter.SingleToUInt32Bits(20.75f)); Put(raw, 18, BitConverter.SingleToUInt32Bits(21.9f)); raw[10] = 255; raw[11] = 9;
        var current = new UIntBox { Value = 158 }; var state = new ForcefieldRegularState { SharedPriestType = current }; int calls = 0;
        var hooks = new ForcefieldRegularHooks
        {
            FindTypeAt = (x, y, type) => { Assert.Equal(20.75f, x); Assert.Equal(21.9f, y); Assert.Equal(current.Value, type); calls++; Put(pool.RawSlot(Target), 26, 1234); return calls <= 2 ? 60 : 0; },
        };
        var body = new RawForcefieldRegular(pool, hash, types, state, hooks); hooks.FindTypeAt = null;
        Assert.Equal(ForcefieldLifecycle.CheckInterval, body.Handle(Target, 2, 0, 0)); current.Value = 0x1009e;
        Assert.Equal(ForcefieldLifecycle.CheckInterval, body.Handle(Target, 2, 0, 0));
        Assert.Throws<InvalidOperationException>(() => body.Handle(Target, 2, 0, 0)); Assert.Equal(3, calls); Assert.Equal(1234u, BinaryPrimitives.ReadUInt32LittleEndian(pool.RawSlot(Target)[26..]));
    }

    /// <summary>알 수 없는 사건 진단은 현재checking에서만 선택하고 부모를 읽거나 검사하지 않는다.</summary>
    [Fact]
    public void CurrentCheckingSelectsDiagnosticWithoutRawRead()
    {
        var pool = new RawSidPool(24000, false); uint checking = 0; int diagnostics = 0, advances = 0;
        var state = new ForcefieldRegularState { Debug = new RawCheckingFlag(() => checking) };
        var body = new RawForcefieldRegular(pool, new RawSquidHash(), Types(), state, new ForcefieldRegularHooks
        {
            Assertion = (expression, file, line) => { Assert.Equal("0", expression); Assert.Equal(@".\Dais.cpp", file); Assert.Equal(0x479u, line); diagnostics++; },
            Advance = (sid, delta, flags) => { Assert.Equal(65535, sid); Assert.Equal(1, delta); Assert.Equal(0u, flags); advances++; },
        });
        Assert.Equal(0f, body.Handle(65535, 99, 0, 0)); checking = 0x80000000;
        Assert.Equal(0f, body.Handle(65535, 99, 0, 0)); Assert.Equal(1, diagnostics);
        Assert.Equal(ForcefieldLifecycle.FrameInterval, body.Handle(65535, 1, 0, 0)); Assert.Equal(1, advances);
    }
    /// <summary>CD는 기존 구성·객체 사전 검사를 유지하고 알 수 없는 사건에서 PATCH 진단을 추가하지 않는다.</summary>
    [Fact]
    public void CdKeepsConstructionObjectPreflightAndNoUnknownDiagnostic()
    {
        var pool = new RawSidPool(24000, false, edition: OriginalEdition.Cd1072); var hash = new RawSquidHash();
        var types = Enumerable.Range(0, 171).Select(_ => new OriginalTypeRecord()).ToArray(); var state = new ForcefieldRegularState { Debug = new RawCheckingFlag(true) };
        Assert.Throws<ArgumentException>(() => new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks()));
        Assert.Throws<ArgumentException>(() => new RawForcefieldRegular(pool, hash, types, state, new ForcefieldRegularHooks()));
        int calls = 0; var prefix = new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks { ReserveRegular = () => { calls++; return false; }, AddRegular = (_, _, _) => Assert.Fail("CD 미선택 생성") });
        Assert.Throws<ArgumentException>(() => prefix.Prefix(Target, 0)); Assert.Equal(0, calls);
        Span<byte> raw = pool.RawSlot(Target); raw.Clear(); Put(raw, 0, ForcefieldLifecycle.CdVtable); raw[10] = 167;
        var body = new RawForcefieldRegular(pool, hash, types, state, new ForcefieldRegularHooks { Advance = (_, _, _) => Assert.Fail("CD 미선택 진행"), Destroy = (_, _) => Assert.Fail("CD 미선택 삭제"), Assertion = (_, _, _) => Assert.Fail("CD 미선택 진단") });
        Assert.Equal(0f, body.Handle(Target, 99, 0, 0)); raw[10] = 74;
        Assert.Throws<ArgumentException>(() => body.Handle(Target, 99, 0, 0));
    }

    /// <summary>첫 생성 효과가 raw를 바꾸거나 마지막 소유자를 놓아도 두 번째 원문 예약·생성을 계속한다.</summary>
    [Fact]
    public void PrefixContinuesAfterCallbackChangesRawAndReleasesOwner()
    {
        var pool = new RawSidPool(24000, false); RawForcefieldPostPop? owner = null; var events = new List<uint>(); int reserves = 0;
        owner = new RawForcefieldPostPop(pool, new ForcefieldPostPopHooks
        {
            ReserveRegular = () => { reserves++; return true; },
            AddRegular = (sid, ev, payload) => { events.Add(ev); pool.RawSlot(sid)[10] = 255; pool.RawSlot(sid)[11] = 9; owner = null; },
        });
        owner.Prefix(Target, 1); Assert.Null(owner); Assert.Equal(2, reserves); Assert.Equal(new uint[] { 1, 2 }, events); Assert.Equal(9, pool.RawSlot(Target)[11]);
    }

}
