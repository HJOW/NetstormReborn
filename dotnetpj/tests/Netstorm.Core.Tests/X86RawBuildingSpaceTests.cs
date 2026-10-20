using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 건물 공간(<see cref="RawBuildingSpace"/>)을 실제 Pop·Unpop 과 이어 원본 x86 기대값에 대조한다.
/// 기대값은 buildingspace-x86.tsv(건물 등록·해제, 판본별 Begin 64회) 중 10.78 몫이다.
/// 각 단계 뒤 Pop 결과·풀 전체·해시 머리·점유 지도 체크섬·앞 192슬롯 바이트를 비교한다.
/// 열 구성은 cpppj/tests/BuildingSpaceTests.cpp 와 같다.
/// CD 10.72 행은 기준 판본이 아니므로 읽지 않는다 (W02 건물 공간 조각, 2026-10-20).
/// </summary>
public sealed class X86RawBuildingSpaceTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 Begin 수.</summary>
    private const int BeginsPerEdition = 64;

    /// <summary>10.78 Pop 단계 수.</summary>
    private const int PopStepsPerEdition = 64;

    /// <summary>10.78 Unpop 단계 수.</summary>
    private const int UnpopStepsPerEdition = 59;

    /// <summary>풀 용량. 원본 관찰과 같다.</summary>
    private const int Capacity = 32768;

    /// <summary>바이트 대조 앞부분 슬롯 수(192슬롯·9,600바이트).</summary>
    private const int PrefixSlots = 192;

    /// <summary>관찰 대상 타입 번호(건물).</summary>
    private const int BuildingType = 74;

    /// <summary>섬 타입 번호.</summary>
    private const int IslandType = 82;

    /// <summary>다리 타입 번호.</summary>
    private const int BridgeType = 81;

    /// <summary>raw 슬롯(50바이트)을 16진수로 직접 심는다. 미할당 슬롯에도 쓸 수 있다.</summary>
    /// <param name="pool">슬롯 풀.</param>
    /// <param name="sid">슬롯 번호.</param>
    /// <param name="hex">슬롯 50바이트 16진수.</param>
    private static void Seed(RawSidPool pool, int sid, string hex)
    {
        byte[] bytes = Convert.FromHexString(hex);
        // 원본 기계어 입력과 같은 크기여야 한다
        Assert.Equal(RawSidPool.Stride, bytes.Length);
        bytes.CopyTo(pool.RawSlot(sid));
    }

    /// <summary>네 단계 공간 해시의 모든 머리를 리틀 엔디언 바이트로 이어 붙인다.</summary>
    /// <param name="hash">대상 해시.</param>
    private static byte[] HashBytes(RawSquidHash hash)
    {
        var bytes = new List<byte>();
        // 0 단계부터 3 단계까지
        foreach (int level in Enumerable.Range(0, RawSquidHash.Levels))
        {
            // 머리마다 두 바이트
            foreach (ushort value in hash.Entries(level))
            {
                bytes.Add((byte)value);
                bytes.Add((byte)(value >> 8));
            }
        }
        return bytes.ToArray();
    }

    /// <summary>
    /// 건물 등록·해제 fixture 의 10.78 몫을 재생한다.
    /// Begin 마다 풀·해시·타입을 세우고 Building·Candidate·Spot 을 심은 뒤 Step 을 실행한다.
    /// </summary>
    [Fact]
    public void WholeBuildingPopAndUnpopMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("buildingspace");
        RawSidPool? pool = null;
        RawSquidHash? hash = null;
        byte[]? spots = null;
        OriginalTypeRecord[] types = [];
        RawPop? pop = null;
        RawUnpop? unpop = null;
        int begins = 0;
        int pops = 0;
        int unpops = 0;
        var state = new BuildingSpaceState { IslandType = IslandType, BridgeType = BridgeType };
        X86Fixture.CheckRows(rows, row =>
        {
            switch (row[0])
            {
                case "Begin":
                {
                    if (row[1] != PatchEdition)
                    {
                        pool = null;
                        return true;
                    }
                    pool = new RawSidPool(Capacity, false);
                    hash = new RawSquidHash();
                    spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];
                    types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
                    // base 생성 입력의 최대 HP·깊이·발자국 초기값이다
                    for (int i = 0; i < types.Length; i++)
                    {
                        types[i] = new OriginalTypeRecord
                        {
                            ConstructorAddress = OriginalConstructors.Address(i),
                            MaxHitPoints = 91,
                            ZOrder = 7,
                            FootX = 1,
                            FootY = 1,
                        };
                    }
                    types[BuildingType].Flags2 = X86Fixture.UInt(row[3]);
                    types[BuildingType].FootX = X86Fixture.Int(row[4]);
                    types[BuildingType].FootY = X86Fixture.Int(row[5]);
                    types[BridgeType].Flags1 = row[6] != "0" ? OriginalTypeRecord.Surface : 0;
                    types[BridgeType].Flags2 = 4;
                    types[IslandType].Flags1 = OriginalTypeRecord.Surface;
                    types[IslandType].Flags2 = 2;
                    var factory = new RawSquidFactory(pool, types);
                    var building = new RawBuildingSpace(pool, hash, types, state);
                    pop = new RawPop(pool, hash, spots);
                    unpop = new RawUnpop(pool, hash, spots);
                    pop.SetBuildingSide(building);
                    unpop.SetBuildingSide(building);
                    begins++;
                    return true;
                }
                case "Building":
                {
                    if (pool is null)
                    {
                        return true;
                    }
                    Assert.Equal(3, row.Length);
                    Assert.Equal(X86Fixture.Int(row[1]), new RawSquidFactory(pool, types).Create(BuildingType, SidAllocation.Client));
                    Seed(pool, X86Fixture.Int(row[1]), row[2]);
                    return true;
                }
                case "Candidate":
                {
                    if (pool is null)
                    {
                        return true;
                    }
                    Assert.Equal(5, row.Length);
                    int sid = X86Fixture.Int(row[1]);
                    Seed(pool, sid, row[4]);
                    hash!.Bucket(0, X86Fixture.Int(row[2]), X86Fixture.Int(row[3])) = (ushort)sid;
                    return true;
                }
                case "Spot":
                {
                    if (pool is null)
                    {
                        return true;
                    }
                    Assert.Equal(4, row.Length);
                    spots![X86Fixture.Int(row[2]) * RawSquidHash.WorldCells + X86Fixture.Int(row[1])] = (byte)X86Fixture.UInt(row[3]);
                    return true;
                }
                case "Step":
                {
                    if (pool is null)
                    {
                        return true;
                    }
                    int sid = X86Fixture.Int(row[2]);
                    if (row[1] == "Pop")
                    {
                        Assert.Equal(8, row.Length);
                        Assert.Equal(X86Fixture.Int(row[3]), (int)pop!.Pop(sid, types[BuildingType], 4f, 4f, 30f, 45f, 8));
                        Assert.Equal(X86Fixture.UInt(row[4]), X86Fixture.Adler32(pool.Bytes));
                        Assert.Equal(X86Fixture.UInt(row[5]), X86Fixture.Adler32(HashBytes(hash!)));
                        Assert.Equal(X86Fixture.UInt(row[6]), X86Fixture.Adler32(spots!));
                        Assert.Equal(row[7], Convert.ToHexStringLower(pool.Bytes[..(PrefixSlots * RawSidPool.Stride)]));
                        pops++;
                    }
                    else
                    {
                        Assert.Equal("Unpop", row[1]);
                        Assert.Equal(8, row.Length);
                        // Unpop 은 반환이 없으므로 기대 결과 0을 확인한다
                        Assert.Equal(0, X86Fixture.Int(row[3]));
                        unpop!.Unpop(sid, types[BuildingType], 8);
                        Assert.Equal(X86Fixture.UInt(row[4]), X86Fixture.Adler32(pool.Bytes));
                        Assert.Equal(X86Fixture.UInt(row[5]), X86Fixture.Adler32(HashBytes(hash!)));
                        Assert.Equal(X86Fixture.UInt(row[6]), X86Fixture.Adler32(spots!));
                        Assert.Equal(row[7], Convert.ToHexStringLower(pool.Bytes[..(PrefixSlots * RawSidPool.Stride)]));
                        unpops++;
                    }
                    return true;
                }
                default:
                    return false;
            }
        });
        Assert.Equal(BeginsPerEdition, begins);
        Assert.Equal(PopStepsPerEdition, pops);
        Assert.Equal(UnpopStepsPerEdition, unpops);
    }
}
