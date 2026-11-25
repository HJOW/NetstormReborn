// AI 에너지 부족 좌표의 실제 본체/목록·순환·효과 뒤 재조회를 검사한다.
using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

namespace Netstorm.Core.Tests;

// AI 부족 좌표를 검사하는 클래스다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.
// 근거: cpppj tests/AiDrySpotsTests.cpp의 14건 전부다.
public sealed class RawAiDrySpotsTests
{
    /// <summary>실제 크기의 소유 블록과 명시 에너지/로그 입력을 준비한다. 2026-11-25 R4-5b 부족 좌표 이식에서 추가됐다.</summary>
    private sealed class DryScene
    {
        /// <summary>미정 바이트를 보존할 같은 소유 등록부다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x48000000u, bytes => bytes.Fill(0xa5));

        /// <summary>실제 본체 토큰이다.</summary>
        public readonly uint Token;

        /// <summary>20개 12바이트 기록 backing이다.</summary>
        public readonly uint Backing;

        /// <summary>명시 에너지 결과다.</summary>
        public uint Result;

        /// <summary>로그 횟수다.</summary>
        public int Logs;

        /// <summary>같은 판본의 실제 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768, true, null, OriginalEdition.Patch1078);

        /// <summary>실제 효과 묶음이다.</summary>
        public AiDrySpotHooks Hooks = new();

        /// <summary>실제 판정 인자와 로그 분류다.</summary>
        public readonly List<(float X, float Y, uint Type, uint Owner)> Calls = [];

        /// <summary>로그된 가짜 여부다.</summary>
        public readonly List<bool> FakeLogs = [];

        /// <summary>원본 생성자의 내장 목록 필드만 설정한다.</summary>
        public DryScene()
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            Backing = Memory.Allocate(120);
            Field(0xadc, 10);
            Field(0xae0, 0);
            Field(0xae4, Backing);
            Field(0xae8, 0);
            Field(0xaec, 2);
            Field(0x280, 73);
            Field(0xb00, 1);
            Hooks.Energy = (text, x, y, type, owner) =>
            {
                Assert.Equal(0, text.Length);
                Calls.Add((x, y, type, owner));
                text.Append('s');
                return Result;
            };
            Hooks.Log = (brain, fake) =>
            {
                Assert.Equal(Token, brain);
                Logs++;
                FakeLogs.Add(fake);
            };
        }

        /// <summary>현재 본체 DWORD를 읽는다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => BinaryPrimitives.ReadUInt32LittleEndian(Memory.Bytes(Token).AsSpan(at, 4));

        /// <summary>선택된 본체 DWORD만 쓴다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Memory.Bytes(Token).AsSpan(at, 4), value);

        /// <summary>현재 backing의 기록 한 DWORD를 읽는다.</summary>
        /// <param name="index">기록 번호다.</param>
        /// <param name="field">칸 안 순서(0:x, 1:y, 2:type)다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Item(int index, int field) => BinaryPrimitives.ReadUInt32LittleEndian(Memory.Bytes(Field(0xae4)).AsSpan(index * 12 + field * 4, 4));

        /// <summary>같은 본체·풀과 명시 하위로 제품을 조립한다.</summary>
        /// <returns>검사할 부족 좌표 몸체다.</returns>
        public RawAiDrySpots Make() => new(Memory, Pool, Hooks);

        /// <summary>원본 자기 경계와 비교 영역 경계를 서로 다른 좌표로 설정한다.</summary>
        public void SeedBounds()
        {
            Field(0x248, 0x20);
            Field(0x270, BitConverter.SingleToUInt32Bits(10.0f));
            Field(0x274, BitConverter.SingleToUInt32Bits(20.0f));
            Field(0x278, BitConverter.SingleToUInt32Bits(30.0f));
            Field(0x27c, BitConverter.SingleToUInt32Bits(60.0f));
            Field(0x2b8, BitConverter.SingleToUInt32Bits(110.0f));
            Field(0x2bc, BitConverter.SingleToUInt32Bits(120.0f));
            Field(0x2c0, BitConverter.SingleToUInt32Bits(130.0f));
            Field(0x2c4, BitConverter.SingleToUInt32Bits(160.0f));
        }
    }

    /// <summary>값을 리틀 엔디언으로 바이트에 쓴다.</summary>
    /// <param name="bytes">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.Slice(offset, 4), value);

    /// <summary>좌표만 중복 검사하며 signed 0은 같고 NaN은 원본처럼 서로 다르다. TYPE 비트는 그대로 저장한다.</summary>
    [Fact]
    public void AddCoordinatesAndRawTypeBits()
    {
        var input = new DryScene();
        Assert.True(input.Make().Add(input.Token, 0.0f, 4.0f, 73));
        Assert.False(input.Make().Add(input.Token, -0.0f, 4.0f, 99));
        float nan = BitConverter.UInt32BitsToSingle(0x7fc12345u);
        Assert.True(input.Make().Add(input.Token, nan, 4.0f, 0x7fc01234u));
        Assert.True(input.Make().Add(input.Token, nan, 4.0f, 0xdeadbeefu));
        Assert.Equal(3u, input.Field(0xae0));
        Assert.Equal(0u, input.Item(0, 0));
        Assert.Equal(0x7fc12345u, input.Item(1, 0));
        Assert.Equal(0x7fc01234u, input.Item(1, 2));
        Assert.Equal(0xdeadbeefu, input.Item(2, 2));
        Assert.Empty(input.Calls);
    }

    /// <summary>full/빈 목록은 포인터에 접근하지 않으며 빈 검사는 cursor를 변경하지 않는다.</summary>
    [Fact]
    public void FullAndEmptyAvoidBackingAccess()
    {
        var input = new DryScene();
        input.Field(0xae4, 0);
        input.Field(0xae0, 10);
        input.Hooks = new AiDrySpotHooks();
        Assert.False(input.Make().Add(input.Token, 1.0f, 2.0f, 73));
        input.Field(0xae0, 0);
        input.Field(0xae8, 7);
        Assert.Equal(0u, input.Make().Scan(input.Token));
        Assert.Equal(7u, input.Field(0xae8));
        Assert.Equal(0u, input.Make().Maintain(input.Token));
        Assert.Equal(0, input.Logs);
    }

    /// <summary>원래 cursor+1의 한 항목만 조회하고 제거해도 shift된 항목용 cursor 보정을 추가하지 않는다.</summary>
    [Fact]
    public void ScanOneAndPreservesOriginalCursor()
    {
        var input = new DryScene();
        input.Make().Add(input.Token, 1.0f, 2.0f, 71);
        input.Make().Add(input.Token, 3.0f, 4.0f, 72);
        input.Make().Add(input.Token, 5.0f, 6.0f, 73);
        input.Result = 0;
        Assert.Equal(0u, input.Make().Scan(input.Token));
        Assert.Equal(1u, input.Field(0xae8));
        Assert.Equal(3u, input.Field(0xae0));
        Assert.Single(input.Calls);
        Assert.Equal(72u, input.Calls[0].Type);
        input.Result = 0x80000000u;
        Assert.Equal(73u, input.Make().Scan(input.Token));
        Assert.Equal(2u, input.Field(0xae0));
        Assert.Equal(2u, input.Field(0xae8));
        Assert.Equal(72u, input.Make().Scan(input.Token));
        Assert.Equal(1u, input.Field(0xae0));
        Assert.Equal(1u, input.Field(0xae8));
        Assert.Equal(72u, input.Calls[^1].Type);
    }

    /// <summary>제거는 12바이트 기록을 안정 압축하고 미사용 꼬리/cursor를 그대로 둔다.</summary>
    [Fact]
    public void RemovePreservesTailAndUncheckedEndCount()
    {
        var input = new DryScene();
        input.Make().Add(input.Token, 1.0f, 2.0f, 71);
        input.Make().Add(input.Token, 3.0f, 4.0f, 72);
        input.Make().Add(input.Token, 5.0f, 6.0f, 0x7fc01234u);
        input.Field(0xae8, 2);
        byte[] tail = input.Memory.Bytes(input.Backing)[24..].ToArray();
        input.Make().Remove(input.Token, 0);
        Assert.Equal(2u, input.Field(0xae0));
        Assert.Equal(2u, input.Field(0xae8));
        Assert.Equal(72u, input.Item(0, 2));
        Assert.Equal(0x7fc01234u, input.Item(1, 2));
        Assert.True(tail.SequenceEqual(input.Memory.Bytes(input.Backing)[24..]));
        input.Make().Remove(input.Token, 99);
        Assert.Equal(1u, input.Field(0xae0));
        input.Field(0xae0, 0);
        input.Field(0xae4, 0);
        input.Make().Remove(input.Token, 0);
        Assert.Equal(0xffffffffu, input.Field(0xae0));
    }

    /// <summary>에너지 효과가 backing/cursor/count를 바꾼 경우 현재 기록 TYPE와 압축 대상을 재조회한다.</summary>
    [Fact]
    public void EnergyMutationUsesCurrentBackingAndCursor()
    {
        var input = new DryScene();
        input.Make().Add(input.Token, 1.0f, 2.0f, 71);
        input.Make().Add(input.Token, 3.0f, 4.0f, 72);
        uint replacement = input.Memory.Allocate(120);
        Put(input.Memory.Bytes(replacement), 8, 91);
        Put(input.Memory.Bytes(replacement), 20, 92);
        input.Hooks.Energy = (text, x, y, type, owner) =>
        {
            Assert.Equal(0, text.Length);
            Assert.Equal(3.0f, x);
            Assert.Equal(4.0f, y);
            Assert.Equal(72u, type);
            Assert.Equal(2u, owner);
            Assert.Equal(1u, input.Field(0xae8));
            input.Memory.Free(input.Backing);
            input.Field(0xae4, replacement);
            input.Field(0xae8, 0);
            input.Field(0xae0, 2);
            input.Field(0xaec, 7);
            input.Field(0x280, 91);
            return 1u;
        };
        Assert.Equal(91u, input.Make().Maintain(input.Token));
        Assert.Equal(1u, input.Field(0xae0));
        Assert.Equal(92u, input.Item(0, 2));
        Assert.False(input.Memory.Contains(input.Backing));
        Assert.Equal([true], input.FakeLogs);
    }

    /// <summary>실제 제거 TYPE와 효과 뒤 현재 fake TYPE/디버그 값으로 로그를 선택한다. TYPE 0 제거는 로그하지 않는다.</summary>
    [Fact]
    public void MaintainUsesCurrentFakeAndDebug()
    {
        var input = new DryScene();
        input.Make().Add(input.Token, 1.0f, 2.0f, 73);
        input.Result = 1;
        Assert.Equal(73u, input.Make().Maintain(input.Token));
        Assert.Equal([true], input.FakeLogs);
        input.Make().Add(input.Token, 3.0f, 4.0f, 74);
        Assert.Equal(74u, input.Make().Maintain(input.Token));
        Assert.Equal([true, false], input.FakeLogs);
        input.Hooks.Energy = (_, _, _, _, _) => { input.Field(0xb00, 0); return 1u; };
        input.Hooks.Log = null!;
        input.Make().Add(input.Token, 5.0f, 6.0f, 75);
        Assert.Equal(75u, input.Make().Maintain(input.Token));
        Assert.Equal(2, input.Logs);
        input.Field(0xb00, 1);
        input.Make().Add(input.Token, 7.0f, 8.0f, 0);
        Assert.Equal(0u, input.Make().Maintain(input.Token));
        Assert.Equal(0u, input.Field(0xae0));
        Assert.Equal(2, input.Logs);
    }

    /// <summary>선택된 하위 누락/물리 범위 앞에서도 이미 쓴 cursor는 보존한다. 정상 범위 밖 나눗셈은 명시 거부한다.</summary>
    [Fact]
    public void SelectedMissingAndPhysicalBounds()
    {
        var input = new DryScene();
        input.Make().Add(input.Token, 1.0f, 2.0f, 71);
        input.Make().Add(input.Token, 3.0f, 4.0f, 72);
        input.Hooks.Energy = null!;
        Assert.ThrowsAny<Exception>(() => input.Make().Scan(input.Token));
        Assert.Equal(1u, input.Field(0xae8));
        Assert.Equal(2u, input.Field(0xae0));
        input.Field(0xae4, 0);
        input.Field(0xae8, 0);
        Assert.ThrowsAny<Exception>(() => input.Make().Scan(input.Token));
        Assert.Equal(1u, input.Field(0xae8));
        input.Field(0xae0, 0xffffffffu);
        input.Field(0xae8, 0x7fffffffu);
        Assert.ThrowsAny<Exception>(() => input.Make().Scan(input.Token));
        uint small = input.Memory.Allocate(4);
        Assert.ThrowsAny<Exception>(() => input.Make().Scan(small));
    }

    /// <summary>능력은 하위 BYTE 20h만 검사하며 금지 상태에서 난수/좌표/목록/효과를 바꾸지 않는다.</summary>
    [Fact]
    public void SeedCapabilityUsesOnlyLowByteNoEffects()
    {
        var input = new DryScene();
        input.SeedBounds();
        input.Field(0x248, 0x2000);
        input.Field(0xae4, 0);
        input.Hooks = new AiDrySpotHooks();
        byte[] before = (byte[])input.Memory.Bytes(input.Token).Clone();
        var random = new GameRandom(8u);
        input.Make().Seed(input.Token, random);
        Assert.Equal(8u, random.State());
        Assert.True(before.SequenceEqual(input.Memory.Bytes(input.Token)));
        Assert.Empty(input.Calls);
        Assert.Equal(0, input.Logs);
    }

    /// <summary>공용 난수 한 번으로 여덟 방향 모두를 선택하고 원본 TYPE DWORD와 두 float 비트를 등록한다.</summary>
    [Fact]
    public void SeedEightDirectionsAndCurrentRawType()
    {
        float[][] expected = [[70, 20], [70, 60], [10, 90], [30, 90], [20, 20], [20, 60], [10, 40], [30, 40]];
        // 작은 실제 시드 1..8은 Next8에서 1..7, 0을 반환하므로 전 분기를 원본 생성기로 선택한다.
        for (uint seed = 1; seed <= 8; seed++)
        {
            var input = new DryScene();
            input.SeedBounds();
            input.Field(0x280, 0xdeadbeefu);
            input.Field(0xae8, 7);
            byte[] before = (byte[])input.Memory.Bytes(input.Token).Clone();
            var random = new GameRandom(seed);
            var oracle = new GameRandom(seed);
            uint choice = oracle.Next(8u);
            Assert.Equal(seed % 8, choice);
            input.Make().Seed(input.Token, random);
            uint x = BitConverter.SingleToUInt32Bits(expected[choice][0]);
            uint y = BitConverter.SingleToUInt32Bits(expected[choice][1]);
            Assert.Equal(oracle.State(), random.State());
            Assert.Equal(x, input.Field(0xaf0));
            Assert.Equal(y, input.Field(0xaf4));
            Assert.Equal(1u, input.Field(0xae0));
            Assert.Equal(x, input.Item(0, 0));
            Assert.Equal(y, input.Item(0, 1));
            Assert.Equal(0xdeadbeefu, input.Item(0, 2));
            Assert.Equal(7u, input.Field(0xae8));
            Assert.Empty(input.Calls);
            Assert.Equal(0, input.Logs);
            byte[] after = before.ToArray();
            Put(after, 0xaf0, x);
            Put(after, 0xaf4, y);
            Put(after, 0xae0, 1);
            Assert.True(after.SequenceEqual(input.Memory.Bytes(input.Token)));
        }
    }

    /// <summary>x87 원본의 합/곱 전체 뒤 float 저장과 중간 float 덧셈을 구별한다.</summary>
    [Fact]
    public void SeedStoresAfterWholeSums()
    {
        var input = new DryScene();
        input.SeedBounds();
        input.Field(0x2c0, BitConverter.SingleToUInt32Bits(16777216.0f));
        input.Field(0x2b8, BitConverter.SingleToUInt32Bits(1.0f));
        input.Field(0x278, BitConverter.SingleToUInt32Bits(-16777216.0f));
        input.Field(0x270, BitConverter.SingleToUInt32Bits(3.0f));
        var random = new GameRandom(8u);
        input.Make().Seed(input.Token, random);
        Assert.Equal(BitConverter.SingleToUInt32Bits(1.0f), input.Field(0xaf0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(1.0f), input.Item(0, 0));
    }

    /// <summary>full은 등록을 건너뛰지만 앞서 쓴 가짜 좌표와 공용 난수 소비를 되돌리지 않는다.</summary>
    [Fact]
    public void SeedFullPreservesPointAndConsumesRandom()
    {
        var input = new DryScene();
        input.SeedBounds();
        input.Field(0xadc, 0);
        input.Field(0xae4, 0);
        input.Hooks = new AiDrySpotHooks();
        var random = new GameRandom(8u);
        var expected = new GameRandom(8u);
        expected.Next(8u);
        input.Make().Seed(input.Token, random);
        Assert.Equal(0u, input.Field(0xae0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(70.0f), input.Field(0xaf0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20.0f), input.Field(0xaf4));
        Assert.Equal(expected.State(), random.State());
    }

    /// <summary>중복 좌표의 기존 TYPE를 교체하지 않으며 새 좌표/난수 소비는 유지한다.</summary>
    [Fact]
    public void SeedDuplicateDoesNotReplaceType()
    {
        var input = new DryScene();
        input.SeedBounds();
        Assert.True(input.Make().Add(input.Token, 70.0f, 20.0f, 91));
        var random = new GameRandom(8u);
        input.Make().Seed(input.Token, random);
        Assert.Equal(1u, input.Field(0xae0));
        Assert.Equal(91u, input.Item(0, 2));
        Assert.Equal(73u, input.Field(0x280));
        Assert.Equal(input.Item(0, 0), input.Field(0xaf0));
        Assert.Equal(input.Item(0, 1), input.Field(0xaf4));
    }

    /// <summary>원본에는 fake TYPE 0/지도 경계 검사가 없으므로 선택된 음수 좌표도 같은 목록에 등록한다.</summary>
    [Fact]
    public void SeedAllowsZeroTypeAndOutsideBounds()
    {
        var input = new DryScene();
        input.SeedBounds();
        input.Field(0x280, 0);
        input.Field(0x274, BitConverter.SingleToUInt32Bits(-300.0f));
        var random = new GameRandom(8u);
        input.Make().Seed(input.Token, random);
        Assert.Equal(1u, input.Field(0xae0));
        Assert.Equal(0u, input.Item(0, 2));
        Assert.Equal(BitConverter.SingleToUInt32Bits(-300.0f), input.Item(0, 1));
    }

    /// <summary>등록 위치의 물리 오류는 앞선 좌표/난수 쓰기를 유지하고 count를 가짜 성공으로 증가시키지 않는다.</summary>
    [Fact]
    public void SeedPhysicalFailureRetainsPointAndRandom()
    {
        var input = new DryScene();
        input.SeedBounds();
        input.Field(0xae4, 0);
        var random = new GameRandom(8u);
        var expected = new GameRandom(8u);
        expected.Next(8u);
        Assert.ThrowsAny<Exception>(() => input.Make().Seed(input.Token, random));
        Assert.Equal(expected.State(), random.State());
        Assert.Equal(BitConverter.SingleToUInt32Bits(70.0f), input.Field(0xaf0));
        Assert.Equal(BitConverter.SingleToUInt32Bits(20.0f), input.Field(0xaf4));
        Assert.Equal(0u, input.Field(0xae0));
    }
}
