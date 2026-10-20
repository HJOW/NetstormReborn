using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공간 해제(<see cref="RawUnpop"/>)와 공간 해시(<see cref="RawSquidHash"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawUnpopTests)이 정상 호출의 결과를 대조하고, 이 검사들은 cpppj/tests/UnpopTests.cpp 의 보호 검사를 10.78 로 옮기며
/// "거부한 호출은 풀·점유 지도·해시 머리를 한 바이트도 바꾸지 않는다", 상태별 동작, 연결 지점(표시·미니맵·건물 옆면)의 호출 순서를 더해 확인한다.
/// 연결 지점 순서는 cpppj 구현의 계약이며 원본 기대값 행으로 증명된 것이 아니다(그 효과를 가진 모듈은 아직 없다) (W02 Unpop 조각, 2026-10-16).
/// </summary>
public sealed class RawUnpopTests
{
    /// <summary>10.78 슬롯의 x 좌표(float) 위치.</summary>
    private const int XOffset = 14;

    /// <summary>10.78 슬롯의 y 좌표(float) 위치.</summary>
    private const int YOffset = 18;

    /// <summary>검사에 쓰는 타입 번호. 파생 생성자가 있고 공통 표시 경로를 지원하는 가상 표를 쓴다(cpppj 보호 검사와 같다).</summary>
    private const uint TestType = 82;

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

    /// <summary>float 좌표를 슬롯의 x·y 위치에 비트 그대로 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="x">x 좌표.</param>
    /// <param name="y">y 좌표.</param>
    private static void PutPosition(Span<byte> bytes, float x, float y)
    {
        Put(bytes, XOffset, BitConverter.SingleToUInt32Bits(x));
        Put(bytes, YOffset, BitConverter.SingleToUInt32Bits(y));
    }

    /// <summary>한 단계의 해시 머리 전체를 비교용으로 복사한다.</summary>
    /// <param name="hash">대상 해시.</param>
    private static ushort[] HeadSnapshot(RawSquidHash hash) => Enumerable.Range(0, RawSquidHash.Levels).SelectMany(level => hash.Entries(level).ToArray()).ToArray();

    /// <summary>
    /// 손상된 검색 경로·미복원 효과는 풀·점유 지도·해시 머리를 바꾸기 전에 거부한다:
    /// 체인에 없음, 지원하지 않는 가상 표, contained, 건물 옆면 미연결, 자기 next, 유한하지 않은 좌표.
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
        var unpop = new RawUnpop(pool, hash, spots);
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[RawSidPool.StateOffset] = 0;
        PutPosition(bytes, 20f, 20f);
        byte[] before = pool.Bytes.ToArray();
        byte[] originalSpots = spots.ToArray();
        // 어느 체인에도 없다
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, types[TestType]));
        Assert.Equal(before, pool.Bytes.ToArray());
        hash.Bucket(1, 20, 20) = (ushort)sid;
        ushort[] heads = HeadSnapshot(hash);
        // 지원하지 않는 가상 표(표시 갱신의 파생 효과 미복원)
        uint vtable = BitConverter.ToUInt32(pool.Slot(sid)[..4]);
        Put(bytes, 0, 0x12345678);
        byte[] invalidVtable = pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, types[TestType]));
        Assert.Equal(invalidVtable, pool.Bytes.ToArray());
        Put(bytes, 0, vtable);
        // contained 슬롯
        bytes[RawSidPool.StateOffset] = 8;
        byte[] contained = pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, types[TestType]));
        Assert.Equal(contained, pool.Bytes.ToArray());
        bytes[RawSidPool.StateOffset] = 0;
        // 건물군 타입(매몰 아님)은 옆면 몸체가 없으면 거부한다
        var building = new OriginalTypeRecord { Flags2 = 0x4000, ConstructorAddress = types[TestType].ConstructorAddress };
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, building));
        // 제거 뒤 자기 자신을 가리키게 되는 next
        Put(bytes, RawSidPool.NextOffset, (uint)sid, 2);
        byte[] selfNext = pool.Bytes.ToArray();
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, types[TestType]));
        Assert.Equal(selfNext, pool.Bytes.ToArray());
        // 유한하지 않은 좌표
        Put(bytes, RawSidPool.NextOffset, 0, 2);
        Put(bytes, XOffset, 0x7FC00000);
        byte[] notFinite = pool.Bytes.ToArray();
        Assert.ThrowsAny<ArgumentOutOfRangeException>(() => unpop.Unpop(sid, types[TestType]));
        Assert.Equal(notFinite, pool.Bytes.ToArray());
        Assert.Equal(originalSpots, spots);
        Assert.Equal((ushort)sid, hash.Bucket(1, 20, 20));
        Assert.Equal(heads, HeadSnapshot(hash));
        // 서로 다른 풀을 쓰는 해제 모듈은 팩토리에 연결할 수 없다
        Assert.Throws<ArgumentException>(() => new RawSquidFactory(new RawSidPool(32768), types, false, unpop));
        // 점유 지도 크기가 틀리면 만들 수 없다
        Assert.Throws<ArgumentException>(() => new RawUnpop(pool, hash, new byte[100]));
    }

    /// <summary>free·void 슬롯은 아무것도 바꾸지 않고 돌아오며, dead 비트는 해제를 막지 않는다. 번호 0 과 자산 타입 밖은 거부한다.</summary>
    [Fact]
    public void StateBitsDecideWhetherUnpopRuns()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var unpop = new RawUnpop(pool, hash, spots);
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        PutPosition(bytes, 30f, 40f);
        // 만든 직후는 void 이므로 체인에 없어도 아무 일 없이 돌아온다
        byte[] before = pool.Bytes.ToArray();
        unpop.Unpop(sid, types[TestType]);
        Assert.Equal(before, pool.Bytes.ToArray());
        // dead 비트가 켜진 활성 오브젝트는 해제된다
        hash.Bucket(1, 30, 40) = (ushort)sid;
        bytes[RawSidPool.StateOffset] = RawSidPool.DeadBit;
        unpop.Unpop(sid, types[TestType]);
        Assert.Equal((byte)(RawSidPool.DeadBit | RawSidPool.VoidBit), bytes[RawSidPool.StateOffset]);
        Assert.Equal((ushort)0, hash.Bucket(1, 30, 40));
        // 번호 0 과 자산 타입(70) 밖 타입은 거부한다
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(0, types[TestType]));
        bytes[RawSidPool.TypeOffset] = 69;
        bytes[RawSidPool.StateOffset] = 0;
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, types[TestType]));
        Assert.Throws<ArgumentOutOfRangeException>(() => unpop.Unpop(32768, types[TestType]));
    }

    /// <summary>
    /// 체인 중간의 오브젝트를 풀면 앞 슬롯의 next 가 뒤 슬롯을 가리키고, 풀린 슬롯 자신의 next 는 보존된다.
    /// 머리를 풀면 머리가 뒤 슬롯으로 바뀐다. 다른 단계 체인은 건드리지 않는다.
    /// </summary>
    [Fact]
    public void ChainRemovalRelinksPreviousAndKeepsOwnNext()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        var unpop = new RawUnpop(pool, hash, new byte[256 * 256]);
        int[] sids = [factory.Create(TestType), factory.Create(TestType), factory.Create(TestType)];
        // 세 오브젝트를 같은 단계 2 버킷의 체인 a → b → c 로 놓는다
        foreach (int sid in sids)
        {
            Span<byte> bytes = pool.AllocatedBytes(sid);
            bytes[RawSidPool.StateOffset] = 0;
            PutPosition(bytes, 50.5f, 60.5f);
        }
        hash.Bucket(2, 50.5f, 60.5f) = (ushort)sids[0];
        Put(pool.AllocatedBytes(sids[0]), RawSidPool.NextOffset, (uint)sids[1], 2);
        Put(pool.AllocatedBytes(sids[1]), RawSidPool.NextOffset, (uint)sids[2], 2);
        // 가운데를 푼다
        unpop.Unpop(sids[1], types[TestType]);
        Assert.Equal(sids[2], pool.Slot(sids[0])[RawSidPool.NextOffset] | (pool.Slot(sids[0])[RawSidPool.NextOffset + 1] << 8));
        Assert.Equal(sids[2], pool.Slot(sids[1])[RawSidPool.NextOffset] | (pool.Slot(sids[1])[RawSidPool.NextOffset + 1] << 8));
        Assert.Equal((ushort)sids[0], hash.Bucket(2, 50.5f, 60.5f));
        // 머리를 푼다
        unpop.Unpop(sids[0], types[TestType]);
        Assert.Equal((ushort)sids[2], hash.Bucket(2, 50.5f, 60.5f));
    }

    /// <summary>호출 순서를 기록하는 연결 지점 시험용 구현. 호출 시점의 슬롯 상태·점유 값을 함께 적는다.</summary>
    private sealed class Recorder : IRawUnpopDisplay, IRawBuildingSide, IRawMinimapQueue
    {
        /// <summary>대상 풀.</summary>
        public required RawSidPool PoolValue { get; init; }

        /// <summary>대상 해시.</summary>
        public required RawSquidHash HashValue { get; init; }

        /// <summary>대상 점유 지도.</summary>
        public required byte[] Spots { get; init; }

        /// <summary>관찰 중인 슬롯 번호.</summary>
        public int Sid { get; set; }

        /// <summary>호출 순서와 그 시점의 관찰값.</summary>
        public List<string> Events { get; } = [];

        /// <summary>테스트가 쓰는 슬롯 풀을 돌려준다.</summary>
        public RawSidPool Pool => PoolValue;

        /// <summary>테스트가 쓰는 공간 해시를 돌려준다.</summary>
        public RawSquidHash Hash => HashValue;

        /// <summary>검증 호출을 기록한다(슬롯 void 여부와 플래그).</summary>
        public void Validate(ReadOnlySpan<byte> slot, uint flags) => Events.Add($"validate void={(slot[RawSidPool.StateOffset] & RawSidPool.VoidBit) != 0} flags={flags}");

        /// <summary>갱신 호출을 기록한다(void 여부·머리·중앙 spot).</summary>
        public void Update(ReadOnlySpan<byte> slot, uint flags) =>
            Events.Add($"update void={(slot[RawSidPool.StateOffset] & RawSidPool.VoidBit) != 0} head={HashValue.Bucket(1, 20, 20)} center={Spots[19 * 256 + 19]:X2}");

        /// <summary>옆면 해제 호출을 칸 좌표와 spot 값과 함께 기록한다.</summary>
        public void UnpopSide(int cellX, int cellY) => Events.Add($"side {cellX},{cellY} cell={Spots[cellY * 256 + cellX]:X2}");

        /// <summary>미니맵 큐 등록 호출을 기록한다.</summary>
        public void Enqueue(int sid) =>
            Events.Add($"minimap void={(PoolValue.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.VoidBit) != 0} head={HashValue.Bucket(1, 20, 20)} center={Spots[19 * 256 + 19]:X2}");
    }

    /// <summary>
    /// 건물군 오브젝트(3×3, 매몰 아님)를 풀 때의 연결 지점 순서: 표시 검증 → void 설정 → (섬·다리일 때만) 미니맵 → 칸마다 "점유 해제 후 옆면 해제" →
    /// 체인 제거 → 표시 갱신. 점유는 이 타입이 적은 비트(바깥 칸 0x12, 안쪽 0x1A)만 지우고 이웃 칸과 다른 비트는 보존한다.
    /// 건물 옆면은 발자국의 왼쪽·오른쪽 열 가운데 맨 위·맨 아래 행을 뺀 칸이 위쪽 행부터 왼쪽 칸 순서로 불리며,
    /// 불린 시점에 그 칸의 점유는 이미 지워져 있다(0xFF → 0xED).
    /// </summary>
    [Fact]
    public void ConnectionPointsRunInOriginalOrder()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        Array.Fill(spots, (byte)0xFF);
        var recorder = new Recorder { PoolValue = pool, HashValue = hash, Spots = spots };
        var unpop = new RawUnpop(pool, hash, spots, recorder);
        unpop.SetBuildingSide(recorder);
        unpop.SetMinimapQueue(recorder);
        // 건물군(0x4000)에 놓기 막음 비트(0x10)를 더하고 섬 비트(2)도 켜 미니맵 알림 경로를 함께 탄다
        var type = new OriginalTypeRecord { Flags2 = 0x4000 | 0x10 | 2, FootX = 3, FootY = 3, ConstructorAddress = types[TestType].ConstructorAddress };
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[RawSidPool.StateOffset] = 0;
        PutPosition(bytes, 20f, 20f);
        hash.Bucket(1, 20, 20) = (ushort)sid;
        unpop.Unpop(sid, type, 0);
        Assert.Equal(
            [
                "validate void=False flags=0",
                "minimap void=True head=" + sid + " center=FF",
                "side 18,19 cell=ED",
                "side 20,19 cell=ED",
                "update void=True head=0 center=E5",
            ],
            recorder.Events);
        // 발자국 아홉 칸에서 이 타입이 적었던 비트(0x12, 안쪽은 0x1A)만 지워졌고 바깥 칸은 그대로다
        Assert.Equal((byte)0xED, spots[18 * 256 + 18]);
        Assert.Equal((byte)0xE5, spots[19 * 256 + 19]);
        Assert.Equal((byte)0xFF, spots[17 * 256 + 17]);
        Assert.Equal((byte)0xFF, spots[21 * 256 + 21]);
        Assert.Equal(RawSidPool.VoidBit, bytes[RawSidPool.StateOffset]);
        // 섬·다리 비트가 없는 건물은 미니맵 큐에 알리지 않는다(표시 검증·옆면·갱신은 그대로 불린다)
        recorder.Events.Clear();
        var plain = new OriginalTypeRecord { Flags2 = 0x4000 | 0x10, FootX = 3, FootY = 3, ConstructorAddress = types[TestType].ConstructorAddress };
        int second = factory.Create(TestType);
        Span<byte> secondBytes = pool.AllocatedBytes(second);
        secondBytes[RawSidPool.StateOffset] = 0;
        PutPosition(secondBytes, 20f, 20f);
        hash.Bucket(1, 20, 20) = (ushort)second;
        unpop.Unpop(second, plain, 0);
        Assert.DoesNotContain(recorder.Events, line => line.StartsWith("minimap", StringComparison.Ordinal));
        Assert.Equal(["validate", "side", "side", "update"], recorder.Events.Select(line => line.Split(' ')[0]));
        // 서로 다른 풀·해시를 쓰는 몸체와 큐는 연결할 수 없다
        var other = new Recorder { PoolValue = new RawSidPool(32768), HashValue = hash, Spots = spots };
        Assert.Throws<ArgumentException>(() => unpop.SetBuildingSide(other));
        Assert.Throws<ArgumentException>(() => unpop.SetMinimapQueue(other));
        var otherHash = new Recorder { PoolValue = pool, HashValue = new RawSquidHash(), Spots = spots };
        Assert.Throws<ArgumentException>(() => unpop.SetBuildingSide(otherHash));
    }

    /// <summary>
    /// 매몰된 오브젝트는 점유를 쓰지 않았으므로 지우지 않고 건물 옆면·미니맵도 건너뛴다(건물 몸체가 없어도 풀린다).
    /// 체인에서는 제거되고 void 가 된다.
    /// </summary>
    [Fact]
    public void BuriedObjectSkipsSpotsSidesAndMinimap()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        Array.Fill(spots, (byte)0xFF);
        var unpop = new RawUnpop(pool, hash, spots);
        var recorder = new Recorder { PoolValue = pool, HashValue = hash, Spots = spots };
        unpop.SetMinimapQueue(recorder);
        var type = new OriginalTypeRecord { Flags2 = 0x4000 | 0x10 | 2, FootX = 3, FootY = 3, ConstructorAddress = types[TestType].ConstructorAddress };
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[RawSidPool.StateOffset] = 0;
        bytes[40] |= 8;
        PutPosition(bytes, 20f, 20f);
        hash.Bucket(1, 20, 20) = (ushort)sid;
        unpop.Unpop(sid, type);
        Assert.Empty(recorder.Events);
        Assert.All(spots, spot => Assert.Equal((byte)0xFF, spot));
        Assert.Equal((ushort)0, hash.Bucket(1, 20, 20));
        Assert.Equal(RawSidPool.VoidBit, bytes[RawSidPool.StateOffset]);
    }

    /// <summary>
    /// 점유를 지우는 발자국이 지도 밖으로 나가는 타입(왼쪽·위쪽이 음수)은 상태를 바꾸기 전에 거부한다.
    /// </summary>
    [Fact]
    public void FootprintOutsideMapIsRejectedBeforeChanges()
    {
        var pool = new RawSidPool(32768);
        OriginalTypeRecord[] types = Types();
        var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash();
        byte[] spots = new byte[256 * 256];
        var unpop = new RawUnpop(pool, hash, spots);
        var type = new OriginalTypeRecord { Flags2 = 0x10, FootX = 5, FootY = 5, ConstructorAddress = types[TestType].ConstructorAddress };
        int sid = factory.Create(TestType);
        Span<byte> bytes = pool.AllocatedBytes(sid);
        bytes[RawSidPool.StateOffset] = 0;
        PutPosition(bytes, 2f, 2f);
        hash.Bucket(1, 2, 2) = (ushort)sid;
        byte[] before = pool.Bytes.ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => unpop.Unpop(sid, type));
        Assert.Equal(before, pool.Bytes.ToArray());
        Assert.Equal((ushort)sid, hash.Bucket(1, 2, 2));
        type.FootX = 0;
        Assert.Throws<InvalidOperationException>(() => unpop.Unpop(sid, type));
        Assert.Equal(before, pool.Bytes.ToArray());
    }

    /// <summary>
    /// 공간 해시: 단계별 배열 한 변은 256·128·64·16, 월드 좌표는 0 방향으로 절삭한 뒤 단계 크기로 나눠 y 가 바깥인 행 우선 인덱스가 된다.
    /// 유한하지 않거나 범위 밖인 좌표와 잘못된 단계는 예외이며, 지우기는 모든 단계의 머리를 0 으로 만든다.
    /// </summary>
    [Fact]
    public void SpatialHashAddressesBucketsByLevelScale()
    {
        Assert.Equal([256, 128, 64, 16], Enumerable.Range(0, 4).Select(RawSquidHash.Side));
        Assert.Equal(0, RawSquidHash.BucketIndex(0, 0f, 0f));
        Assert.Equal(255 * 256 + 255, RawSquidHash.BucketIndex(0, 255.99f, 255.99f));
        Assert.Equal(10 * 128 + 5, RawSquidHash.BucketIndex(1, 11.9f, 21.9f));
        Assert.Equal(15 * 16 + 14, RawSquidHash.BucketIndex(3, 239.99f, 255.5f));
        Assert.Equal(1 * 16 + 1, RawSquidHash.BucketIndex(3, 16f, 16f));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.BucketIndex(0, 256f, 0f));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.BucketIndex(0, 0f, -0.5f));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.BucketIndex(0, float.NaN, 0f));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.BucketIndex(0, float.PositiveInfinity, 0f));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.Side(4));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidHash.Side(-1));
        var hash = new RawSquidHash();
        hash.Bucket(3, 40f, 40f) = 77;
        Assert.Equal((ushort)77, hash.Entries(3)[2 * 16 + 2]);
        Assert.Equal(64 * 64, hash.Entries(2).Length);
        hash.Reset();
        Assert.All(Enumerable.Range(0, 4), level => Assert.All(hash.Entries(level).ToArray(), head => Assert.Equal((ushort)0, head)));
    }
}
