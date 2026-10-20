using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 전체 풀 재구성에서 내부 Form 을 생략하는 경로(<see cref="RawGraph.Rebuild(bool)"/>)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/GraphFormsTests.cpp 와 같다: 실제 슬롯 7개·표 1,530바이트·시드 스택 위에서 재구성한 뒤
/// 슬롯 전체·표 전체·스택 전체를 비교한다. 기준 판본은 10.78 이며 CD 행은 읽지 않는다 (W02 열여섯째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawGraphFormsTests
{
    /// <summary>10.78 판본의 이름. CD 행은 건너뛴다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>리틀 엔디언 hex 를 바이트로 읽는다.</summary>
    /// <param name="hex">hex 문자열.</param>
    private static byte[] Bytes(string hex) => Convert.FromHexString(hex);

    /// <summary>1,530바이트 hex 를 표 레코드 255개로 읽는다.</summary>
    /// <param name="hex">hex 문자열.</param>
    private static GraphRecord[] Records(string hex)
    {
        byte[] bytes = Bytes(hex);
        var records = new GraphRecord[RawGraph.TableSize];
        // 레코드마다 세 WORD 를 부호 있는 값으로 읽는다
        for (int i = 0; i < records.Length; i++)
        {
            records[i] = new GraphRecord
            {
                Surfaces = BitConverter.ToInt16(bytes, i * 6),
                InUse = BitConverter.ToInt16(bytes, i * 6 + 2),
                Reserved = BitConverter.ToInt16(bytes, i * 6 + 4),
            };
        }
        return records;
    }

    /// <summary>표 전체를 바이트로 늘어놓는다.</summary>
    /// <param name="records">표 레코드.</param>
    private static byte[] RecordBytes(IReadOnlyList<GraphRecord> records)
    {
        var bytes = new byte[RawGraph.TableSize * 6];
        // 레코드마다 세 WORD 를 쓴다
        for (int i = 0; i < records.Count; i++)
        {
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6), records[i].Surfaces);
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6 + 2), records[i].InUse);
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 6 + 4), records[i].Reserved);
        }
        return bytes;
    }

    /// <summary>스택 전체를 바이트로 늘어놓는다.</summary>
    /// <param name="stack">스택.</param>
    private static byte[] StackBytes(IReadOnlyList<uint> stack)
    {
        var bytes = new byte[RawGraph.FloodSize * 4];
        // DWORD 마다 네 바이트를 쓴다
        for (int i = 0; i < stack.Count; i++)
        {
            BitConverter.TryWriteBytes(bytes.AsSpan(i * 4), stack[i]);
        }
        return bytes;
    }

    /// <summary>내부 Form 을 포함한 전체 풀 재구성 10.78 몫 32행을 재생한다.</summary>
    [Fact]
    public void FullPoolRebuildSkipsNonSpatialFormsMatchOriginalX86()
    {
        string[][] rows = X86Fixture.ReadFile("graphforms-x86.tsv");
        int count = 0;
        // 빈 줄·주석은 X86Fixture 가 이미 걸러낸다
        foreach (string[] row in rows)
        {
            if (row[0] != PatchEdition)
            {
                continue;
            }
            Assert.Equal(11, row.Length);
            bool resetAll = row[1] != "0";
            int[] expectedIds = row[4].Split(',').Select(int.Parse).ToArray();
            string[] before = row[5].Split(';');
            string[] after = row[6].Split(';');
            Assert.Equal(expectedIds.Length, before.Length);
            Assert.Equal(expectedIds.Length, after.Length);
            var pool = new RawSidPool(32768, false);
            var hash = new RawSquidHash();
            var ids = new List<int>();
            // 실제 client 할당 순서가 원본과 같은지 확인한다
            for (int i = 0; i < expectedIds.Length; i++)
            {
                int id = pool.Allocate(SidAllocation.Client);
                Assert.Equal(expectedIds[i], id);
                ids.Add(id);
                byte[] bytes = Bytes(before[i]);
                Assert.Equal(pool.Slot(id).Length, bytes.Length);
                bytes.CopyTo(pool.AllocatedBytes(id));
            }
            hash.Bucket(0, 20, 20) = (ushort)ids[0];
            var spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
            var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 장면의 base 초기화는 표면 판정과 무관한 값이다
            for (int i = 0; i < types.Length; i++)
            {
                types[i] = new OriginalTypeRecord { MaxHitPoints = 91, ZOrder = 7, FootX = 1, FootY = 1, Cost = 1.25f };
            }
            types[74].Flags1 = OriginalTypeRecord.Surface;
            types[74].Flags2 = 2;
            types[74].FootX = 1;
            types[74].FootY = 1;
            List<IReadOnlyList<FrameCode>> frames = Enumerable.Range(0, types.Length)
                .Select(_ => (IReadOnlyList<FrameCode>)new List<FrameCode>()).ToList();
            frames[74] = new List<FrameCode>
            {
                new FrameCode('A', 'P', 1, FrameCodeFlags.None),
                new FrameCode('A', 'P', 2, FrameCodeFlags.None),
            };
            uint seed = X86Fixture.UInt(row[9]);
            var stack = new uint[RawGraph.FloodSize];
            // 오래된 스택 흔적을 같은 시드로 채운다
            for (int i = 0; i < stack.Length; i++)
            {
                stack[i] = unchecked(seed + (uint)i);
            }
            var graph = new RawGraph(pool, hash, spots, types, Records(row[7]), stack, frames);
            graph.Rebuild(resetAll);
            // 내부 Form 의 NaN·stale 필드를 포함한 슬롯 전체를 비교한다
            for (int i = 0; i < ids.Count; i++)
            {
                string actualSlot = Convert.ToHexStringLower(pool.Slot(ids[i]));
                if (actualSlot != after[i])
                {
                    throw new InvalidOperationException($"슬롯 {ids[i]} 바이트 불일치");
                }
            }
            string actualTable = Convert.ToHexStringLower(RecordBytes(graph.Records));
            if (actualTable != row[8])
            {
                throw new InvalidOperationException("표 전체 바이트 불일치");
            }
            string actualFlood = Convert.ToHexStringLower(StackBytes(graph.FloodStack));
            if (actualFlood != row[10])
            {
                throw new InvalidOperationException("스택 전체 바이트 불일치");
            }
            Assert.Equal(ids[0], hash.Bucket(0, 20, 20));
            Assert.All(spots, spot => Assert.Equal(0, spot));
            count++;
        }
        Assert.Equal(32, count);
    }
}
