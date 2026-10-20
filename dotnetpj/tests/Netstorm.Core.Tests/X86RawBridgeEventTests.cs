using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 다리 사건 처리기(<see cref="RawBridgeEvents"/>)를 원본 x86 기대값에 대조한다.
/// 기대값은 neighbor-x86.tsv 의 Event 행(사건 경유 칸 변환, 판본별 288회) 중 10.78 몫이다.
/// 각 호출마다 반환 비트·효과 순서·다리 슬롯 50바이트·새 객체 슬롯 50바이트를 비교한다.
/// 열 구성은 cpppj/tests/NeighborTests.cpp 와 같다.
/// CD 10.72·10.37 행은 기준 판본이 아니므로 읽지 않는다 (W02 다리 사건 조각, 2026-10-20).
/// </summary>
public sealed class X86RawBridgeEventTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>판본별 사건 호출 수. 전체 576행 중 10.78 몫이다.</summary>
    private const int EventsPerEdition = 288;

    /// <summary>원본 가상 표(10.78). 슬롯 식별 값으로만 쓴다.</summary>
    private const uint Vtable = 0x11010000;

    /// <summary>기준(다리) 번호.</summary>
    private const int Source = 50;

    /// <summary>새로 만드는 객체 번호. 분석 관찰과 같다.</summary>
    private const int Born = 60;

    /// <summary>값을 리틀 엔디언으로 쓴다.</summary>
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
    /// 다리 사건 fixture 의 10.78 몫 288회를 재생한다.
    /// 사건 반환·효과 순서·두 슬롯 전체 바이트를 대조한다.
    /// </summary>
    [Fact]
    public void BridgeEventsMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("neighbor");
        int count = 0;
        var failures = new List<string>();
        // 판본 이름이 originals 인 Event 행만 10.78 경로로 재생한다
        foreach (string[] row in rows)
        {
            if (row[0] != "Event" || row[1] != PatchEdition)
            {
                continue;
            }
            try
            {
                Assert.Equal(25, row.Length);
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
                foreach (int sid in new[] { Source, Born })
                {
                    pool.RawSlot(sid).Clear();
                }
                // 각 노드의 raw·타입·프레임·해시 체인을 채운다
                foreach (string entry in row[19].Split(';'))
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
                    frames[(int)typ] = Frames(X86Fixture.Int(row[6]));
                    ref ushort head = ref hash.Bucket(X86Fixture.Int(n[11]), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(n[4])), BitConverter.UInt32BitsToSingle(X86Fixture.UInt(n[5])));
                    Put(raw, 4, head, 2);
                    head = (ushort)sid;
                }
                if (row[20] != "-")
                {
                    // 발자국·표시 비트 입력을 쓴다
                    foreach (string entry in row[20].Split(';'))
                    {
                        string[] n = entry.Split(',');
                        spots[X86Fixture.Int(n[1]) * RawSquidHash.WorldCells + X86Fixture.Int(n[0])] = (byte)X86Fixture.UInt(n[2]);
                    }
                }
                var state = new BridgeEventState
                {
                    Authority = row[3] == "1",
                    DebugKeep = row[4] == "1",
                    BridgeType = X86Fixture.UInt(row[5]),
                };
                Span<byte> sourceRaw = pool.AllocatedBytes(Source);
                Put(sourceRaw, 8, X86Fixture.UInt(row[11]), 2);
                Put(sourceRaw, 12, X86Fixture.UInt(row[10]), 2);
                sourceRaw[34] = (byte)X86Fixture.UInt(row[12]);
                sourceRaw[40] = (byte)X86Fixture.UInt(row[13]);
                var neighbors = new RawSquidNeighbors(pool, hash, spots, types, frames);
                var events = new List<string>();
                var hooks = new BridgeEventHooks
                {
                    Frames = type => frames[(int)type],
                    FirstNeighbor = sid =>
                    {
                        // 임시 프레임 바이트와 첫 이웃 번호를 기록한다
                        events.Add($"F:{sid}:{pool.Slot(sid)[36]}:0");
                        int first = neighbors.First(sid);
                        events.Add($"I:{first}");
                        return first;
                    },
                    NotifySurface = sid => events.Add($"N:{sid}"),
                    Create = type =>
                    {
                        events.Add($"C:{type}:0");
                        Span<byte> born = pool.AllocatedBytes(Born);
                        Put(born, 0, Vtable, 4);
                        born[RawSidPool.TypeOffset] = (byte)type;
                        born[RawSidPool.StateOffset] = 0;
                        return Born;
                    },
                    Destroy = (sid, flags) => events.Add($"D:{sid}:{flags}"),
                    SetOwner = (sid, owner) => events.Add($"O:{sid}:{owner}"),
                    Pop = (sid, x, y, flags) => events.Add($"P:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{flags}"),
                };
                var bridge = new RawBridgeEvents(pool, state, hooks);
                float result = bridge.Handle(Source, X86Fixture.UInt(row[15]), 0, X86Fixture.FloatBits(row[16]));
                string actual = events.Count == 0 ? "-" : string.Join(';', events);
                string resultBits = BitConverter.SingleToUInt32Bits(result).ToString();
                if (resultBits != row[21] || actual != row[22]
                    || Convert.ToHexStringLower(pool.Slot(Source)) != row[23]
                    || Convert.ToHexStringLower(pool.Slot(Born)) != row[24])
                {
                    failures.Add($"{count}: 기대 {row[22][..Math.Min(100, row[22].Length)]} / 실제 {actual[..Math.Min(100, actual.Length)]}");
                }
                count++;
            }
            catch (Exception error)
            {
                failures.Add($"{count}: {error.GetType().Name}: {error.Message}");
                count++;
            }
            if (failures.Count >= 6)
            {
                break;
            }
        }
        Assert.True(failures.Count == 0, $"원본 입력 {count}개 중 {failures.Count}개 불일치\n" + string.Join('\n', failures.Take(6)));
        Assert.Equal(EventsPerEdition, count);
    }
}
