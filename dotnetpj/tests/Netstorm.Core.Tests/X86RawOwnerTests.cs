using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 소유자 지정(<see cref="SquidOwner"/>)을 원본 x86 기대값에 대조한다.
/// 기대값은 owner-x86.tsv(소유자 변경, 판본별 1,000회) 중 10.78 몫이다.
/// 각 호출 뒤 소유자 바이트·소유자별 목록·대상 슬롯 체크섬을 비교한다.
/// 열 구성은 cpppj/tests/OwnerTests.cpp 와 같다.
/// CD 10.72·10.37 행은 기준 판본이 아니므로 읽지 않는다 (W02 Owner 조각, 2026-10-20).
/// </summary>
public sealed class X86RawOwnerTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>판본별 호출 수. 전체 3,000행 중 10.78 몫이다.</summary>
    private const int RowsPerEdition = 1000;

    /// <summary>관찰 대상 타입 번호.</summary>
    private const int ObjectType = 82;

    /// <summary>검사 대상 번호.</summary>
    private const int Target = 50;

    /// <summary>소유자 수(0~8번).</summary>
    private const int OwnerCount = 9;

    /// <summary>한 소유자 목록의 관찰 칸 수.</summary>
    private const int ListSlots = 6;

    /// <summary>한 행의 입력으로 풀·장부·모드를 준비하고 소유자 지정을 실행한다.</summary>
    /// <param name="row">fixture 행(14칸).</param>
    /// <param name="pool">슬롯 풀(5~70번 확보).</param>
    /// <param name="book">공유 장부.</param>
    /// <param name="owners">부류별 소유자 지정 캐시.</param>
    /// <param name="typeSets">부류별 타입 표 캐시.</param>
    private static void Run(string[] row, RawSidPool pool, RawPostPopState book, Dictionary<uint, OriginalTypeRecord[]> typeSets)
    {
        var mode = new SquidOwnerMode
        {
            Fort = row[2] != "0",
            Battle = row[3] != "0",
            Challenge = row[4] != "0",
        };
        Span<byte> raw = pool.AllocatedBytes(Target);
        raw.Clear();
        raw[RawSidPool.TypeOffset] = ObjectType;
        raw[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(row[6]);
        raw[34] = (byte)X86Fixture.UInt(row[8]);
        raw[40] = (byte)X86Fixture.UInt(row[7]);
        // 소유자마다 "자리:개수:값 6개"를 읽어 자리만큼을 실제 배열로, 나머지를 stale 보관으로 둔다
        string[] lists = row[10].Split('|');
        Assert.Equal(OwnerCount, lists.Length);
        var stale = new List<uint>[OwnerCount];
        // 소유자마다 기대 목록 문자열을 나눠 stale 배열을 만든다
        for (int owner = 0; owner < OwnerCount; owner++)
        {
            string[] parts = lists[owner].Split(':');
            string[] entries = parts[2].Split(',');
            Assert.Equal(ListSlots, entries.Length);
            uint capacity = X86Fixture.UInt(parts[0]);
            RawPostPopList list = book.OwnerFactories[owner];
            list.Entries = new uint[capacity];
            stale[owner] = [];
            // 목록 슬롯마다 기대 값을 읽어 용량 안에서만 채운다
            for (int i = 0; i < ListSlots; i++)
            {
                uint value = X86Fixture.UInt(entries[i]);
                if ((uint)i < capacity)
                {
                    list.Entries[i] = value;
                }
                else
                {
                    stale[owner].Add(value);
                }
            }
            list.Count = X86Fixture.UInt(parts[1]);
        }
        uint genus = X86Fixture.UInt(row[5]);
        // 같은 부류는 같은 타입 표를 재사용한다
        if (!typeSets.TryGetValue(genus, out OriginalTypeRecord[]? cached))
        {
            var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < types.Length; i++)
            {
                types[i] = new OriginalTypeRecord();
            }
            types[ObjectType].Flags2 = genus;
            typeSets[genus] = types;
            cached = types;
        }
        // 모드는 행마다 바뀌므로 현재 행의 모드로 묶는다
        var holder = new SquidOwner(pool, cached, book, mode);
        holder.Set(Target, X86Fixture.UInt(row[9]));
        // 결과: 소유자 바이트·소유자마다 "개수:값 나열(배열 전체+stale)"·슬롯 Adler-32
        var actual = new List<string>();
        // 소유자마다 최종 목록 전체(배열 + stale)를 모아 기대 문자열과 비교할 준비를 한다
        for (int owner = 0; owner < OwnerCount; owner++)
        {
            RawPostPopList list = book.OwnerFactories[owner];
            var values = new List<uint>(list.Entries);
            values.AddRange(stale[owner]);
            actual.Add($"{list.Count}:{string.Join(',', values)}");
        }
        ReadOnlySpan<byte> after = pool.Slot(Target);
        Assert.Equal(X86Fixture.UInt(row[11]), after[34]);
        Assert.Equal(row[12], string.Join('|', actual));
        Assert.Equal(X86Fixture.UInt(row[13]), X86Fixture.Adler32(after));
    }

    /// <summary>
    /// 소유자 지정 fixture 의 10.78 몫 1,000회를 재생한다.
    /// 호출마다 소유자 바이트·목록·슬롯 체크섬을 대조한다.
    /// </summary>
    [Fact]
    public void OwnerMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("owner");
        int count = 0;
        var pool = new RawSidPool(24000, false);
        // 원본 관찰의 번호 범위를 클라이언트 영역에 확보한다
        for (int id = 5; id <= 70; id++)
        {
            Assert.Equal(id, pool.Allocate(SidAllocation.Client));
        }
        var book = new RawPostPopState();
        var typeSets = new Dictionary<uint, OriginalTypeRecord[]>();
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[1] != PatchEdition)
            {
                return true;
            }
            Assert.Equal(14, row.Length);
            Assert.Equal("Owner", row[0]);
            Run(row, pool, book, typeSets);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
