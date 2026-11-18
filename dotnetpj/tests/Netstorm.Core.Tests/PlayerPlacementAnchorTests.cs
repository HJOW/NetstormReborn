// Player 조회·거리 선택·그래프와 원본 임시 후보 목록을 독립 fixture로 대조한다.
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// RawPlayerPlacementAnchor의 collector·그래프·거리 선택을 검증하는 테스트 클래스다. 2026-11-18 0.610에서 R4-2 농지 잔여의 MayPlace 실결선용으로 추가했다.
public sealed class PlayerPlacementAnchorTests
{
    /// <summary>판본별 원본 관찰 행 수다. 2026-11-18 0.610에서 추가했다.</summary>
    private const int Rows = 1598;

    /// <summary>같은 가상 슬롯 범위다. 2026-11-18 0.610에서 추가했다.</summary>
    private const int Slots = 128;

    /// <summary>원본이 기록한 기대 행을 읽는다. C++ 판단을 다시 계산하지 않는다.</summary>
    /// <returns>판본 이름이 첫 칸인 행 목록이다.</returns>
    private static string[][] Fixture() => X86Fixture.Read("playeranchor");

    /// <summary>같은 십진수 문자열을 원본 DWORD로 복원한다.</summary>
    /// <param name="value">십진수 문자열이다.</param>
    private static uint Number(string value) => uint.Parse(value, CultureInfo.InvariantCulture);

    /// <summary>탭 fixture의 쉼표 DWORD 목록을 읽되 빈 입력은 빈 목록으로 유지한다.</summary>
    /// <param name="text">쉼표로 나눈 십진수 목록이다.</param>
    /// <returns>순서·중복을 지우지 않은 번호 목록이다.</returns>
    private static List<uint> Numbers(string text)
    {
        var result = new List<uint>();
        if (string.IsNullOrEmpty(text))
        {
            return result;
        }

        // 입력 번호만 옮기며 순서·중복은 지우지 않는 반복문이다.
        foreach (string part in text.Split(','))
        {
            result.Add(Number(part));
        }

        return result;
    }

    /// <summary>원본 little endian 슬롯 필드에 값을 쓴다.</summary>
    /// <param name="raw">쓸 슬롯 범위다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 너비만큼 쓰는 반복문이다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>같은 raw/type/목록/지도 입력에서 C++ 반환·후보 순서·읽기 전용 자료를 대조한다.</summary>
    /// <param name="name">판본 이름이다. originals·originalCD·original1037 중 하나다.</param>
    private static void Replay(string name)
    {
        OriginalEdition edition = name == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
        bool patch = edition == OriginalEdition.Patch1078;
        var pool = new RawSidPool(24000, false, null, edition);
        var types = new OriginalTypeRecord[patch ? 188 : 171];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        ushort[] surfaces = new ushort[65536];
        var bookkeeping = new RawPostPopState();
        var additional = new RawPostPopList();
        var contained = new ContainedFinderState();
        var state = new PlayerPlacementAnchorState();
        var anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, bookkeeping, additional, contained, state);
        byte[] mapBytes = new byte[131072];
        string[][] rows = Fixture();
        Assert.Equal(3 * Rows, rows.Length);
        int count = 0;
        // 판본별 모든 입력의 물리 자료를 원본 실행기와 같은 초기 상태로 만드는 반복문이다.
        foreach (string[] row in rows)
        {
            if (row[0] != name)
            {
                continue;
            }

            Assert.Equal(20, row.Length);
            int stride = pool.SlotStride;
            // 첫 128슬롯을 원본 실행기와 같은 0으로 만드는 반복문이다.
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.RawSlot(sid).Fill(0);
            }

            types[82].Flags1 = 0x800;
            types[82].Flags2 = Number(row[7]);
            types[83].Flags1 = types[83].Flags2 = 0;
            state.GraphReady = Number(row[4]);
            state.InvalidGraph = Number(row[5]);
            state.IgnoreRestrictions = Number(row[6]);
            uint owner = Number(row[2]);
            List<uint> workshops = Numbers(row[12]), extras = Numbers(row[13]);
            bookkeeping.OwnerFactories[owner].Entries = workshops.ToArray();
            bookkeeping.OwnerFactories[owner].Count = (uint)workshops.Count;
            additional.Entries = extras.ToArray();
            additional.Count = (uint)extras.Count;
            ushort surface = Number(row[9]) != 0 ? (ushort)116 : (ushort)0;
            Array.Fill(surfaces, surface);
            // native 지도와 같은 little endian 바이트를 checksum 대조용으로 준비하는 반복문이다.
            for (int i = 0; i < surfaces.Length; ++i)
            {
                mapBytes[2 * i] = (byte)surfaces[i];
                mapBytes[2 * i + 1] = (byte)(surfaces[i] >> 8);
            }

            // 실제 raw 입력의 head/next와 겹치는 종속 kind/mask까지 같은 순서로 쓰는 반복문이다.
            foreach (string entry in row[14].Split(';', StringSplitOptions.RemoveEmptyEntries))
            {
                List<uint> node = Numbers(entry);
                Assert.Equal(10, node.Count);
                int offset = (int)node[0] * stride;
                Span<byte> slot = pool.RawSlot((int)node[0]);
                slot[10] = (byte)node[1];
                slot[patch ? 34 : 32] = (byte)node[2];
                slot[patch ? 30 : 28] = (byte)node[3];
                Put(slot, 14, node[4], 4);
                Put(slot, 18, node[5], 4);
                Put(slot, 6, node[6], 2);
                Put(slot, 4, node[7], 2);
                if (node[1] == 5 || node[1] == 6)
                {
                    Put(slot, 18, node[8], 2);
                    Put(slot, 20, node[9], 2);
                }
            }

            float x = BitConverter.UInt32BitsToSingle(Number(row[10])), y = BitConverter.UInt32BitsToSingle(Number(row[11]));
            var observed = new PlayerAnchorSelection();
            if (row[1] == "L")
            {
                observed = anchor.Inspect(owner, Number(row[3]), x, y);
            }
            else if (row[1] == "G")
            {
                observed.Sid = anchor.GraphAt(x, y);
            }
            else if (row[1] == "S")
            {
                observed.Sid = anchor.GraphFor(Number(row[15]));
            }
            else
            {
                observed.Sid = anchor.Nearest(workshops, owner, x, y);
            }

            // 첫 128슬롯의 물리 바이트를 원본 checksum과 같은 순서로 모으는 반복문이다. CD는 앞 36바이트 packing이다.
            int slotBytes = patch ? 50 : 36;
            byte[] rawBytes = new byte[Slots * slotBytes];
            for (int sid = 0; sid < Slots; ++sid)
            {
                pool.Slot(sid).Slice(0, slotBytes).CopyTo(rawBytes.AsSpan(sid * slotBytes, slotBytes));
            }

            List<uint> expected = Numbers(row[17]);
            bool same = observed.Sid == Number(row[16]) && observed.Candidates.SequenceEqual(expected)
                && X86Fixture.Adler32(rawBytes) == Number(row[18]) && X86Fixture.Adler32(mapBytes) == Number(row[19]);
            Assert.True(same, $"앵커 {name} 행 {count}({row[1]}) 불일치 sid {observed.Sid}/{row[16]} 후보 {observed.Candidates.Count}/{expected.Count} raw {X86Fixture.Adler32(rawBytes)}/{row[18]} map {X86Fixture.Adler32(mapBytes)}/{row[19]}");
            ++count;
        }

        Assert.Equal(Rows, count);
    }

    /// <summary>패치 전체 collector·실제 거리/그래프/contained 반환을 대조한다.</summary>
    [Fact]
    public void PatchX86()
    {
        Replay("originals");
    }

    /// <summary>CD 임시 배열 정리와 BYTE graph/owner 반환을 대조한다.</summary>
    [Fact]
    public void CdX86()
    {
        Replay("originalCD");
    }

    /// <summary>추가 판본의 실제 명령으로 기록한 모든 입력을 재생한다.</summary>
    [Fact]
    public void ThirdX86()
    {
        Replay("original1037");
    }

    /// <summary>손상된 자료는 실제 접근 시 진단하고 무효 그래프/빈 목록의 조기 경로는 유지한다.</summary>
    [Fact]
    public void RequiredDataAndLazyGuards()
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var other = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        OriginalTypeRecord[] types = new OriginalTypeRecord[188];
        // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
        for (int i = 0; i < types.Length; ++i)
        {
            types[i] = new OriginalTypeRecord();
        }

        ushort[] surfaces = new ushort[65536];
        var bookkeeping = new RawPostPopState();
        var additional = new RawPostPopList();
        var contained = new ContainedFinderState();
        var state = new PlayerPlacementAnchorState();
        var anchor = new RawPlayerPlacementAnchor(pool, types, surfaces, bookkeeping, additional, contained, state);
        Assert.Equal(0u, anchor.Locate(0xffffffff, 0xffffffff, 20f, 21f));
        Assert.Throws<ArgumentException>(() => RawPlayerPlacementAnchor.MakePlayerAnchorQuery(other, anchor));
        Assert.Throws<ArgumentException>(() => new RawPlayerPlacementAnchor(pool, types, Array.Empty<ushort>(), bookkeeping, additional, contained, state));
        state.InvalidGraph = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => anchor.Locate(9, 82, 20f, 21f));
        additional.Count = 1;
        Assert.Equal(0u, anchor.Locate(1, 0xffffffff, 20f, 21f));
        bookkeeping.OwnerFactories[1].Count = 1;
        Assert.Throws<InvalidOperationException>(() => anchor.Locate(1, 82, 20f, 21f));
        Assert.Equal(0u, anchor.GraphFor(0));
        contained.CheckingDead = new RawCheckingFlag(true);
        Assert.Throws<ArgumentException>(() => anchor.GraphFor(0));
        Assert.Throws<ArgumentOutOfRangeException>(() => anchor.GraphFor(65536));
    }
}
