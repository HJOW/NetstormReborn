using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 두 PE의 독립 섬 검색 반환(<see cref="RawIslandQueries"/>)과 전체 3204BYTE 불변을 같은 mutable 목록에서 대조한다.
/// cpppj <c>IslandQueriesTests</c>의 장면 3종(패치 4245행, CD 4245행, 라이브 레코드 조회·물리 초과 거부)을 옮겼다. (F2c-12, 2026-11-07)
/// </summary>
public sealed class IslandQueriesTests
{
    /// <summary>리틀 엔디안 정수를 바이트 스팬의 지정 오프셋에 기록한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="raw">기록할 대상 버퍼다.</param>
    /// <param name="offset">바이트 오프셋이다.</param>
    /// <param name="value">기록할 부호 없는 32비트 값이다.</param>
    /// <param name="width">기록할 바이트 너비다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 하위 바이트부터 지정 너비만큼 순서대로 기록한다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 실제 월드의 단일 3204BYTE·CoreData 모드와 기존 runtime에 검색 몸체를 연결하는 입력 장면이다.
    /// (F2c-12, 2026-11-07)
    /// </summary>
    private sealed class IslandQueryScene
    {
        /// <summary>runtime의 판본과 실제 풀 동일성을 제공한다. (F2c-12, 2026-11-07)</summary>
        public RawSidPool Pool;

        /// <summary>기존 runtime의 같은 hash0다. (F2c-12, 2026-11-07)</summary>
        public RawSquidHash Hash = new();

        /// <summary>월드가 장기 소유하는 실제 mutable 100기록(3204바이트)이다. (F2c-12, 2026-11-07)</summary>
        public byte[] Records = new byte[RawIslandStorage.Bytes];

        /// <summary>원본 fort·special·chaff의 같은 현재 자료다. (F2c-12, 2026-11-07)</summary>
        public FortFinishOwnerState Modes = new();

        /// <summary>owner setter에 필요한 같은 거주 목록이며 세 검색은 읽지 않는다. (F2c-12, 2026-11-07)</summary>
        public RawPostPopList Residences = new();

        /// <summary>기존 runtime의 공유 지도 리비전 전역이다. (F2c-12, 2026-11-07)</summary>
        public uint Revision;

        /// <summary>기존 runtime의 공유 월드 다시 그리기 전역이다. (F2c-12, 2026-11-07)</summary>
        public uint Changed;

        /// <summary>완료된 count·Exists·현재 owner getter를 제공하는 섬 런타임이다. (F2c-12, 2026-11-07)</summary>
        public RawIslandRuntime Runtime;

        /// <summary>위 runtime·현재 모드를 참조하는 세 복원 검색이다. (F2c-12, 2026-11-07)</summary>
        public RawIslandQueries Queries;

        /// <summary>같은 풀·기록·모드 수명을 검색과 runtime보다 길게 유지한다. (F2c-12, 2026-11-07)</summary>
        /// <param name="edition">원본 판본이다.</param>
        public IslandQueryScene(OriginalEdition edition)
        {
            Pool = new RawSidPool(32768, false, null, edition);
            Runtime = new RawIslandRuntime(
                Pool,
                Hash,
                Records,
                Modes,
                Residences,
                () => Changed,
                value => Changed = value,
                () => Revision,
                value => Revision = value,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (_, _) => throw new InvalidOperationException("IslandQueries 예상밖 가상 owner"),
                    Assertion = _ => throw new InvalidOperationException("IslandQueries 예상밖 진단"),
                });
            Queries = new RawIslandQueries(Runtime, Modes);
        }

        /// <summary>명시 원본 입력만 구성하며 검색 반환·일치 수는 계산하지 않는다. (F2c-12, 2026-11-07)</summary>
        /// <param name="input">12개 입력 DWORD 배열이다.</param>
        public void Prepare(IReadOnlyList<uint> input)
        {
            Assert.Equal(12, input.Count);
            Records.AsSpan().Fill(0xA5);
            Put(Records, 0, input[0]);
            // 실제 100기록의 신전·존재·current DWORD·원래 BYTE·territory BYTE와 padding을 각각 배치한다
            for (int index = 0; index < 100; index++)
            {
                int record = 4 + index * 32;
                Put(Records, record, (uint)(0x11110000 + index));
                Put(Records, record + 4, 0);
                Put(Records, record + 8, input[3]);
                Put(Records, record + 12, input[5], 1);
                Put(Records, record + 13, input[4], 1);
                if (input[1] == 1)
                {
                    Put(Records, record + 4, index % 3 != 0 ? input[2] : 0u);
                }
                if (input[1] == 2)
                {
                    Put(Records, record + 4, index == 0 || index == 2 || index == 99 ? input[2] : 0u);
                    if (index % 2 != 0)
                    {
                        Put(Records, record + 8, input[3] ^ 0x01000100u);
                        Put(Records, record + 13, input[4] ^ 0x80u, 1);
                        Put(Records, record + 12, input[5] ^ 0x80u, 1);
                    }
                }
            }
            Put(Records, 4 + (int)input[11] * 32 + 4, input[2]);
            Modes.Fort = input[8] != 0;
            Modes.Special = input[9] != 0;
            Modes.ChaffMode = input[10] != 0;
        }
    }

    /// <summary>양판본 각 4245개 검색과 전체 raw·세 모드·revision 불변을 비교한다. (F2c-12, 2026-11-07)</summary>
    /// <param name="edition">대상 판본 이름(<c>originals</c> 또는 <c>originalCD</c>)이다.</param>
    private static void ReplayIslandQueries(string edition)
    {
        IslandQueryScene scene = new(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        string[][] rows = X86Fixture.Read("islandqueries");
        int count = 0;
        // 원본에서 독립 실행한 모든 검색 입력을 같은 단일 runtime에 전달한다
        foreach (string[] row in rows)
        {
            if (row[0] != edition)
            {
                continue;
            }
            Assert.Equal(6, row.Length);
            string[] numberCells = row[2].Split(',');
            uint[] input = new uint[numberCells.Length];
            // 실행기에 전달한 DWORD 비트를 그대로 읽는다
            for (int index = 0; index < numberCells.Length; index++)
            {
                input[index] = X86Fixture.UInt(numberCells[index]);
            }

            scene.Prepare(input);
            byte[] before = scene.Records.ToArray();
            uint result = row[1] switch
            {
                "FindOwnerTerritory" => scene.Queries.FindOwnerTerritory(unchecked((int)input[6]), unchecked((int)input[7])),
                "FindTerritory" => scene.Queries.FindTerritory(unchecked((int)input[7])),
                "CountOwned" => scene.Queries.CountOwned(input[6]),
                _ => throw new InvalidOperationException("알 수 없는 검색 종류: " + row[1]),
            };

            string modes = string.Create(
                CultureInfo.InvariantCulture,
                $"{(scene.Modes.Fort ? 1 : 0)},{(scene.Modes.Special ? 1 : 0)},{(scene.Modes.ChaffMode ? 1 : 0)}");
            Assert.Equal(X86Fixture.UInt(row[3]), result);
            Assert.True(scene.Records.AsSpan().SequenceEqual(before));
            Assert.Equal(X86Fixture.UInt(row[4]), X86Fixture.Adler32(scene.Records));
            Assert.Equal(row[5], modes);
            Assert.Equal(0u, scene.Revision);
            Assert.Equal(0u, scene.Changed);
            count++;
        }
        Assert.Equal(4245, count);
    }

    /// <summary>10.78의 세 원본 검색 4245개를 전수 비교한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandQueriesReplaysPatch() => ReplayIslandQueries("originals");

    /// <summary>CD 10.72의 실제 Exists·validId·Owner 호출을 포함한 4245개를 전수 비교한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandQueriesReplaysCd() => ReplayIslandQueries("originalCD");

    /// <summary>실제 Add·Delete·Reset과 현재 fort가 다음 검색에 즉시 반영되고 물리 100기록 밖 접근을 거부하는지 확인한다. (F2c-12, 2026-11-07)</summary>
    [Fact]
    public void IslandQueriesReadsLiveRecordsAndRejectsPhysicalOverflow()
    {
        OriginalEdition[] editions = [OriginalEdition.Patch1078, OriginalEdition.Cd1072];
        // 두 판본의 같은 3204BYTE mutable 자료를 복사 벡터 없이 재사용한다
        foreach (OriginalEdition edition in editions)
        {
            IslandQueryScene scene = new(edition);
            scene.Runtime.Reset();
            Assert.Equal(127u, scene.Queries.FindTerritory(2));
            Assert.Equal(0u, scene.Queries.CountOwned(0));
            Assert.Equal(0u, scene.Runtime.Add(1, 2));
            Assert.Equal(1u, scene.Runtime.Add(2, 2));
            Assert.Equal(1u, scene.Queries.CountOwned(1));
            Assert.Equal(0u, scene.Queries.FindOwnerTerritory(1, 2));

            Put(scene.Records, 4 + 8, 2);
            Assert.Equal(127u, scene.Queries.FindOwnerTerritory(1, 2));
            Assert.Equal(2u, scene.Queries.CountOwned(2));

            scene.Modes.Fort = true;
            Put(scene.Records, 4 + 8, 0);
            Assert.Equal(0u, scene.Queries.FindOwnerTerritory(7, 2));
            Assert.Equal(1u, scene.Queries.CountOwned(0));

            scene.Runtime.Delete(0);
            Assert.Equal(2u, scene.Runtime.Count());
            Assert.Equal(1u, scene.Queries.FindTerritory(2));

            scene.Runtime.Reset();
            Assert.Equal(127u, scene.Queries.FindOwnerTerritory(0, 2));
            Assert.Equal(0u, scene.Queries.CountOwned(0));

            Put(scene.Records, 0, 0xFFFFFFFFu);
            Assert.Equal(127u, scene.Queries.FindTerritory(2));
            Assert.Equal(0u, scene.Queries.CountOwned(0));

            Put(scene.Records, 0, 101);
            Assert.Throws<ArgumentOutOfRangeException>(() => scene.Queries.CountOwned(0));
        }
    }
}
