using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본 옵션 수신과 재진입 뒤 현재 길이/관문 및 부분 복사를 검사한다.
namespace Netstorm.Core.Tests;

/// <summary>67h 전체 부모의 원본18입력과 실제 물리/현재값 경계를 검증한다.</summary>
public sealed class StateBattleOptionsTests
{
    /// <summary>검사 입력 패킷의 원문 길이 WORD만 바꾼다.</summary>
    private static void Length(byte[] packet, ushort value) => BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(5), value);
    /// <summary>정해진 payload 입력만 만들며 수신 결과를 계산하지 않는다.</summary>
    private static byte[] Packet(int length)
    {
        byte[] packet = new byte[length + 7]; packet[0] = 1; packet[1] = 0x67; BinaryPrimitives.WriteUInt16LittleEndian(packet.AsSpan(2), (ushort)packet.Length); Length(packet, (ushort)length);
        // 복사 전후와 꼬리를 구별할 서로 다른 명시 입력 바이트다.
        for (int index = 0; index < length; index++) packet[7 + index] = unchecked((byte)(index * 17 + 3));
        return packet;
    }

    /// <summary>기존 독립 클라이언트 fixture 중67h18입력의 전체 옵션·파생값·효과를 그대로 대조한다.</summary>
    [Fact]
    public void OriginalOptionReceiver18()
    {
        string[][] rows = X86Fixture.Read("mission-client-packets").Where(row => row[0].StartsWith("103,")).ToArray(); Assert.Equal(18, rows.Length); var seen = new HashSet<string>();
        // 각 원본 입력의 현재 옵션 원문을 독립적으로 공급한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0])); uint[] input = row[0].Split(',').Select(X86Fixture.UInt).ToArray(); byte[] options = new byte[87], packet = X86Fixture.Bytes(row[1]); Array.Fill(options, (byte)0xa5); uint energy = 0; var value = new UIntBox { Value = 10 }; var trace = new List<string>();
            RawStateBattleOptions.Receive(input[8], packet, new()
            {
                Options = options, Challenge = () => input[6] & 1, Value = value, Battle = () => true, Mission = () => input[3] != 0, Saved = () => true,
            }, new()
            {
                Assertion = text => trace.Add("Assert," + Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant()), ChallengeOptions = (sender, actual) => { Assert.True(actual.Equals(packet.AsMemory())); trace.Add("Outpost," + sender); },
                Range = () => trace.Add("Range"), Cost = () => { trace.Add("Energy"); return 1251; }, StoreCost = number => { trace.Add("Type,122"); energy = BitConverter.SingleToUInt32Bits(number); },
                CurrentValue = () => { trace.Add("Option"); return 17; }, Reconnect = () => trace.Add("ReconnectUI"),
            });
            byte[] storage = X86Fixture.Bytes(row[5]); Assert.Equal(storage.AsSpan(6, 87).ToArray(), options); Assert.Equal(BinaryPrimitives.ReadUInt32LittleEndian(storage.AsSpan(103)), energy);
            Assert.Equal(X86Fixture.UInt(row[4].Split(',')[6]), value.Value); Assert.Equal(row[7], string.Join(';', trace));
        }
    }

    /// <summary>길이 진단 복귀 뒤 WORD를 다시 읽고 각 callback이 바꾼 현재 전역을 다음 효과에서 읽는다.</summary>
    [Fact]
    public void ReturningAssertionAndCallbacksChooseCurrentState()
    {
        byte[] packet = Packet(88), options = new byte[87]; Array.Fill(options, (byte)0xa5); var value = new UIntBox { Value = 10 }; var trace = new List<string>(); uint challenge = 0; bool battle = false, mission = true, saved = false;
        RawStateBattleOptions.Receive(9, packet, new()
        {
            Options = options, Value = value, Challenge = () => { trace.Add("challenge"); return challenge; }, Battle = () => { trace.Add("battle"); return battle; }, Mission = () => { trace.Add("mission"); return mission; }, Saved = () => { trace.Add("saved"); return saved; },
        }, new()
        {
            Assertion = text => { trace.Add(text); Length(packet, 5); challenge = 1; },
            ChallengeOptions = (sender, original) => { trace.Add("outpost"); Assert.Equal(9u, sender); Assert.True(original.Equals(packet.AsMemory())); Assert.Equal(packet.AsSpan(7, 5).ToArray(), options[..5]); packet[7] = 222; },
            Range = () => { trace.Add("range"); battle = true; }, Cost = () => { trace.Add("cost"); return int.MinValue; }, StoreCost = number => { trace.Add("store"); Assert.Equal(BitConverter.SingleToUInt32Bits((float)int.MinValue), BitConverter.SingleToUInt32Bits(number)); mission = false; },
            CurrentValue = () => { trace.Add("value"); saved = true; return 77; }, Reconnect = () => { trace.Add("reconnect"); Assert.Equal(77u, value.Value); },
        });
        Assert.Equal(3, options[0]); Assert.All(options[5..], item => Assert.Equal(0xa5, item));
        Assert.Equal(new[] { "z->dataLen <= sizeof(globalBattleOptions)", "challenge", "outpost", "range", "cost", "store", "value", "battle", "mission", "saved", "reconnect" }, trace);
    }

    /// <summary>현재 관문의 단락 평가가 비선택 장치를 요구하지 않음을 확인한다.</summary>
    [Theory]
    [InlineData(false, false, false)]
    [InlineData(false, false, true)]
    [InlineData(false, true, false)]
    [InlineData(false, true, true)]
    [InlineData(true, false, false)]
    [InlineData(true, false, true)]
    [InlineData(true, true, false)]
    [InlineData(true, true, true)]
    public void ReconnectUsesShortCircuitCurrentGates(bool battle, bool mission, bool saved)
    {
        int calls = 0; bool selected = battle && !mission && saved;
        RawStateBattleOptions.Receive(9, Packet(0), new()
        {
            Options = new byte[87], Challenge = () => 0, Battle = () => battle, Mission = battle ? () => mission : null, Saved = battle && !mission ? () => saved : null,
        }, new() { Range = () => { }, Cost = () => 0, StoreCost = _ => { }, CurrentValue = () => 7, Reconnect = selected ? () => calls++ : null });
        Assert.Equal(selected ? 1 : 0, calls);
    }

    /// <summary>같은 배열의 앞방향 DWORD 복사는 뒤쪽 중첩에서 원본처럼 먼저 쓴 값을 다시 읽는다.</summary>
    [Theory]
    [InlineData(3)]
    [InlineData(11)]
    public void CopyUsesActualForwardDwordsWithOverlap(int destination)
    {
        byte[] packet = Packet(24), original = (byte[])packet.Clone(); Length(packet, 12);
        RawStateBattleOptions.Receive(9, packet, new() { Options = packet.AsMemory(destination, 12), Challenge = () => 0, Battle = () => false }, new() { Range = () => { }, Cost = () => 0, StoreCost = _ => { }, CurrentValue = () => 0 });
        byte[] expected = destination < 7 ? original.AsSpan(7, 12).ToArray() : Enumerable.Repeat(original.AsSpan(7, 4).ToArray(), 3).SelectMany(bytes => bytes).ToArray();
        Assert.Equal(expected, packet.AsSpan(destination, 12).ToArray());
    }

    /// <summary>불완전한 다음DWORD·BYTE와 늦은 하위 누락 전까지 완료한 쓰기를 되돌리지 않는다.</summary>
    [Theory]
    [InlineData(10, 87, 8)]
    [InlineData(12, 6, 4)]
    [InlineData(5, 4, 4)]
    public void PhysicalCopyFailurePreservesCompletedPrefix(int available, int capacity, int copied)
    {
        byte[] full = Packet(12), packet = full[..(7 + available)], options = new byte[capacity]; Array.Fill(options, (byte)0xa5); if (available == 5) Length(packet, 5);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawStateBattleOptions.Receive(9, packet, new() { Options = options }, new()));
        Assert.Equal(full.AsSpan(7, copied).ToArray(), options[..copied]); Assert.All(options[copied..], item => Assert.Equal(0xa5, item));
    }

    /// <summary>복사가 끝난 뒤 미연결 range에서 실패해도 원본 옵션/이전 파생값은 유지한다.</summary>
    [Fact]
    public void MissingDerivedEffectPreservesCopiedBytesAndOldValue()
    {
        byte[] packet = Packet(5), options = new byte[87]; var value = new UIntBox { Value = 10 };
        Assert.Throws<InvalidOperationException>(() => RawStateBattleOptions.Receive(9, packet, new() { Options = options, Value = value, Challenge = () => 0 }, new()));
        Assert.Equal(packet[7..], options[..5]); Assert.Equal(10u, value.Value);
    }
}
