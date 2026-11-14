using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 현재 원문 부모의 도달 장치·부분 쓰기와 실제 등록/가상 후처리 순서를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 공간 등록(<see cref="RawPop"/>)의 보호 경로와 연결 지점 계약을 확인한다.
/// 원본 기대값 재생(X86RawPopTests)이 정상 호출의 결과를 대조하고, 이 검사들은 cpppj/tests/PopTests.cpp 의 보호 검사를 10.78 로 옮기며
/// 도달 실패 앞의 부분 공간 쓰기·진단 복귀·겹침 반환과 현재 가상 후처리 순서를 확인한다.
/// 연결 지점 순서는 cpppj 구현의 계약이며 원본 기대값 행으로 증명된 것이 아니다(그 효과를 가진 모듈은 아직 없다) (W02 Pop 조각, 2026-10-20).
/// </summary>
public sealed class RawPopTests
{
    /// <summary>검사에 쓰는 타입 번호. 파생 생성자가 없고 기본 가상 표를 쓴다(cpppj 보호 검사와 같다).</summary>
    private const uint TestType = 74;

    /// <summary>기본 가상 표(타입 74 의 생성자 주소가 0 이므로 base). Pop 지원 표에 있다.</summary>
    private const uint BaseVtable = OriginalConstructors.BaseVtable;

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
        int sid = factory.Create(TestType, SidAllocation.Client);
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

        /// <summary>Player 몸체를 쓰지 않으므로 항상 거짓이다.</summary>
        public bool HandlesPlayer(int sid) => false;

        /// <summary>호출 기록.</summary>
        public void Validate(int sid, uint flags, in PopProjection projection) => log.Add($"PostValidate:{flags:X}");

        /// <summary>호출 기록.</summary>
        public void Activate(int sid, uint flags) => log.Add($"PostActivate:{flags:X}");

        /// <summary>부모와 후처리 관측이 공유하는 같은 깊이다.</summary>
        private uint _depth;
        /// <summary>실제 Activate 부모에 동일한 깊이를 빌린다.</summary>
        public ref uint ActivationDepth => ref _depth;
        /// <summary>실제 부모가 올린 깊이를 공통 후처리 관측 뒤 한 번 줄인다.</summary>
        public void PostPopVirtual(int sid, uint flags) { if (handles is not null && !handles(sid)) throw new InvalidOperationException("명시 가상몸체 거부"); Activate(sid, flags); _depth = unchecked(_depth - 1); }

    }

    /// <summary>최신 cpp PopTests처럼 도달하지 않는 옆면은 요구하지 않고 시작점 진단 앞 좌표는 유지한다.</summary>
    [Fact]
    public void SelectedGuardsPreserveOriginalPartialWrites()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash(); var spots = new byte[65536]; var pop = new RawPop(pool, hash, spots); int sid = VoidSlot(factory, pool);
        types[TestType].Flags2 = 0x4000;
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40)); Assert.Equal((ushort)sid, hash.Bucket(1, 30, 40));
        pool.RawSlot(sid)[11] = 4; hash.Reset(); types[TestType].Flags2 = 0x11; types[TestType].FootX = 40;
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 35, 44));
        Assert.Equal(BitConverter.SingleToUInt32Bits(35), Read(pool.Slot(sid), 14)); Assert.Equal(4, pool.Slot(sid)[11]);
        Assert.Equal(0x30, pool.Slot(sid)[22]); Assert.Equal(2, pool.Slot(sid)[23]); Assert.Equal(0xe4, pool.Slot(sid)[24]); Assert.Equal(1, pool.Slot(sid)[25]);
        Assert.All(spots, value => Assert.Equal(0, value)); Assert.Equal((ushort)0, hash.Bucket(1, 35, 44));
    }

    /// <summary>구성·외부 공간 대여 불일치는 효과를 실행하기 전에 거부한다.</summary>
    [Fact]
    public void ConstructorAndForeignSpaceBindingsFailBeforeEffects()
    {
        var pool = new RawSidPool(32768, false); var hash = new RawSquidHash(); var spots = new byte[65536]; var pop = new RawPop(pool, hash, spots);
        var other = new RawSidPool(32768, false); var otherHash = new RawSquidHash();
        Assert.Throws<ArgumentException>(() => new RawPop(other, hash, spots, null, new RecordingPostPop(pool, [])));
        Assert.Throws<ArgumentException>(() => new RawPop(pool, hash, new byte[100]));
        Assert.Throws<ArgumentException>(() => pop.SetBuildingSide(new RecordingBuilding(other, hash, [])));
        Assert.Throws<ArgumentException>(() => pop.SetBuildingSide(new RecordingBuilding(pool, otherHash, [])));
        Assert.Throws<ArgumentException>(() => pop.SetMetadisplay(new RecordingMetadisplay(other, [])));
        Assert.All(spots, value => Assert.Equal(0, value)); Assert.All(HeadSnapshot(hash), value => Assert.Equal(0, value));
    }

    /// <summary>checking0에서도 시작점 진단은 정상 복귀하며 물리0번 칸에 실제 spot을 쓴다.</summary>
    [Fact]
    public void ZeroCellStartDiagnosticResumesPhysicalMap()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); types[TestType].Flags2 = 0x11; types[TestType].FootX = 31;
        var factory = new RawSquidFactory(pool, types); var hash = new RawSquidHash(); var spots = new byte[65536]; var pop = new RawPop(pool, hash, spots); int sid = VoidSlot(factory, pool);
        var lines = new List<uint>(); pop.BindDiagnostics(new SquidPopDiagnostics { Types = types, Assertion = (_, file, line) => { Assert.Equal(@"\Ns\O\Squid.cpp", file); lines.Add(line); } });
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40)); Assert.Equal(new uint[] { 0x6b7 }, lines);
        // 원문 순서대로 왼쪽0번부터 오른쪽30번까지의 동일한 점유 값을 관찰한다.
        for (int x = 0; x <= 30; x++) Assert.Equal(0x11, spots[40 * 256 + x]);
        Assert.Equal((ushort)sid, hash.Bucket(1, 30, 40)); Assert.Equal(0, pool.Slot(sid)[11]);
    }

    /// <summary>미지원 표시 가상표의 도달 실패는 이미 쓴 좌표·체인을 되돌리지 않는다.</summary>
    [Fact]
    public void UnrestoredVirtualFailsAfterSpatialRegistration()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash(); var spots = new byte[65536]; var pop = new RawPop(pool, hash, spots); int sid = VoidSlot(factory, pool, 0x12345678);
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(BitConverter.SingleToUInt32Bits(30), Read(pool.Slot(sid), 14)); Assert.Equal((ushort)sid, hash.Bucket(1, 30, 40));
        Assert.Equal(4, pool.Slot(sid)[11]); Assert.Equal(0, pool.Slot(sid)[40]);
    }

    /// <summary>매몰 섬은 미니맵/spot을 생략하고 같은 원문 부모의0단계 체인에 등록한다.</summary>
    [Fact]
    public void BuriedIslandSkipsMinimapAndSpots()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); types[TestType].Flags2 = 2;
        var factory = new RawSquidFactory(pool, types); var hash = new RawSquidHash(); var spots = new byte[65536]; var log = new List<string>();
        var pop = new RawPop(pool, hash, spots); pop.SetMetadisplay(new RecordingMetadisplay(pool, log)); int sid = VoidSlot(factory, pool, BaseVtable, 8);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40)); Assert.Empty(log); Assert.All(spots, value => Assert.Equal(0, value)); Assert.Equal((ushort)sid, hash.Bucket(0, 30, 40));
    }

    /// <summary>이미 등록됨·contained·0/범위 밖 조기 반환은 물리·TYPE를 더 읽지 않고 그대로 끝난다.</summary>
    [Fact]
    public void AlreadyRegisteredAndInvalidIdsReturnUnchanged()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash(); var spots = new byte[65536]; var pop = new RawPop(pool, hash, spots); int sid = VoidSlot(factory, pool);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40)); byte[] before = pool.Bytes.ToArray();
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(sid, types[TestType], 1, 1, 40, 50)); Assert.Equal(before, pool.Bytes.ToArray());
        pool.RawSlot(sid)[11] = 12; pool.RawSlot(sid)[10] = 255; before = pool.Bytes.ToArray();
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(sid, types[TestType], 1, 1, 40, 50));
        Assert.Equal(RawPopResult.Unchanged, pop.Pop(0, types[TestType], 1, 1, 40, 50)); Assert.Equal(RawPopResult.Unchanged, pop.Pop(pool.Capacity, types[TestType], 1, 1, 40, 50)); Assert.Equal(before, pool.Bytes.ToArray());
    }

    /// <summary>겹침은 좌표와 앞선 OR만 유지하고 미지원 가상표·해시·최초 Pop을 선택하지 않는다.</summary>
    [Fact]
    public void OverlapKeepsCoordinatesAndSkipsUnknownVirtual()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); types[TestType].Flags2 = 0x11; types[TestType].FootX = 2;
        var factory = new RawSquidFactory(pool, types); var hash = new RawSquidHash(); var spots = new byte[65536]; var pop = new RawPop(pool, hash, spots); int sid = VoidSlot(factory, pool, 0x12345678);
        spots[40 * 256 + 30] = 1; Assert.Equal(RawPopResult.Overlap, pop.Pop(sid, types[TestType], 1, 1, 30, 40));
        Assert.Equal(0x11, spots[40 * 256 + 29]); Assert.Equal(1, spots[40 * 256 + 30]); Assert.Equal((ushort)0, hash.Bucket(1, 30, 40));
        Assert.Equal(BitConverter.SingleToUInt32Bits(30), Read(pool.Slot(sid), 14)); Assert.Equal(4, pool.Slot(sid)[11]); Assert.Equal(0, pool.Slot(sid)[40]);
    }

    /// <summary>최신 경계 순서는 미니맵→도달 옆면→표시→현재 가상 후처리이며 사전 Validate는 실행하지 않는다.</summary>
    [Fact]
    public void ConnectionPointsRunInCurrentOriginalOrder()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); types[TestType].Flags2 = OriginalTypeRecord.BuildingMask | 2; types[TestType].FootX = 2; types[TestType].FootY = 3;
        var factory = new RawSquidFactory(pool, types); var hash = new RawSquidHash(); var spots = new byte[65536]; var log = new List<string>(); var post = new RecordingPostPop(pool, log);
        var pop = new RawPop(pool, hash, spots, new RecordingDisplay(log), post); pop.SetBuildingSide(new RecordingBuilding(pool, hash, log)); pop.SetMetadisplay(new RecordingMetadisplay(pool, log)); int sid = VoidSlot(factory, pool);
        hash.Cell(0, 29, 39) = 9; hash.Cell(0, 30, 39) = 10; log.Clear();
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 2, 3, 30, 40));
        Assert.Equal(new[] { "Enqueue:" + sid, "PopSide:29,39", "PopSide:30,39", "Update:0", "PostActivate:11" }, log);
        Assert.Equal((ushort)sid, hash.Bucket(0, 30, 40)); Assert.Equal(127, pool.Slot(sid)[8]); Assert.Equal(0, pool.Slot(sid)[11]); Assert.Equal(0u, post.ActivationDepth);
    }

    /// <summary>명시 가상 몸체의 거부는 이미 완료한 공간/최초 Pop/void 해제와 증가한 깊이를 남긴다.</summary>
    [Fact]
    public void ExplicitBodyFailurePreservesActivationPrefix()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash(); var spots = new byte[65536]; var post = new RecordingPostPop(pool, [], _ => false); var pop = new RawPop(pool, hash, spots, null, post); int sid = VoidSlot(factory, pool);
        Assert.Throws<InvalidOperationException>(() => pop.Pop(sid, types[TestType], 1, 1, 30, 40)); Assert.Equal((ushort)sid, hash.Bucket(1, 30, 40));
        Assert.Equal(0, pool.Slot(sid)[11]); Assert.Equal(0x80, pool.Slot(sid)[40]); Assert.Equal(1u, post.ActivationDepth);
    }

    /// <summary>두 번째 등록은 최초 Pop 비트 없이 가상 후처리하며 PATCH 선행 Validate를 다시 넣지 않는다.</summary>
    [Fact]
    public void SecondPopSkipsFirstBitsAndPreflight()
    {
        var pool = new RawSidPool(32768, false); OriginalTypeRecord[] types = Types(); var factory = new RawSquidFactory(pool, types);
        var hash = new RawSquidHash(); var spots = new byte[65536]; var log = new List<string>(); var pop = new RawPop(pool, hash, spots, null, new RecordingPostPop(pool, log)); var unpop = new RawUnpop(pool, hash, spots); int sid = VoidSlot(factory, pool);
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40)); unpop.Unpop(sid, types[TestType]); pool.RawSlot(sid)[40] |= 1;
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[TestType], 1, 1, 30, 40)); Assert.Equal(new[] { "PostActivate:11", "PostActivate:10" }, log.Where(entry => entry.StartsWith("PostActivate:", StringComparison.Ordinal)));
        Assert.DoesNotContain(log, entry => entry.StartsWith("PostValidate:", StringComparison.Ordinal));
    }

    /// <summary>정확한 원문 DWORD 비트를 읽어 좌표의 저장 시점을 관찰한다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset) => System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(raw[offset..]);
}
