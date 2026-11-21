using System.Buffers.Binary;
using Netstorm.Core.Original;

// 압축의 지역 복사와 같은 owner 캐시/현재TYPE의 소비 순서를 검사한다.
namespace Netstorm.Tests;

/// <summary>16필드 마스크와 실제35BYTE 복원/진단 뒤 현재 상태/부분 실패를 확인한다.</summary>
public sealed class RawSquidCompressedReceiveTests
{
    /// <summary>원문WORD 마스크와 BYTE 개수/실제 본문을 구성한다.</summary>
    private static byte[] Packet(ushort mask, params byte[] data)
    {
        byte[] packet = new byte[9 + data.Length]; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(4, 2), 15000); BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(6, 2), mask); packet[8] = (byte)data.Length; data.CopyTo(packet, 9); return packet;
    }
    /// <summary>선택TYPE의같은레코드만 빌리며 기본HP 함수는 필요한 검사에서만 제공한다.</summary>
    private static SquidCompressedReceiveHooks Hooks(OriginalTypeRecord type) => new() { DefaultType = () => 82, Type = number => { Assert.Equal(82u, number); return type; } };

    /// <summary>지역 데이터 복사 뒤callback이입력을바꿔도원래입력과현재선택TYPE의HP/zOrder를소비한다.</summary>
    [Fact]
    public void AllSelectedFieldsKeepCopiedDataAndPartialWidthDefaults()
    {
        byte[] packet = Packet(0x3b75, 5, 7, 0x34, 70, 2, 3, 5, 9, 0xaa, 0xbb);
        var state = new RawSquidTransmissionState { LastOwner = 2, Update = new(0x2c, 0x81, 0x2345) }; var type = new OriginalTypeRecord { Flags1 = 0x10, ZOrder = 1 }; var trace = new List<string>();
        byte[] image = RawSquidCompressedReceive.Decode(packet, state, new()
        {
            DefaultType = () => { trace.Add("default"); packet.AsSpan(9).Fill(0); packet[8] = 0; return 82; },
            Type = number => { trace.Add("type"); Assert.Equal(70u, number); return type; },
            MaximumHp = selected => { trace.Add("hp"); Assert.Same(type, selected); type.ZOrder = -2; return 1000; },
        });
        byte[] expected = new byte[35]; expected[0] = 1; expected[1] = 0x2c; BinaryPrimitives.WriteUInt16LittleEndian(expected.AsSpan(2, 2), 0x2345); BinaryPrimitives.WriteUInt16LittleEndian(expected.AsSpan(4, 2), 15000);
        BinaryPrimitives.WriteSingleLittleEndian(expected.AsSpan(6, 4), 5); BinaryPrimitives.WriteSingleLittleEndian(expected.AsSpan(10, 4), 7); expected[14] = 0x34; expected[16] = 70; expected[17] = 0x12; expected[18] = 3;
        BinaryPrimitives.WriteUInt16LittleEndian(expected.AsSpan(27, 2), 0x305); expected[29] = 9; expected[30] = 0xaa; expected[34] = 0xbb;
        Assert.Equal(expected, image); Assert.Equal(3u, state.LastOwner); Assert.Equal(new[] { "default", "type", "hp" }, trace);
    }

    /// <summary>생략 필드의 생성자 기본값과 현재 캐시의 하위BYTE/선택TYPE의HP·zOrder를 그대로 유지한다.</summary>
    [Fact]
    public void EmptyMaskUsesConstructorDefaultsAndCurrentSelectedRecord()
    {
        var state = new RawSquidTransmissionState { LastOwner = 300 }; var type = new OriginalTypeRecord { Flags1 = 0x10, ZOrder = -2 }; var hooks = Hooks(type); hooks.MaximumHp = _ => -1;
        byte[] image = RawSquidCompressedReceive.Decode(Packet(0), state, hooks);
        Assert.Equal(35, image.Length); Assert.Equal(27, BinaryPrimitives.ReadUInt16LittleEndian(image.AsSpan(2, 2))); Assert.Equal(-10000f, BinaryPrimitives.ReadSingleLittleEndian(image.AsSpan(6, 4))); Assert.Equal(-10000f, BinaryPrimitives.ReadSingleLittleEndian(image.AsSpan(10, 4)));
        Assert.Equal(82, image[16]); Assert.Equal(44, image[18]); Assert.Equal(65535, BinaryPrimitives.ReadUInt16LittleEndian(image.AsSpan(27, 2))); Assert.Equal(254, image[29]); Assert.Equal(300u, state.LastOwner);
    }

    /// <summary>서로상충하는두좌표마스크의진단이복귀하면뒤필드가앞좌표를덮어쓴다.</summary>
    [Fact]
    public void ConflictingCoordinateMasksContinueInOriginalOrder()
    {
        byte[] data = new byte[10]; data[0] = 5; data[1] = 7; BinaryPrimitives.WriteSingleLittleEndian(data.AsSpan(2, 4), 1.25f); BinaryPrimitives.WriteSingleLittleEndian(data.AsSpan(6, 4), -2.5f);
        var hooks = Hooks(new()); var lines = new List<uint>(); hooks.Assertion = (_, file, line) => { Assert.Equal(@".\Clientboard.cpp", file); lines.Add(line); };
        byte[] image = RawSquidCompressedReceive.Decode(Packet(3, data), new(), hooks);
        Assert.Equal(new uint[] { 0x14f, 0x15c }, lines); Assert.Equal(1.25f, BinaryPrimitives.ReadSingleLittleEndian(image.AsSpan(6, 4))); Assert.Equal(-2.5f, BinaryPrimitives.ReadSingleLittleEndian(image.AsSpan(10, 4)));
    }

    /// <summary>crazy owner 진단이복귀한뒤현재캐시를다시읽고별도초기값으로치환하지않는다.</summary>
    [Fact]
    public void SameOwnerSentinelRereadsCurrentCacheAfterDiagnostic()
    {
        var state = new RawSquidTransmissionState { LastOwner = 0xffffff9d }; var hooks = Hooks(new()); int assertions = 0;
        hooks.Assertion = (expression, _, line) => { assertions++; Assert.Equal("hackySameAsLastPlayerId != CRAZY_PLAYER_ID", expression); Assert.Equal(0x1b0u, line); state.LastOwner = 11; };
        byte[] image = RawSquidCompressedReceive.Decode(Packet(0x80), state, hooks); Assert.Equal(11, image[18]); Assert.Equal(11u, state.LastOwner); Assert.Equal(1, assertions);
    }

    /// <summary>owner 갱신뒤더늦은미정의읽기가실패해도앞선캐시저장은유지한다.</summary>
    [Fact]
    public void LaterTruncatedFieldKeepsEarlierOwnerCacheWrite()
    {
        var state = new RawSquidTransmissionState { LastOwner = 2 }; var hooks = Hooks(new()); var lines = new List<uint>(); hooks.Assertion = (expression, _, line) => { Assert.Equal("index <= dataLength", expression); lines.Add(line); };
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidCompressedReceive.Decode(Packet(0x500, 5, 0x80), state, hooks)); Assert.Equal(5u, state.LastOwner); Assert.Equal(new uint[] { 0x137 }, lines);
    }

    /// <summary>첫 두 좌표의BYTE 읽기 부족은 별도128진단이며 범위 밖 스택을0으로 합성하지 않는다.</summary>
    [Fact]
    public void ByteReadUnderflowHasOriginalDistinctDiagnostic()
    {
        var hooks = Hooks(new()); int assertions = 0; hooks.Assertion = (_, _, line) => { assertions++; Assert.Equal(0x128u, line); };
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidCompressedReceive.Decode(Packet(1, 7), new(), hooks)); Assert.Equal(1, assertions);
    }

    /// <summary>원문14/15는 데이터를 소비하지 않으며64BYTE 지역 복사 상한은 장치 조회보다 먼저 검사한다.</summary>
    [Fact]
    public void ReservedMaskBitsIgnoreDataAndCopyBoundsPrecedeHooks()
    {
        byte[] image = RawSquidCompressedReceive.Decode(Packet(0xc000, Enumerable.Repeat((byte)0xff, 64).ToArray()), new(), Hooks(new())); Assert.Equal(82, image[16]);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidCompressedReceive.Decode(Packet(0, new byte[65]), new(), new()));
        byte[] shortBody = Packet(0); shortBody[8] = 1; Assert.Throws<ArgumentOutOfRangeException>(() => RawSquidCompressedReceive.Decode(shortBody, new(), new()));
    }
}
