using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 원문 설정의 읽기 순서·현재 수신/장기F를 같은 실제 버퍼로 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>독립164입력과 callback 뒤 현재값·부분 store·장기 canonical 대여를 대조한다.</summary>
public sealed class MissionSettingsPacketTests
{
    /// <summary>같은 실제 DWORD만 입력으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>전체 원문 바이트의 fixture 표현이다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>원본에 공급한 현재7DWORD 입력만 생성한다.</summary>
    private static uint[] Values(uint profile, uint seed)
    {
        uint[] values = new uint[7];
        if (profile == 1) Array.Fill(values, uint.MaxValue);
        else if (profile == 2) values = [0, 1, 0x80000000, 0x7fffffff, uint.MaxValue, 3, 16];
        else if (profile == 3)
        {
            // 각 현재 필드를 서로 다른 명시DWORD로 구별한다.
            for (int index = 0; index < values.Length; index++) values[index] = unchecked(0x12345678u + (uint)index * 0x01010101 + seed);
        }
        else if (profile >= 4) { ReadOnlySpan<uint> bits = [1, 0x80000000, uint.MaxValue]; values[(profile - 4) / 3] = bits[(int)((profile - 4) % 3)]; }
        return values;
    }
    /// <summary>기대값을 계산하지 않고 원본 외부 효과가 공급한 현재값 변경만 적용한다.</summary>
    private static void Change(uint[] words, int field, uint step, uint seed)
    {
        // 이미 읽은 필드 이외의 다음 현재값만 바꾼다.
        for (int index = 0; index < words.Length; index++) if (index != field) words[index] ^= unchecked(0x9e3779b9u + seed + step * 17 + (uint)index);
    }
    /// <summary>정렬 가정 없이 현재7DWORD 전체를 직렬화한다.</summary>
    private static string SettingsHex(uint[] words)
    {
        byte[] bytes = new byte[28];
        // payload 필드 순서로 현재 실제 값만 표현한다.
        for (int index = 0; index < words.Length; index++) Put(bytes, index * 4, words[index]); return Hex(bytes);
    }
    /// <summary>선택되지 않은 분배 효과는 빈 관측이고 실제 호출만 알린다.</summary>
    private static PacketDispatchHooks DispatchHooks(List<string> trace) => new() { Now = () => { trace.Add("N"); return 1; }, Record = (_, _, _) => trace.Add("D"), Terminate = _ => trace.Add("T"), Close = _ => trace.Add("C"), Release = _ => trace.Add("F") };

    /// <summary>원본117송신/45수신/2observer 입력의 전체 packet/설정/효과 순서를 비교한다.</summary>
    [Fact]
    public void OriginalSettingsPacket164()
    {
        string[][] rows = X86Fixture.Read("mission-settings-packet").Where(row => row[0] != "input").ToArray(); Assert.Equal(164, rows.Length); var seen = new HashSet<string>();
        // 각 독립 입력은 새 실제 저장소에서 실행한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0])); string[] input = row[0].Split(','); uint profile = X86Fixture.UInt(input[1]), seed = X86Fixture.UInt(input[2]), target = X86Fixture.UInt(input[3]), mode = X86Fixture.UInt(input[4]); uint[] current = Values(profile, seed); byte[] packet = new byte[32];
            // 수신/observer의 원문 헤더도 native 명시 입력 그대로다.
            for (int index = 0; index < packet.Length; index++) packet[index] = unchecked((byte)(index * 37 + 11));
            if (input[0] != "S")
            {
                // 수신에 공급한 현재payload 입력이며 수신 결과 모델이 아니다.
                for (int index = 0; index < current.Length; index++) Put(packet, 4 + index * 4, current[index]);
            }
            if (input[0] == "R")
            {
                // 수신 전 전역은 같은 packet과 독립된 stale 입력이다.
                for (int index = 0; index < current.Length; index++) current[index] = 0xcccc0000u + (uint)index;
            }
            var trace = new List<string>(); uint step = 0;
            /// <summary>관측 뒤 다음 현재 getter/payload 입력만 native와 같이 변경한다.</summary>
            void Mutate(int field, bool receive)
            {
                step++; if (mode != 2 && !(mode == 1 && step == 1)) return;
                if (!receive) Change(current, field, step, seed);
                else
                {
                    uint[] words = new uint[7];
                    // callback이 바꾸는 같은 현재payload의 명시 입력이다.
                    for (int index = 0; index < words.Length; index++) words[index] = BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(4 + index * 4));
                    Change(words, field, step, seed);
                    // 실제 다음 read가 볼 원문 버퍼에 그대로 쓴다.
                    for (int index = 0; index < words.Length; index++) Put(packet, 4 + index * 4, words[index]);
                }
            }
            var settings = new RawMissionSettingsPacket(OriginalEdition.Patch1078, new()
            {
                Current = field => { int index = (int)field; uint value = current[index]; trace.Add($"R,{index},{value}"); Mutate(index, false); return value; },
                SetCurrent = (field, value) => { int index = (int)field; trace.Add($"W,{index},{value}"); current[index] = value; Mutate(index, true); },
                Send = (recipient, bytes) => { trace.Add("S," + unchecked((uint)recipient)); Assert.Equal(32, bytes.Length); bytes.Span.CopyTo(packet); },
            });
            if (input[0] == "S") settings.Send(unchecked((int)target)); else if (input[0] == "R") settings.Receive(packet);
            else { Assert.Equal("O", input[0]); var observed = new List<string>(); var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, DispatchHooks(observed)); MissionSettingsRegistration.Register(dispatch, settings); dispatch.Registration(0x32).NotifyObserver(Memory<byte>.Empty, mode); Assert.Empty(observed); }
            Assert.Equal(row[1], Hex(packet)); Assert.Equal(row[2], SettingsHex(current)); Assert.Equal(row[3], trace.Count == 0 ? "-" : string.Join(';', trace));
        }
    }

    /// <summary>장기 실제32h의 같은 canonical F와 metadata는 짧은 typed/배열 등록으로 교체하지 않는다.</summary>
    [Fact]
    public void RegistrationBorrowsActualCanonicalBaseline()
    {
        var trace = new List<string>(); var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, DispatchHooks(trace)); uint[] current = new uint[7];
        dispatch.RegisterCanonicalDirect(0x32, 0x0044dc20, (_, _, packet) => RawMissionSettingsPacket.ReceiveCurrent(current, packet)); dispatch.Version(0x32, 3, 5); dispatch.Observe(0x32, direction => trace.Add("observer:" + direction)); CanonicalPacketReceiver? selected = dispatch.Registration(0x32).CanonicalReceive;
        var settings = new RawMissionSettingsPacket(OriginalEdition.Patch1078, new() { Current = _ => 0 }); MissionSettingsRegistration.Register(dispatch, settings); MissionSettingsRegistration.Register(dispatch, new uint[7]);
        Assert.Same(selected, dispatch.Registration(0x32).CanonicalReceive); Assert.Equal(3, dispatch.Registration(0x32).Version); Assert.Equal(5, dispatch.Registration(0x32).Mismatch); dispatch.Registration(0x32).NotifyObserver(Memory<byte>.Empty, 1); Assert.Equal(new[] { "observer:1" }, trace);
        byte[] bytes = new byte[32]; Put(bytes, 4, 77); dispatch.InvokeDirect(0x32, 9, null, bytes); Assert.Equal(77u, current[0]);
    }

    /// <summary>실제 일반 분배의 현재 설정 쓰기와 잘린 다음DWORD 앞의 store 보존을 확인한다.</summary>
    [Fact]
    public void ActualDispatchAndPartialStoreUseCurrentOwner()
    {
        var trace = new List<string>(); var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, DispatchHooks(trace)); uint[] values = Values(3, 7); byte[] packet = new byte[32]; var settings = new RawMissionSettingsPacket(OriginalEdition.Patch1078, new() { Current = field => values[(int)field], SetCurrent = (field, value) => values[(int)field] = value, Send = (_, bytes) => bytes.Span.CopyTo(packet) });
        settings.Send(9); uint[] expected = (uint[])values.Clone(); Array.Clear(values); MissionSettingsRegistration.Register(dispatch, settings); PacketRegistration entry = dispatch.Registration(0x32); Assert.Equal(32, entry.Length); Assert.Equal(1, entry.Version); Assert.Equal(0, entry.Mismatch); Assert.NotNull(entry.Observer);
        var node = new PacketConnection { Sender = 9 }; Assert.True(dispatch.Dispatch(node, packet)); Assert.Equal(expected, values); Assert.Equal(new[] { "N", "D" }, trace); Assert.Equal(1u, node.Received); Assert.Equal(0u, node.Depth);
        Array.Clear(values); Assert.Throws<ArgumentOutOfRangeException>(() => settings.Receive(packet.AsMemory(0, 10))); Assert.Equal(expected[0], values[0]); Assert.Equal(0u, values[1]);
        packet[2] = 31; Assert.False(dispatch.Validate(packet));
    }
}
