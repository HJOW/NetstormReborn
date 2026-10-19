using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공간 등록(<see cref="RawPop"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawPopTests)이 정상 호출의 결과를 대조하고, 이 검사들은 cpppj/tests/PopTests.cpp 의 보호 검사를 10.78 로 옮기며
/// "거부한 호출은 풀·점유 지도·해시 머리를 한 바이트도 바꾸지 않는다", 겹침 반환의 쓰기 범위, 연결 지점(표시·미니맵·건물 옆면·파생)의 호출 순서를 더해 확인한다.
/// 연결 지점 순서는 cpppj 구현의 계약이며 원본 기대값 행으로 증명된 것이 아니다(그 효과를 가진 모듈은 아직 없다) (W02 Pop 조각, 2026-10-20).
/// </summary>
public sealed class RawPopTests
{
    /// <summary>검사에 쓰는 타입 번호. 파생 생성자가 없고 기본 가상 표를 쓴다(cpppj 보호 검사와 같다).</summary>
    private const uint TestType = 74;

    /// <summary>기본 가상 표(타입 74 의 생성자 주소가 0 이므로 base). Pop 지원 표에 있다.</summary>
    private const uint BaseVtable = OriginalConstructors.BaseVtable;

    /// <summary>표시 지원 표에는 있으나 Pop 지원 표에는 없는 가상 표. 파생 경로 검사에 쓴다.</summary>
    private const uint DisplayOnlyVtable = 0x00501EA0;

    /// <summary>생성자 주소 표를 채운 10.78 타입 레코드 188개.</summary>
    private static OriginalTypeRecord[] Types()
    {
        var types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
        // 모든 타입 번호의 실제 생성자 주소를 채운다
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(i) };
        }
        return types;
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>void 상태의 합성 슬롯을 만든다. 팩토리가 만든 슬롯에 가상 표·추가 비트를 덮는다.</summary>
    private static int VoidSlot(RawSquidFactory factory, RawSidPool pool, uint vtable = BaseVtable, byte extra = 0)
    {
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        Put(bytes, 0, vtable);
        bytes[40] = extra;
        return sid;
    }

    /// <summary>한 단계의 해시 머리 전체를 비교용으로 복사한다.</summary>
    /// <param name="hash">대상 해시.</param>
    private static ushort[] HeadSnapshot(RawSquidHash hash) => Enumerable.Range(0, RawSquidHash.Levels).SelectMany(level => hash.Entries(level).ToArray()).ToArray();

    /// <summary>호출 순서를 기록하는 공통 표시 경로 대역.</summary>
    private sealed class RecordingDisplay(List<string> log) : IRawUnpopDisplay
    {
        /// <summary>호출 기록.</summary>
        public void Validate(ReadOnlySpan<byte> slot, uint flags) => log.Add($"Validate:{flags:X}");

        /// <summary>호출 기록.</summary>
        public void Update(ReadOnlySpan<byte> slot, uint flags) => log.Add($"Update:{flags:X}");
    }

    /// <summary>호출 순서를 기록하는 미니맵 큐 대역.</summary>
    private sealed class RecordingMetadisplay(RawSidPool pool, List<string> log) : IRawPopMetadisplay
    {
        /// <summary>연결된 풀.</summary>
        public RawSidPool Pool => pool;

        /// <summary>호출 기록.</summary>
        public void Enqueue(int sid) => log.Add($"Enqueue:{sid}");
    }

    /// <summary>호출 순서를 기록하는 건물 옆면 대역.</summary>
    private sealed class RecordingBuilding(RawSidPool pool, RawSquidHash hash, List<string> log) : IRawPopBuildingSide
    {
        /// <summary>연결된 풀.</summary>
        public RawSidPool Pool => pool;

        /// <summary>연결된 해시.</summary>
        public RawSquidHash Hash => hash;

        /// <summary>호출 기록.</summary>
        public void PopSide(int cellX, int cellY, float buildingX, float buildingY) => log.Add($"PopSide:{cellX},{cellY}");
    }

    /// <summary>호출 순서를 기록하는 파생 등록 효과 대역.</summary>
    private sealed class RecordingPostPop(RawSidPool pool, List<string> log, Func<int, bool>? handles = null) : IRawPopPostPop
    {
        /// <summary>연결된 풀.</summary>
        public RawSidPool Pool => pool;

        /// <summary>호출 기록.</summary>
        public void ValidateSpace(RawSquidHash hash, byte[] spots) => log.Add("ValidateSpace");

        /// <summary>지정한 대로 처리 가능 여부를 돌려준다(기본 true).</summary>
        public bool HandlesDerived(int sid) => handles?.Invoke(sid) ?? true;

        /// <summary>호출 기록.</summary>
        public void Validate(int sid, uint flags, in PopProjection projection) => log.Add($"PostValidate:{flags:X}");

        /// <summary>호출 기록.</summary>
        public void Activate(int sid, uint flags) => log.Add($"PostActivate:{flags:X}");
    }

    /// <summary>
    /// 손상된 등록 경로·미복원 효과는 풀·점유 지도·해시 머리를 바꾸기 전에 거부한다:
    /// 지원하지 않는 가상 표, 건물 옆면 미연결, 너무 큰 발자국, 빈 슬롯, 유한하지 않은 좌표, 해시 머리 충돌.
    /// </summary>
    [Fact]
    public void GuardsRejectBeforeChangingPoolOrSpace()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        Array.Fill(spots, (byte)0xFF);
        var pop = new RawPop(pool, hash, spots);
        int sid = VoidSlot(factory, pool);
        byte[] before = pool.Bytes.ToArray();
        byte[] originalSpots = spots.ToArray();
        ushort[] heads = HeadSnapshot(hash);
        // 지원하지 않는 가상 표(파생 firstPop/postPop 효과 미복원)
        Put(pool.AllocatedBytes(sid), 0, 0x12345678);
        before = pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
        Put(pool.AllocatedBytes(sid), 0, BaseVtable);
        before = pool.Bytes.ToArray();
        // 건물군 타입(매몰 아님)은 옆면 몸체가 없으면 거부한다
        var building = new OriginalTypeRecord { Flags2 = 0x4000, ConstructorAddress = types[TestType].ConstructorAddress };
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, building, 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
        // 발자국이 지도를 벗어난다
        building.Flags2 = 0x11;
        building.FootX = 40;
        Assert.Throws<ArgumentOutOfRangeException>(() => pop.Pop(sid, building, 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
        // 빈 슬롯은 void 가 아니므로 그대로 돌려준다(원본은 free 검사가 void 검사 뒤에 있다)
        int free = pool.Allocate(SidAllocation.Client);
        pool.Release(free);
        byte[] afterRelease = pool.Bytes.ToArray();
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(free, types[TestType], 1, 1, 30, 40));
        Assert.Equal(afterRelease, pool.Bytes.ToArray());
        // 전송 중 슬롯은 거부한다
        Span<byte> transmitting = pool.AllocatedBytes(sid);
        transmitting[RawSidPool.StateOffset] |= 16;
        before = pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
        transmitting[RawSidPool.StateOffset] = RawSidPool.VoidBit;
        before = pool.Bytes.ToArray();
        // 유한하지 않은 좌표는 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => pop.Pop(sid, types[TestType], 1, 1, float.NaN, 20));
        Assert.Equal(before, pool.Bytes.ToArray());
        // 해시 머리가 자기 자신이면 거부한다
        hash.Bucket(1, 30, 40) = (ushort)sid;
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
        hash.Bucket(1, 30, 40) = 0;
        // 해시 머리가 풀 범위를 벗어나면 거부한다
        hash.Bucket(1, 30, 40) = (ushort)pool.Capacity;
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
        hash.Bucket(1, 30, 40) = 0;
        Assert.Equal(originalSpots, spots);
        Assert.Equal(heads, HeadSnapshot(hash));
        // 서로 다른 풀을 쓰는 연결 지점은 만들 때·연결할 때 거부한다
        Assert.Throws<ArgumentException>(() => new RawPop(new RawSidPool(32768), hash, new byte[256 * 256], null,
            new RecordingPostPop(pool, [])));
        Assert.Throws<ArgumentException>(() => new RawPop(pool, hash, new byte[100]));
        var otherPool = new RawSidPool(32768);
        var otherHash = new RawSquidHash();
        Assert.Throws<ArgumentException>(() => pop.SetBuildingSide(new RecordingBuilding(otherPool, hash, [])));
        Assert.Throws<ArgumentException>(() => pop.SetBuildingSide(new RecordingBuilding(pool, otherHash, [])));
        Assert.Throws<ArgumentException>(() => pop.SetMetadisplay(new RecordingMetadisplay(otherPool, [])));
    }

    /// <summary>발자국 왼쪽·위쪽 칸이 0 이면 지도 밖으로 보고 거부한다(원본은 1 미만이면 거부한다).</summary>
    [Fact]
    public void FootprintTouchingZeroCellIsRejected()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = 0x11;
        types[TestType].FootX = 31;
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var pop = new RawPop(pool, hash, spots);
        int sid = VoidSlot(factory, pool);
        byte[] before = pool.Bytes.ToArray();
        // 왼쪽 칸이 30-31+1=0 이므로 거부한다
        Assert.Throws<ArgumentOutOfRangeException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(before, pool.Bytes.ToArray());
    }

    /// <summary>매몰(buried) 섬·다리는 미니맵 큐에 알리지 않고 spot 도 쓰지 않는다.</summary>
    [Fact]
    public void BuriedIslandSkipsMinimapAndSpots()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = 0x2;
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var log = new List<string>();
        var pop = new RawPop(pool, hash, spots);
        pop.SetMetadisplay(new RecordingMetadisplay(pool, log));
        int sid = VoidSlot(factory, pool, BaseVtable, 8);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.DoesNotContain(log, entry => entry.StartsWith("Enqueue:", StringComparison.Ordinal));
        Assert.All(spots, spot => Assert.Equal(0, spot));
    }

    /// <summary>이미 등록된(void 꺼짐)·들어 있는 내용물·번호 0 은 등록하지 않고 그대로 돌려준다.</summary>
    [Fact]
    public void AlreadyRegisteredReturnsUnchanged()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var pop = new RawPop(pool, hash, spots);
        int sid = VoidSlot(factory, pool);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        byte[] afterFirst = pool.Bytes.ToArray();
        // 두 번째 등록은 아무것도 바꾸지 않고 그대로 돌려준다
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(afterFirst, pool.Bytes.ToArray());
        // 들어 있는 내용물은 그대로 돌려준다
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[RawSidPool.StateOffset] |= 8;
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        // 번호 0 은 그대로 돌려준다
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(0, types[TestType], 1, 1, 30, 40));
    }

    /// <summary>
    /// 이미 차 있는 spot 칸과 겹치면 체인에 넣지 않고 Overlap 으로 돌려준다.
    /// 좌표·화면 좌표 쓰기와 그 칸까지의 spot·옆면·미니맵 알림은 남고, 머리·void·최초 Pop 비트는 바뀌지 않는다.
    /// </summary>
    [Fact]
    public void OverlapKeepsCoordinatesButSkipsChaining()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = 0x11;
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        spots[40 * 256 + 30] = 0x11;
        var pop = new RawPop(pool, hash, spots);
        int sid = VoidSlot(factory, pool);
        Assert.Equal(RawPopResult.Overlap, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        ReadOnlySpan<byte> bytes = pool.Slot(sid);
        Assert.Equal(30f, BitConverter.UInt32BitsToSingle((uint)(bytes[14] | (bytes[15] << 8) | (bytes[16] << 16) | (bytes[17] << 24))));
        Assert.Equal(40f, BitConverter.UInt32BitsToSingle((uint)(bytes[18] | (bytes[19] << 8) | (bytes[20] << 16) | (bytes[21] << 24))));
        Assert.Equal((ushort)480, (ushort)(bytes[22] | (bytes[23] << 8)));
        Assert.Equal((ushort)440, (ushort)(bytes[24] | (bytes[25] << 8)));
        // 체인에 넣지 않았고 void·최초 Pop 비트도 그대로다
        Assert.Equal(0, hash.Bucket(1, 30, 40));
        Assert.Equal(RawSidPool.VoidBit, (byte)(bytes[RawSidPool.StateOffset] & RawSidPool.VoidBit));
        Assert.Equal(0, bytes[40]);
        Assert.Equal(0x11, spots[40 * 256 + 30]);
    }

    /// <summary>표에는 없으나 표시 갱신을 지원하는 가상 표는 파생 몸체가 처리할 때만 등록한다.</summary>
    [Fact]
    public void DerivedVtableNeedsPostPopHandler()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        // 표시 지원 표에만 있는 가상 표(마스크 3)라 Pop 표 검사에서는 떨어진다
        Assert.False(OriginalPopDisplay.Supports(DisplayOnlyVtable, 0));
        Assert.True(OriginalUnpopDisplay.Supports(DisplayOnlyVtable, 0));
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var bare = new RawPop(pool, hash, spots);
        int sid = VoidSlot(factory, pool, DisplayOnlyVtable);
        Assert.Throws<InvalidOperationException>(() => bare.Pop(sid, types[TestType], 1, 1, 30, 40));
        // 파생 몸체가 거부해도 등록하지 않는다
        var log = new List<string>();
        var refusing = new RawPop(pool, hash, spots, null, new RecordingPostPop(pool, log, _ => false));
        Assert.Throws<InvalidOperationException>(() => refusing.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.DoesNotContain(log, entry => entry.StartsWith("PostActivate", StringComparison.Ordinal));
        // 파생 몸체가 처리하면 정규화한 플래그(최초 Pop 1 + Activate 0x10)로 등록한다
        var accepting = new RawPop(pool, hash, spots, null, new RecordingPostPop(pool, log));
        Assert.Equal(RawPopResult.Registered, accepting.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Contains("PostValidate:11", log);
        Assert.Contains("PostActivate:11", log);
    }

    /// <summary>
    /// 연결 지점 호출 순서는 "표시 Validate → 파생 Validate → 미니맵 알림 → 건물 옆면 → 표시 Update → 파생 Activate"다.
    /// 건물군·섬 비트를 함께 가진 타입으로 여섯 경로가 모두 불리는지 확인한다.
    /// </summary>
    [Fact]
    public void ConnectionPointsRunInOriginalOrder()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        types[TestType].Flags2 = OriginalTypeRecord.BuildingMask | 0x2;
        types[TestType].FootX = 2;
        types[TestType].FootY = 3;
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var log = new List<string>();
        var postPop = new RecordingPostPop(pool, log);
        var pop = new RawPop(pool, hash, spots, new RecordingDisplay(log), postPop);
        pop.SetBuildingSide(new RecordingBuilding(pool, hash, log));
        pop.SetMetadisplay(new RecordingMetadisplay(pool, log));
        int sid = VoidSlot(factory, pool);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 2, 3, 30, 40));
        int displayValidate = log.IndexOf("Validate:0");
        int postValidate = log.IndexOf("PostValidate:11");
        int enqueue = log.FindIndex(entry => entry.StartsWith("Enqueue:", StringComparison.Ordinal));
        int firstSide = log.FindIndex(entry => entry.StartsWith("PopSide:", StringComparison.Ordinal));
        int displayUpdate = log.IndexOf("Update:0");
        int postActivate = log.IndexOf("PostActivate:11");
        Assert.True(displayValidate >= 0 && postValidate > displayValidate);
        Assert.True(enqueue > postValidate && firstSide > enqueue);
        Assert.True(displayUpdate > firstSide && postActivate > displayUpdate);
        // 2×3 발자국의 가운데 행 양쪽 열에만 옆면을 쓴다
        Assert.Equal(2, log.Count(entry => entry.StartsWith("PopSide:", StringComparison.Ordinal)));
        Assert.Contains("PopSide:29,39", log);
        Assert.Contains("PopSide:30,39", log);
        // 섬 비트라 0단계에 머리를 연결하고 섬 번호 127을 쓴다
        Assert.Equal((ushort)sid, hash.Bucket(0, 30, 40));
        ReadOnlySpan<byte> bytes = pool.Slot(sid);
        Assert.Equal(127, bytes[8]);
        Assert.Equal(0, bytes[RawSidPool.StateOffset] & RawSidPool.VoidBit);
        Assert.Equal(0x80, bytes[40] & 0x80);
    }

    /// <summary>최초 Pop 비트가 이미 있으면 파생 플래그에 최초 Pop 비트를 더하지 않는다(두 번째 등록은 0x10만).</summary>
    [Fact]
    public void SecondPopSkipsFirstPopBits()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var log = new List<string>();
        var pop = new RawPop(pool, hash, spots, null, new RecordingPostPop(pool, log));
        var unpop = new RawUnpop(pool, hash, spots);
        int sid = VoidSlot(factory, pool);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        unpop.Unpop(sid, types[TestType]);
        // 추가 비트 1(abstract)이 있어도 최초 Pop 비트가 있으면 파생 플래그에 더하지 않는다
        pool.AllocatedBytes(sid)[40] |= 1;
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        string[] activates = log.Where(entry => entry.StartsWith("PostActivate:", StringComparison.Ordinal)).ToArray();
        Assert.Equal(["PostActivate:11", "PostActivate:10"], activates);
        string[] validates = log.Where(entry => entry.StartsWith("PostValidate:", StringComparison.Ordinal)).ToArray();
        Assert.Equal(["PostValidate:11", "PostValidate:10"], validates);
    }
}
