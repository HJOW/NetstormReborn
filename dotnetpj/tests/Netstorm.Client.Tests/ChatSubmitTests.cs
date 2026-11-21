using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// 원문 제출/수신자 추가의 모든 물리와 실제 확장/반납 장치를 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>독립383관측과 실제 manager/수신자 수명 검사다.</summary>
public sealed class ChatSubmitTests
{
    /// <summary>원문 manager/이전 버퍼/새 버퍼의 격리 식별자다.</summary>
    private const uint Manager = 0x12010000, Buffer = 0x12001000, NewBuffer = 0x12070000;
    /// <summary>원문 하위 장치의 명시 중단이다.</summary>
    private sealed class Stop : Exception
    {
        /// <summary>정상/물리 실패와 구분하는 실제 중단 이름이다.</summary>
        public string Where { get; }
        /// <summary>동일 중단 위치만 전달한다.</summary>
        public Stop(string where) { Where = where; }
    }
    /// <summary>명시 입력/장치 응답의 같은 DWORD를 쓴다.</summary>
    private static void Put(byte[] bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at, 4), value);
    /// <summary>이미 정의된 실제 DWORD만 읽는다.</summary>
    private static uint Word(byte[] bytes, int at) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(at, 4));
    /// <summary>원문 전체 물리 hex이며 빈 배열은 '-'다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => bytes.IsEmpty ? "-" : Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>관측의 명시 BYTE 입력을 복호화한다.</summary>
    private static byte[] Bytes(string hex) => hex == "-" ? [] : Convert.FromHexString(hex);
    /// <summary>NUL 앞의 실제 장치 이름만 읽는다.</summary>
    private static string Text(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0); Assert.True(end >= 0); return Encoding.Latin1.GetString(bytes[..end]);
    }
    /// <summary>trace 문자열의 빈 값은 '-' 없이 빈 hex다.</summary>
    private static string TextHex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();
    /// <summary>빈/여섯/100행의 원문 독립 기록 입력이다.</summary>
    private static byte[] Records(uint scene)
    {
        byte[] records = new byte[4800]; uint[] subscribers = [11, 22, 11, 0, 33, 44], flags = [0x20, 0x40, 0x28, 0xff, 0x80, 0xa0];
        // 중복/빈 구독자와 부호 있는 flags도 같은 원시 입력으로 공급한다.
        for (int index = 0; index < 100; index++)
        {
            if (scene == 0 || (scene == 1 && index >= 6)) continue;
            records[index * 48 + 40] = unchecked((byte)(scene == 2 ? 0x20 : flags[index])); Put(records, index * 48 + 44, scene == 2 ? (uint)index + 11 : subscribers[index]);
        }
        return records;
    }
    /// <summary>383관측의 전체 manager/이전·새 배열/지역/호출 순서를 독립 대조한다.</summary>
    [Fact]
    public void ReplaysIndependentNativeFullState()
    {
        int rows = 0, stops = 0;
        // 일반 헤더1행 뒤의 실제383관측만 소비한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-submit-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(16, row.Length); uint scene = uint.Parse(row[2], CultureInfo.InvariantCulture), mode = uint.Parse(row[3], CultureInfo.InvariantCulture), initialCount = uint.Parse(row[4], CultureInfo.InvariantCulture), capacity = uint.Parse(row[5], CultureInfo.InvariantCulture), subscriber = uint.Parse(row[6], CultureInfo.InvariantCulture);
            var memory = new Dictionary<uint, byte[]>(); byte[] manager = new byte[2712]; memory[Manager] = manager;
            // 원문이 안 쓰는 모든 manager BYTE도 최초 입력 패턴으로 대조한다.
            for (int index = 0; index < manager.Length; index++) manager[index] = unchecked((byte)(165 + index * 17));
            Put(manager, 0, initialCount); Put(manager, 4, capacity); Put(manager, 8, Buffer); Put(manager, 12, 7); Put(manager, 16, 0xaabbccdd); byte[] old = new byte[capacity * 4]; memory[Buffer] = old;
            // 이전 capacity 전체를 정의해 count 이상 복사도 관찰한다.
            for (uint index = 0; index < capacity; index++) Put(old, (int)index * 4, 100 + index);
            byte[] records = Records(scene), input = Bytes(row[1]), identity = new byte[60], work = Bytes(row[13]); identity.AsSpan().Fill(0xa5); if (row[11] != "-") Bytes(row[11]).CopyTo(identity, 0);
            var trace = new List<string>(); var allocations = new List<uint>(); uint next = NewBuffer; int resolves = 0;
            var hooks = new ChatSubmitHooks
            {
                Memory = token => memory[token],
                Allocate = size => { trace.Add("N:" + size); if (mode == 6) return 0; uint token = next; next = unchecked(next + ((size + 15) & ~15u)); byte[] bytes = new byte[size]; bytes.AsSpan().Fill(0xd3); memory[token] = bytes; allocations.Add(token); return token; },
                Release = token => { trace.Add("F:" + token); if (mode == 7) throw new Stop("free"); },
                Assertion = number => { trace.Add("A:" + number); throw new Stop("assertion"); },
                Resolve = record =>
                {
                    string name = Text(record.AsSpan(8)); trace.Add("R:" + TextHex(name)); resolves++; if (mode == 5 && resolves == 2) throw new Stop("resolve");
                    if (mode == 3 && name == "Unknown") Encoding.Latin1.GetBytes("ALLIES\0").CopyTo(record, 8);
                    bool accepted = mode == 2 || (mode != 1 && name is "Alice" or "Bob"); if (accepted) Put(record, 4, name == "Bob" ? 22u : 11u); return accepted;
                },
                Translate = key => { trace.Add("T:" + TextHex(key)); return mode == 4 && key == "EVERYONE" ? "Tous" : key; },
                Records = () => records, InputVisible = visible => trace.Add("V:" + visible), Dispatch = text => trace.Add("D:" + TextHex(Text(text))),
            };
            string status = "normal";
            try { if (row[0] == "submit") RawChatSubmit.SubmitChatInput(Manager, input, identity, work, hooks); else RawChatSubmit.AddChatRecipient(Manager, subscriber, hooks); }
            catch (Stop stop) { status = stop.Where; stops++; }
            Assert.True(status == row[7], $"row={rows + 1} kind={row[0]} status={status} expected={row[7]}"); Assert.Equal(row[8], Hex(manager)); Assert.Equal(row[9], Hex(old));
            string buffers = string.Join('|', allocations.Select(token => token + ":" + Hex(memory[token]))); Assert.Equal(row[10], buffers.Length == 0 ? "-" : buffers); if (row[12] != "-") Assert.Equal(row[12], Hex(identity)); if (row[14] != "-") Assert.Equal(row[14], Hex(work)); Assert.Equal(row[15], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(383, rows); Assert.Equal(7, stops);
    }
    /// <summary>실제100수신자 확장/이전 버퍼 반납/중복 재제출과 current 교체 뒤 이전 manager 유지다.</summary>
    [Fact]
    public void ActualOwnerGrowsRecipientsAndRetainsExplicitOldManager()
    {
        var owner = new ClientChatManagerGlobals(); uint manager = owner.Allocate(RawChatManagerPrefix.ManagerBytes); RawChatManagerPrefix.ConstructChatManager(manager, owner.MakePrefixHooks()); owner.Current.Value = manager;
        uint old = Word(owner.Bytes(manager), 8); var hooks = owner.MakeSubmitHooks(); byte[] records = Records(2), identity = new byte[60], work = new byte[RawChatSubmit.WorkBytes]; identity.AsSpan().Fill(0xa5); work.AsSpan().Fill(0xa5); string message = ""; int sends = 0;
        hooks.Resolve = _ => false; hooks.Translate = key => key; hooks.Records = () => records; hooks.Dispatch = text => { message = Text(text); sends++; };
        RawChatSubmit.SubmitChatInput(manager, "EVERYONE:hello"u8, identity, work, hooks); Assert.Equal("hello", message); Assert.Equal(1, sends); Assert.Equal(100u, Word(owner.Bytes(manager), 0)); Assert.Equal(128u, Word(owner.Bytes(manager), 4));
        uint expanded = Word(owner.Bytes(manager), 8); Assert.Equal(512, owner.Bytes(expanded).Length); Assert.Throws<ArgumentOutOfRangeException>(() => owner.Bytes(old));
        // 실제 기록 순서와 같은100개 DWORD이며 중복을 추가하지 않는다.
        for (int index = 0; index < 100; index++) Assert.Equal((uint)index + 11, Word(owner.Bytes(expanded), index * 4));
        uint other = owner.Allocate(RawChatManagerPrefix.ManagerBytes); owner.Current.Value = other; RawChatSubmit.SubmitChatInput(manager, "EVERYONE:again"u8, identity, work, hooks); Assert.Equal("again", message); Assert.Equal(2, sends); Assert.Equal(other, owner.Current.Value); Assert.Equal(expanded, Word(owner.Bytes(manager), 8)); Assert.Equal(100u, Word(owner.Bytes(manager), 0)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.ReleaseRecipientBuffer(manager));
    }
    /// <summary>빈 본문도 먼저 실제 수신자 필드를 갱신하고 도달한 입력줄/분배 장치만 요구한다.</summary>
    [Fact]
    public void EmptyBodyAndSelectedDispatchFollowActualRecipientWrites()
    {
        var owner = new ClientChatManagerGlobals(); uint manager = owner.Allocate(RawChatManagerPrefix.ManagerBytes); RawChatManagerPrefix.ConstructChatManager(manager, owner.MakePrefixHooks()); var hooks = owner.MakeSubmitHooks(); byte[] identity = new byte[60], work = new byte[RawChatSubmit.WorkBytes]; int hides = 0;
        hooks.InputVisible = visible => { Assert.Equal(0u, visible); hides++; }; RawChatSubmit.SubmitChatInput(manager, ":"u8, identity, work, hooks); Assert.Equal(1, hides); Assert.Equal(1u, Word(owner.Bytes(manager), 12)); Assert.Equal(0u, Word(owner.Bytes(manager), 16));
        hooks.Records = () => new byte[4800]; Assert.Throws<InvalidOperationException>(() => RawChatSubmit.SubmitChatInput(manager, "hello"u8, identity, work, hooks)); Assert.Equal(1, hides); Assert.Equal(0x20u, Word(owner.Bytes(manager), 16));
    }
    /// <summary>지역 물리가 부족해도 선행 두DWORD만 쓴 뒤 후속 입력이나 장치를 요구하지 않는다.</summary>
    [Fact]
    public void ShortWorkKeepsOnlyTwoSuccessfulPrefixStores()
    {
        var owner = new ClientChatManagerGlobals(); uint manager = owner.Allocate(RawChatManagerPrefix.ManagerBytes); byte[] bytes = owner.Bytes(manager); bytes.AsSpan().Fill(0xa5);
        Assert.Throws<ArgumentOutOfRangeException>(() => RawChatSubmit.SubmitChatInput(manager, [], [], new byte[4096], owner.MakeSubmitHooks())); Assert.Equal(0u, Word(bytes, 0)); Assert.Equal(0u, Word(bytes, 12)); Assert.Equal(0xa5a5a5a5u, Word(bytes, 4)); Assert.Equal(0xa5a5a5a5u, Word(bytes, 16));
    }
}
