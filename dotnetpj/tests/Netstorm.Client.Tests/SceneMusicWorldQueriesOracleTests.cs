using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Client.Audio;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 새 원문984행의 상위 조회 계약만 재생한다. 사제/finder/Count/Local은 명시된 기록경계다.
namespace Netstorm.Client.Tests;

/// <summary>원문 bool/EAX·호출인자/순서·Count뒤프레임·Local뒤현재CS를 그대로 비교하는 독립 대조 검사다.</summary>
public sealed class SceneMusicWorldQueriesOracleTests
{
    /// <summary>원문 에뮬레이터의 격리 풀 주소이며 Count의 ECX를 동일 가상SID로 표시할 때만 사용한다.</summary>
    private const uint OraclePool = 0x16000000;
    /// <summary>원문 부호없는 DWORD입력을 현재 문화권과 무관하게 읽는다.</summary>
    private static uint U(string value) => uint.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>signed 프레임입력을 원문 DWORD비트로 바꾼다.</summary>
    private static uint FrameBits(string value) => unchecked((uint)int.Parse(value, CultureInfo.InvariantCulture));
    /// <summary>원시비트를 비정렬 슬롯에 그대로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[offset..], value);

    /// <summary>새fixture의 입력/출력984행을 같은 순수본체로 재생한다. 실제 Game/가상분배/사제/finder부모 완료로 세지 않는다.</summary>
    [Fact]
    public void Matches984OriginalSceneWorldQueryObservations()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "scene-world-queries-x86.tsv");
        var pool = new RawSidPool(24000, false);
        Assert.Equal(5, pool.Allocate(SidAllocation.Client)); Assert.Equal(6, pool.Allocate(SidAllocation.Client));
        int rows = 0, sacrificingRows = 0, waitingRows = 0;
        // 원문 순서를 유지하며 기대값을 계산하거나 기존Scene9339행을 수정하지 않는다.
        foreach (string line in File.ReadLines(path))
        {
            if (line.Length == 0 || line.StartsWith('#')) continue;
            string[] row = line.Split('\t'); Assert.Equal(7, row.Length); Assert.Equal("originals", row[0]);
            string[] input = row[2].Split(','); Assert.Equal(9, input.Length);
            uint owner = U(input[0]), priest = U(input[1]), altar = U(input[2]), count = U(input[4]), mutate = U(input[5]);
            uint challenge = U(input[6]), known = U(input[7]), flag = U(input[8]);
            pool.AllocatedBytes(5).Clear(); pool.AllocatedBytes(6).Clear(); Put(pool.AllocatedBytes(5), 14, 0x42210000); Put(pool.AllocatedBytes(5), 18, 0x42250000);
            Put(pool.AllocatedBytes(6), 36, FrameBits(input[3]));
            var cs = new ClientChallengeGlobals(); Assert.True(cs.Prepare()); uint firstToken = cs.Current;
            uint csOwner = row[1] == "waiting" ? owner : 0;
            Put(cs.Participant(csOwner).Span, 0xa4, flag);
            var events = new List<string>();
            bool result;
            if (row[1] == "sacrificing")
            {
                sacrificingRows++;
                result = SceneMusicWorldQueries.SacrificingBody(pool, owner,
                    value => { events.Add("priest:" + value); return priest; },
                    (x, y) => { events.Add($"altar:{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)},3,4194304"); return checked((int)altar); },
                    sid =>
                    {
                        events.Add("count:" + (OraclePool + unchecked((uint)sid) * 50));
                        if (mutate != 0) Put(pool.AllocatedBytes(sid), 36, 0);
                        return count;
                    });
            }
            else
            {
                Assert.Equal("waiting", row[1]); waitingRows++;
                result = SceneMusicWorldQueries.WaitingRoomBody(() => challenge != 0, () => known, () =>
                {
                    events.Add("local");
                    if (mutate != 0)
                    {
                        cs.Current = 0; Assert.True(cs.Prepare()); Put(cs.Participant(owner).Span, 0xa4, 0x80000000);
                    }
                    return owner;
                }, cs);
            }
            Assert.Equal(U(row[3]), result ? 1u : 0u);
            Assert.Equal(row[4], events.Count == 0 ? "-" : string.Join('|', events));
            Assert.Equal(U(row[5]), BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(6)[36..]));
            Assert.Equal(row[6], cs.Current == firstToken ? "A" : "B");
            rows++;
        }
        Assert.Equal(984, rows); Assert.Equal(768, sacrificingRows); Assert.Equal(216, waitingRows);
    }
}
