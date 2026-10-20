using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 이웃 탐색기(<see cref="RawSquidNeighbors"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 neighbor-x86.tsv(첫 이웃, 판본별 288회) 중 10.78 몫이다.
/// 각 탐색마다 첫 이웃 번호를 비교한다.
/// 열 구성은 cpppj/tests/NeighborTests.cpp 와 같다.
/// CD 10.72·10.37 행과 다리 사건(Event) 행은 기준 판본·범위가 아니므로 읽지 않는다 (W02 이웃 조각, 2026-10-20).
/// </summary>
public sealed class X86RawNeighborTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>판본별 First 호출 수. 전체 864행 중 10.78 몫이다.</summary>
    private const int FirstsPerEdition = 288;

    /// <summary>원본 가상 표(10.78). 슬롯 식별 값으로만 쓰며 호스트 함수로 호출하지 않는다.</summary>
    private const uint Vtable = 0x11010000;

    /// <summary>검사 대상 번호.</summary>
    private const int Source = 50;

    /// <summary>값을 리틀 엔디언으로 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>분석 요구와 같은 생성 코드 목록을 변형 번호로 만든다.</summary>
    /// <param name="variant">하드 비트 변형(0·1·2).</param>
    private static List<FrameCode> Frames(int variant)
    {
        const string letters = "JKALMNOJKLBJPPCD";
        var codes = new List<FrameCode>();
        // 변형 1은 0·1·7·8·11번, 변형 2는 0·8번을 하드로 둔다
        for (int i = 0; i < letters.Length; i++)
        {
            bool hard = variant == 1 ? i == 0 || i == 1 || i == 7 || i == 8 || i == 11 : variant == 2 && (i == 0 || i == 8);
            var flags = (FrameCodeFlags)(hard ? 0x40 : letters[i] == 'L' || letters[i] == 'M' ? 0x20 : 0);
            codes.Add(new FrameCode(letters[i], 'P', i + 1, flags));
        }
        return codes;
    }

    /// <summary>
    /// 첫 이웃 fixture 의 10.78 몫 288회를 재생한다.
    /// 호출마다 첫 이웃 번호를 대조한다.
    /// </summary>
    [Fact]
    public void FirstNeighborMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("neighbor");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != "First" || row[1] != PatchEdition)
            {
                return true;
            }
            Assert.Equal(6, row.Length);
            var pool = new RawSidPool(24000, false);
            // 원본 관찰의 번호 5~74번을 클라이언트 영역에 확보한다
            for (int id = 5; id <= 74; id++)
            {
                Assert.Equal(id, pool.Allocate(SidAllocation.Client));
            }
            var hash = new RawSquidHash();
            var spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
            var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            for (int i = 0; i < types.Length; i++)
            {
                types[i] = new OriginalTypeRecord();
            }
            var frames = new List<IReadOnlyList<FrameCode>>();
            for (int i = 0; i < types.Length; i++)
            {
                frames.Add([]);
            }
            hash.Reset();
            // 두 기준 슬롯의 stale 바이트를 지운다
            foreach (int sid in new[] { Source, 60 })
            {
                pool.RawSlot(sid).Clear();
            }
            // 각 노드의 raw·타입·프레임·해시 체인을 채운다
            foreach (string entry in row[2].Split(';'))
            {
                string[] n = entry.Split(',');
                Assert.Equal(12, n.Length);
                int sid = X86Fixture.Int(n[0]);
                uint typ = X86Fixture.UInt(n[1]);
                Span<byte> raw = pool.AllocatedBytes(sid);
                raw.Clear();
                Put(raw, 0, Vtable, 4);
                raw[RawSidPool.TypeOffset] = (byte)typ;
                raw[RawSidPool.StateOffset] = (byte)X86Fixture.UInt(n[2]);
                raw[40] = (byte)X86Fixture.UInt(n[3]);
                Put(raw, 14, X86Fixture.UInt(n[4]), 4);
                Put(raw, 18, X86Fixture.UInt(n[5]), 4);
                Put(raw, 36, X86Fixture.UInt(n[10]), 4);
                types[typ].FootX = X86Fixture.Int(n[6]);
                types[typ].FootY = X86Fixture.Int(n[7]);
                types[typ].Flags1 = X86Fixture.UInt(n[8]);
                types[typ].Flags2 = X86Fixture.UInt(n[9]);
                frames[(int)typ] = Frames(X86Fixture.Int(row[4]));
                ref ushort head = ref hash.Bucket(X86Fixture.Int(n[11]), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(n[4])), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(n[5])));
                Put(raw, 4, head, 2);
                head = (ushort)sid;
            }
            if (row[3] != "-")
            {
                // 발자국·표시 비트 입력을 쓴다
                foreach (string entry in row[3].Split(';'))
                {
                    string[] n = entry.Split(',');
                    spots[X86Fixture.Int(n[1]) * RawSquidHash.WorldCells + X86Fixture.Int(n[0])] = (byte)X86Fixture.UInt(n[2]);
                }
            }
            var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
            Assert.Equal(X86Fixture.Int(row[5]), neighbors.First(Source));
            count++;
            return true;
        });
        Assert.Equal(FirstsPerEdition, count);
    }
}
