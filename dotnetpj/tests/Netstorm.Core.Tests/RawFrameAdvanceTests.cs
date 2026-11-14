using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 독립 PE 관측을 실제 진행·지정·공간 모듈과 현재 자산 계약에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>프레임 진행의 한 번 감싸기·signed 넘침·현재 표·같은 풀 연결을 검사한다.</summary>
public sealed class RawFrameAdvanceTests
{
    /// <summary>원본 진행 입력의 대상 SID다.</summary>
    private const int Target = 50;
    /// <summary>원본 SHP 추가 헤더의 여덟 칸 크기다.</summary>
    private static readonly float[][] Sizes = [[1, 1], [2, 2], [2.5f, 1], [3, 4], [4, 4.5f], [0.5f, 9], [2, 1.5f], [4, 4]];
    /// <summary>원본 입력의 flags1·flags2·프레임 수다.</summary>
    private static readonly uint[][] Kinds = [[0x802, 4, 8], [0x28000803, 2, 8], [0x28000003, 0x1000000, 8], [0x69012, 0x210000, 4]];

    /// <summary>현재 표를 교체할 수 있도록 같은 순서의 코드 입력을 만든다. 결과 구간은 제품 코드가 계산한다.</summary>
    private static RiftTypeFrames Codes(string letters) => new(letters.Select((side, index) =>
        new RiftFrameCode((byte)side, (byte)(80 + index % 2), (byte)(11 - index), (byte)(index & 1))));

    /// <summary>판본의 실제 생성자 주소가 든 TYPE 표를 만든다.</summary>
    private static OriginalTypeRecord[] Types(OriginalEdition edition) => Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171)
        .Select(number => new OriginalTypeRecord { ConstructorAddress = OriginalConstructors.Address(edition, number) }).ToArray();

    /// <summary>패치 DWORD/CD BYTE의 필드만 쓰고 이웃 바이트는 보존한다.</summary>
    private static void PutFrame(RawSidPool pool, int sid, uint frame)
    {
        Span<byte> raw = pool.AllocatedBytes(sid);
        if (pool.Edition == OriginalEdition.Patch1078) BinaryPrimitives.WriteUInt32LittleEndian(raw[36..], frame);
        else raw[34] = unchecked((byte)frame);
    }

    /// <summary>패치 DWORD/CD BYTE를 원본 관측 문자열과 같은 unsigned 값으로 읽는다.</summary>
    private static uint Frame(RawSidPool pool, int sid) => pool.Edition == OriginalEdition.Patch1078
        ? BinaryPrimitives.ReadUInt32LittleEndian(pool.Slot(sid)[36..]) : pool.Slot(sid)[34];

    /// <summary>원본과 같은 client 할당으로 SID50까지 실제 활성 영역에 넣는다.</summary>
    private static RawSidPool Pool(OriginalEdition edition)
    {
        var pool = new RawSidPool(24000, false, edition: edition);
        // 기록된 SID가 모두 실제 할당 범위 안에 있도록 순서대로 확보한다.
        for (int sid = 5; sid <= Target; sid++) Assert.Equal(sid, pool.Allocate(SidAllocation.Client));
        return pool;
    }

    /// <summary>세 원본 PE의 각7680관측을 실제 방향 구간→진행→지정으로 전수 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void Native7680PerEdition(string edition)
    {
        bool patch = edition == "originals"; var pool = Pool(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        OriginalTypeRecord[] types = Types(pool.Edition);
        RiftTypeFrames[] codes = types.Select(_ => Codes("")).ToArray(); var events = new List<string>();
        string[][] all = X86Fixture.Read("frameadvance"); Assert.Equal(23040, all.Length); int checkedRows = 0;
        X86Fixture.CheckRows(all, row =>
        {
            if (row[0] != edition) return true;
            Assert.Equal(10, row.Length); uint[] kind = Kinds[X86Fixture.Int(row[2])];
            types[82].Flags1 = kind[0]; types[82].Flags2 = kind[1]; codes[82] = Codes(X86Fixture.Int(row[1]) == 0 ? "AAABBJJJ" : "JAJBAPJP");
            Span<byte> raw = pool.AllocatedBytes(Target); raw.Clear(); BinaryPrimitives.WriteUInt32LittleEndian(raw, 0x11010000);
            raw[10] = 82; BinaryPrimitives.WriteSingleLittleEndian(raw[14..], 20.75f); BinaryPrimitives.WriteSingleLittleEndian(raw[18..], 21.9f);
            PutFrame(pool, Target, X86Fixture.UInt(row[4])); raw[patch ? 33 : 31] = (byte)X86Fixture.UInt(row[3]); events.Clear();
            var setter = new SquidFrame(pool, types, new SquidFrameHooks
            {
                FrameSize = (_, frame) =>
                {
                    if (patch) Assert.InRange(frame, 0, checked((int)(kind[2] * ((kind[0] & 0x440000) != 0 ? 2u : 1u))) - 1);
                    return Sizes[frame];
                },
                Update = (sid, flags) => events.Add($"{(flags == 0 ? 'D' : 'E')}:{sid}:{Frame(pool, sid)}"),
                Unpop = (sid, flags) => events.Add($"U:{sid}:{flags}:{Frame(pool, sid)}"),
                Pop = (sid, x, y, flags) => events.Add($"P:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{flags}:{Frame(pool, sid)}"),
            });
            var advance = new RawFrameAdvance(pool, codes, setter); bool wrapped = advance.Advance(Target, X86Fixture.Int(row[5]), X86Fixture.UInt(row[6]));
            string observed = events.Count == 0 ? "-" : string.Join(';', events); checkedRows++;
            return (wrapped ? 1u : 0u) == X86Fixture.UInt(row[7]) && observed == row[8]
                && Convert.ToHexStringLower(pool.Slot(Target)[..(patch ? 50 : 36)]) == row[9];
        });
        Assert.Equal(7680, checkedRows);
    }

    /// <summary>생성 시 TYPE/효과의 사본과 명시 현재 표 대여·복귀를 구별한다.</summary>
    [Fact]
    public void SetterOwnsSnapshotAndBorrowsOnlyExplicitCurrentTypes()
    {
        var pool = Pool(OriginalEdition.Patch1078); OriginalTypeRecord[] types = Types(pool.Edition); types[82].Flags2 = 4;
        Span<byte> raw = pool.AllocatedBytes(Target); raw.Clear(); raw[10] = 82; int sizes = 0, updates = 0;
        var hooks = new SquidFrameHooks
        {
            FrameSize = (_, _) => { sizes++; return [5, 5]; }, Update = (_, _) => updates++,
            Unpop = (_, _) => throw new InvalidOperationException("미도달 공간 해제"), Pop = (_, _, _, _) => throw new InvalidOperationException("미도달 공간 등록"),
        };
        var setter = new SquidFrame(pool, types, hooks); types[82].Flags2 = 0; hooks.Update = (_, _) => throw new InvalidOperationException("외부 함수 객체 교체");
        Assert.Equal(0, setter.StoreLevel(Target)); Assert.Equal(0, sizes);
        setter.BindCurrentTypes(types); Assert.Equal(3, setter.StoreLevel(Target)); Assert.Equal(1, sizes);
        types[82] = new OriginalTypeRecord { Flags2 = 2 }; Assert.Equal(0, setter.StoreLevel(Target)); Assert.Equal(1, sizes);
        setter.BindCurrentTypes(null); Assert.Equal(0, setter.StoreLevel(Target));
        setter.BindCurrentTypes([]); setter.Set(Target, 7, 0); Assert.Equal(2, updates); Assert.Equal(1, sizes);
        Assert.Throws<ArgumentException>(() => setter.BindCurrentTypes(types[..187])); Assert.Equal(7u, Frame(pool, Target));
    }

    /// <summary>현재 코드 교체를 반영하고 잘못된 풀·타입·현재프레임·방향·flags를 효과 전에 거부한다.</summary>
    [Fact]
    public void CurrentFramesAndInvalidInputsPreserveOrder()
    {
        var pool = Pool(OriginalEdition.Patch1078); OriginalTypeRecord[] types = Types(pool.Edition); types[82].Flags2 = 4;
        RiftTypeFrames[] codes = types.Select(_ => Codes("")).ToArray(); codes[82] = Codes("AAAB"); int updates = 0;
        var setter = new SquidFrame(pool, types, new SquidFrameHooks
        {
            FrameSize = (_, _) => throw new InvalidOperationException("섬에서 미도달 SHP"), Update = (_, _) => updates++,
            Unpop = (_, _) => throw new InvalidOperationException("미도달 해제"), Pop = (_, _, _, _) => throw new InvalidOperationException("미도달 등록"),
        });
        Assert.Throws<ArgumentException>(() => new RawFrameAdvance(Pool(pool.Edition), codes, setter));
        Assert.Throws<ArgumentException>(() => new RawFrameAdvance(pool, [], setter));
        var advance = new RawFrameAdvance(pool, codes, setter); Span<byte> raw = pool.AllocatedBytes(Target); raw.Clear(); raw[10] = 82;
        Assert.False(advance.Advance(Target, 1)); Assert.Equal(1u, Frame(pool, Target));
        codes[82] = Codes("ABBB"); Assert.False(advance.Advance(Target, 1)); Assert.Equal(2u, Frame(pool, Target)); Assert.Equal(4, updates);
        PutFrame(pool, Target, 3); codes[82] = Codes("ABQZ"); string before = Convert.ToHexStringLower(raw);
        Assert.Throws<ArgumentOutOfRangeException>(() => advance.Advance(Target, 1)); Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Target)));
        PutFrame(pool, Target, uint.MaxValue); Assert.Throws<ArgumentOutOfRangeException>(() => advance.Advance(Target, 1));
        PutFrame(pool, Target, 0); codes[82] = Codes("ABBB"); before = Convert.ToHexStringLower(raw);
        Assert.Throws<ArgumentException>(() => advance.Advance(Target, 1, 3)); Assert.Equal(before, Convert.ToHexStringLower(pool.Slot(Target)));
        var shortTable = new RawFrameAdvance(pool, codes[..80], setter); Assert.Throws<ArgumentOutOfRangeException>(() => shortTable.Advance(Target, 1));
        codes[82] = new RiftTypeFrames([new RiftFrameCode(255, 0, 0, 0)]);
        Assert.Equal(-1, RawPathAnimation.Side(codes[82], 0)); Assert.Equal(-66, RawPathAnimation.Direction(codes[82], 0)); Assert.Equal(4, updates);
    }

    /// <summary>실제 생성자·Pop/Unpop 해시를 통해 현재 프레임 기준 단계가 한 호출 늦게 옮겨지는지 확인한다.</summary>
    [Fact]
    public void ExplicitSpaceEffectsUseActualFactoryAndHash()
    {
        // 실제 공장까지 연결한 제품 대상 판본이다. CD는 독립 원본 관측으로만 대조한다.
        const OriginalEdition edition = OriginalEdition.Patch1078;
        var pool = new RawSidPool(32768, true, edition: edition); OriginalTypeRecord[] types = Types(edition);
        types[155].Flags1 = 0x28000002; types[155].Flags2 = 0x2000000; types[155].FootX = types[155].FootY = 1;
        var hash = new RawSquidHash(); var spots = new byte[65536]; var unpop = new RawUnpop(pool, hash, spots); var pop = new RawPop(pool, hash, spots);
        var factory = new RawSquidFactory(pool, types, unpop: unpop); int sid = factory.Create(155); float[][] sizes = [[1, 1], [2, 1.5f], [5, 5], [6, 6]];
        Assert.Equal(RawPopResult.Registered, pop.Pop(sid, types[155], 1, 1, 20, 20)); var effects = new List<string>();
        var setter = new SquidFrame(pool, types, new SquidFrameHooks
        {
            FrameSize = (_, frame) => sizes[frame], Update = (_, _) => effects.Add("update"),
            Unpop = (target, flags) => { effects.Add("unpop"); unpop.Unpop(target, types[155], flags); },
            // 현행 C# Pop의 명시 SHP 입력 경계를 사용한다. 최신 일반 Pop/표시 어댑터는 별도 이식 대상이다.
            Pop = (target, x, y, flags) => { effects.Add("pop"); float[] size = sizes[Frame(pool, target)]; Assert.Equal(RawPopResult.Registered, pop.Pop(target, types[155], size[0], size[1], x, y, flags)); },
        });
        RiftTypeFrames[] codes = types.Select(_ => Codes("")).ToArray(); codes[155] = Codes("AAAA"); var advance = new RawFrameAdvance(pool, codes, setter);
        Assert.False(advance.Advance(sid, 1)); Assert.Equal(new[] { "update", "update" }, effects); Assert.Equal((ushort)sid, hash.Bucket(1, 20, 20));
        effects.Clear(); Assert.False(advance.Advance(sid, 1)); Assert.Equal((ushort)sid, hash.Bucket(1, 20, 20)); Assert.Equal(2u, Frame(pool, sid));
        effects.Clear(); Assert.False(advance.Advance(sid, 1)); Assert.Equal(new[] { "unpop", "pop" }, effects); Assert.Equal((ushort)0, hash.Bucket(1, 20, 20)); Assert.Equal((ushort)sid, hash.Bucket(3, 20, 20));
        effects.Clear(); Assert.True(advance.Advance(sid, 1)); Assert.Equal((ushort)sid, hash.Bucket(3, 20, 20));
        effects.Clear(); Assert.False(advance.Advance(sid, 1)); Assert.Equal(new[] { "unpop", "pop" }, effects); Assert.Equal((ushort)sid, hash.Bucket(1, 20, 20)); Assert.Equal((ushort)0, hash.Bucket(3, 20, 20));
    }
}
