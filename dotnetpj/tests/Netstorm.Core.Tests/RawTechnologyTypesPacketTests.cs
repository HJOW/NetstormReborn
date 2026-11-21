using System.Buffers.Binary;
using Netstorm.Core.Original;

// 69h의 NUL 목록·지정 폭·진단과 이전 부분 저장을 관측한다.
namespace Netstorm.Core.Tests;

/// <summary>기술 목록의 실제 현재 물리와 수신/송신 순서를 검사한다.</summary>
public sealed class RawTechnologyTypesPacketTests
{
    /// <summary>길이BYTE를 수신 경계로 사용하지 않고 기존/신규 전체 unsigned BYTE를 순서대로 표시한다.</summary>
    [Fact]
    public void ReceiveUnionsThenIndicatesWholeCurrentListWithoutLengthOrTypeFiltering()
    {
        byte[] types = new byte[256]; types[0] = 5;
        byte[] packet = [0xff, 0xff, 0, 0, 0, 5, 0x80, 0xff, 0x80, 0, 77];
        var calls = new List<uint>();
        RawTechnologyTypesPacket.Receive(packet, types, calls.Add);
        Assert.Equal(new byte[] { 5, 0x80, 0xff, 0 }, types[..4]);
        Assert.Equal(new uint[] { 5, 0x80, 0xff }, calls);
        Assert.All(types.Skip(3), value => Assert.Equal(0, value));
    }

    /// <summary>표시 효과가 바꾼 다음 항목과 종료점은 사본 대신 현재 목록에서 읽는다.</summary>
    [Fact]
    public void IndicationRereadsNextEntryAfterEachEffect()
    {
        byte[] types = new byte[256]; types[0] = 1; types[1] = 2; types[2] = 3;
        var calls = new List<uint>();
        RawTechnologyTypesPacket.Receive(new byte[6], types, type => { calls.Add(type); if (type == 1) { types[1] = 200; types[2] = 0; } });
        Assert.Equal(new uint[] { 1, 200 }, calls);
    }

    /// <summary>입력 끝의 NUL 부족은 앞선 추가를 보존하며 표시 단계에 아직 진입하지 않는다.</summary>
    [Fact]
    public void MissingInputTerminatorKeepsPreviousStoresBeforeAnyIndication()
    {
        byte[] types = new byte[256]; types[0] = 7; int calls = 0;
        Assert.Throws<ArgumentOutOfRangeException>(() => RawTechnologyTypesPacket.Receive(new byte[] { 0, 0, 0, 0, 0, 8, 9 }, types, _ => calls++));
        Assert.Equal(new byte[] { 7, 8, 9, 0 }, types[..4]); Assert.Equal(0, calls);
    }

    /// <summary>종료 없는 손상 목록은 실제256번째 경계에서 실패하고 바깥 물리를 쓰지 않는다.</summary>
    [Fact]
    public void FullCorruptListFailsAtActualPhysicalBoundary()
    {
        byte[] types = Enumerable.Repeat((byte)1, 256).ToArray(); byte[] before = types.ToArray();
        Assert.Throws<ArgumentOutOfRangeException>(() => RawTechnologyTypesPacket.Add(new byte[] { 2, 0 }, types));
        Assert.Equal(before, types);
    }

    /// <summary>표시 장치는 첫 비영 항목에서만 필요하며 합집합 저장 이후의 실패를 되돌리지 않는다.</summary>
    [Fact]
    public void EmptyListNeedsNoIndicationButSelectedMissingEffectPreservesUnion()
    {
        byte[] types = new byte[256];
        RawTechnologyTypesPacket.Receive(new byte[6], types, null);
        Assert.Throws<InvalidOperationException>(() => RawTechnologyTypesPacket.Receive(new byte[] { 0, 0, 0, 0, 0, 71, 0 }, types, null));
        Assert.Equal(71, types[0]); Assert.Equal(0, types[1]);
    }

    /// <summary>진단 인자와 복귀 뒤 현재BYTE를 쓰며 선언WORD 외 부분 물리는 유지한다.</summary>
    [Fact]
    public void OutgoingAssertionUsesCurrentByteAndPreservesPartialStorage()
    {
        byte[] all = Enumerable.Repeat((byte)0xd3, 20).ToArray(); Memory<byte> packet = all.AsMemory(7, 8); packet.Span[4] = 0xff;
        byte[] expected = all.ToArray(); expected[11] = 2; BinaryPrimitives.WriteUInt16LittleEndian(expected.AsSpan(9, 2), 7); int calls = 0;
        RawTechnologyTypesPacket.Observe(packet, 1, (expression, file, line) =>
        {
            calls++; Assert.Equal("len != (unsigned char)-1", expression); Assert.Equal(@"\Ns\O\_p_packets.cpp", file); Assert.Equal(0xa5fu, line); packet.Span[4] = 2;
        });
        Assert.Equal(1, calls); Assert.Equal(expected, all);
    }

    /// <summary>복귀형 진단에서FF가 그대로면260을 쓰며 진단 누락/중단은 기존 길이를 보존한다.</summary>
    [Fact]
    public void ReturningUnchangedSentinelAndStoppedAssertionKeepDistinctEffects()
    {
        byte[] packet = Enumerable.Repeat((byte)0xff, 5).ToArray(); byte[] before = packet.ToArray();
        Assert.Throws<InvalidOperationException>(() => RawTechnologyTypesPacket.Observe(packet, 1, null)); Assert.Equal(before, packet);
        Assert.Throws<IOException>(() => RawTechnologyTypesPacket.Observe(packet, 1, (_, _, _) => throw new IOException("선택 진단 중단"))); Assert.Equal(before, packet);
        RawTechnologyTypesPacket.Observe(packet, 1, (_, _, _) => { });
        Assert.Equal(new byte[] { 0xff, 0xff, 4, 1, 0xff }, packet);
    }
}
