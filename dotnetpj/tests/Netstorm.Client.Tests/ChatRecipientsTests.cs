using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Chat;
using Netstorm.Core.Original;

// 원문350과 실제 식별자 접합48관측의 전체 지역/trace를 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>고정 대상 검사와 기존 실제 RawPlayerIdentity 접합 검증이다.</summary>
public sealed class ChatRecipientsTests
{
    /// <summary>선택된 하위 장치의 명시 중단이다.</summary>
    private sealed class Stop : Exception
    {
        /// <summary>원문 중단 위치 이름이다.</summary>
        public string Where { get; }
        /// <summary>이름만 전달하며 상태를 복구하지 않는다.</summary>
        public Stop(string where) { Where = where; }
    }
    /// <summary>원문 실제 BYTE 문자열로만 복호화한다.</summary>
    private static string Bytes(string value) => value == "-" ? "" : Encoding.Latin1.GetString(Convert.FromHexString(value));
    /// <summary>전체 BYTE를 원문 소문자hex로 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>좁은 이름/번역 trace만 같은 BYTE로 직렬화한다.</summary>
    private static string Hex(string value) => Hex(Encoding.Latin1.GetBytes(value));
    /// <summary>실제60BYTE 지역의 현재NUL 이름을 읽는다.</summary>
    private static string Name(byte[] bytes) { int count = bytes.AsSpan(8).IndexOf((byte)0); Assert.True(count >= 0); return Encoding.Latin1.GetString(bytes.AsSpan(8, count)); }
    /// <summary>원문350입력의 전체 부분쓰기/외부 순서/명시 중단을 대조한다.</summary>
    [Fact]
    public void ReplaysIndependentNative350()
    {
        int rows = 0, stops = 0;
        // 일반 헤더1행을 버리고 원문 관측만 소비한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-recipients-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(8, row.Length); uint manager = uint.Parse(row[0], CultureInfo.InvariantCulture), mode = uint.Parse(row[2], CultureInfo.InvariantCulture); string? text = row[1] == "~" ? null : Bytes(row[1]); byte[] scratch = new byte[60]; scratch.AsSpan().Fill(0xa5); var trace = new List<string>(); int resolutions = 0;
            var hooks = new ChatRecipientHooks
            {
                Resolve = record => { string name = Name(record); trace.Add("R:" + Hex(name)); resolutions++; if (mode == 5 && resolutions == 2) throw new Stop("resolve"); if (mode == 4 && name == "Unknown") Encoding.Latin1.GetBytes("EVERYONE\0").CopyTo(record, 8); return mode == 2 || (mode != 1 && name is "Alice" or "ALICE"); },
                Translate = key => { trace.Add("T:" + Hex(key)); if (mode == 6) throw new Stop("translate"); return mode == 3 && key == "EVERYONE" ? "Tous" : key; },
            };
            string status = "normal"; bool result = false;
            try { result = ChatRecipients.CheckChatRecipients(manager, text, scratch, hooks); } catch (Stop stopped) { status = stopped.Where; stops++; }
            Assert.Equal(row[3], status); if (status == "normal") Assert.Equal(row[4] == "1", result); if (row[5] != "-") Assert.Equal(row[5], Hex(scratch)); Assert.Equal(row[6], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(350, rows); Assert.True(stops > 0);
    }
    /// <summary>실제 Player9/Chat100/RawIdentity를 같은 원문48입력에 사용한다.</summary>
    [Fact]
    public void ActualIdentityReplaysNative48()
    {
        int rows = 0;
        // 기대 출력은 원문에서만 읽고 하위는 실제 식별자 본체로 처리한다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-recipient-identity-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(6, row.Length); uint scene = uint.Parse(row[0], CultureInfo.InvariantCulture); byte[] players = new byte[1620], chat = new byte[4800];
            Encoding.Latin1.GetBytes("LocalUser\0").CopyTo(players, 192); BinaryPrimitives.WriteUInt32LittleEndian(players.AsSpan(224, 4), 100); Encoding.Latin1.GetBytes("Alice\0").CopyTo(players, 372); BinaryPrimitives.WriteUInt32LittleEndian(players.AsSpan(404, 4), 200);
            Encoding.Latin1.GetBytes("Bob\0").CopyTo(chat, 0); BinaryPrimitives.WriteUInt32LittleEndian(chat.AsSpan(44, 4), 300); Encoding.Latin1.GetBytes(new string('B', 40) + '\0').CopyTo(chat, 48); BinaryPrimitives.WriteUInt32LittleEndian(chat.AsSpan(92, 4), 400);
            var identity = new RawPlayerIdentity(OriginalEdition.Patch1078, players, [], chat, new() { LocalOwner = () => 1, Subscriber = () => 100, Address = () => 0, Name = () => "LocalUser", Battle = () => scene == 1, Editor = () => scene == 2, Metamap = () => false }); byte[] scratch = new byte[60]; scratch.AsSpan().Fill(0xa5); var trace = new List<string>();
            bool result = ChatRecipients.CheckChatRecipients(1, Bytes(row[1]), scratch, new() { Resolve = record => { trace.Add("R:" + Hex(Name(record))); return identity.Resolve(record); }, Translate = key => { trace.Add("T:" + Hex(key)); return key; } });
            Assert.Equal(row[2] == "1", result); Assert.Equal(row[3], Hex(scratch)); Assert.Equal(row[4], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(48, rows);
    }
    /// <summary>자연 관문은 scratch를 보존하고 VT/FF는 이름의 실제 일부다.</summary>
    [Fact]
    public void LazyDevicesAndExactSeparatorsKeepNativeBehavior()
    {
        byte[] scratch = new byte[60]; scratch.AsSpan().Fill(0xa5); byte[] before = (byte[])scratch.Clone(); Assert.False(ChatRecipients.CheckChatRecipients(0, "Alice", scratch, new())); Assert.Equal(before, scratch); Assert.False(ChatRecipients.CheckChatRecipients(1, null, scratch, new())); Assert.Equal(before, scratch);
        Assert.True(ChatRecipients.CheckChatRecipients(1, " \r\n\t", scratch, new())); Assert.Throws<InvalidOperationException>(() => ChatRecipients.CheckChatRecipients(1, "Alice", scratch, new())); Assert.True(ChatRecipients.CheckChatRecipients(1, "A\vB\fC", scratch, new() { Resolve = _ => true })); Assert.Equal("A\vB\fC", Name(scratch));
    }
}
