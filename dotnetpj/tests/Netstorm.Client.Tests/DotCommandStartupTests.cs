using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Scripting;
using Netstorm.Core.Original;

// 점 명령의 인접 표/정적 이미지와 실제 Host 시작 부모의 공유를 검증한다.
namespace Netstorm.Client.Tests;

/// <summary>원문250+12+122입력 및 실제 같은 명령표/manager 초기화 검사다.</summary>
public sealed class DotCommandStartupTests
{
    /// <summary>CRT 복사 뒤 명시 중단이며 물리 실패와 구별한다.</summary>
    private sealed class CopyStop : Exception { }
    /// <summary>고정 원문 파일만 출력의 X86 경로에서 읽는다.</summary>
    private static string PathOf(string name) => Path.Combine(AppContext.BaseDirectory, "X86", name);
    /// <summary>전체 DWORD 원문 값을 문화권과 무관하게 읽는다.</summary>
    private static uint Number(string text) => uint.Parse(text, CultureInfo.InvariantCulture);
    /// <summary>명시 초기 count만 인접 물리에 쓴다.</summary>
    private static void Put(byte[] bytes, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(0xa04, 4), value);
    /// <summary>같은 표에서 현재 count를 직접 읽는다.</summary>
    private static uint Count(byte[] bytes) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(0xa04, 4));
    /// <summary>모든 바이트를 생략 없는 소문자 hex로 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>원문이 작성하기 전 독립 입력 패턴을 전체 물리에 채운다.</summary>
    private static byte[] Input(uint count)
    {
        byte[] bytes = new byte[RawDotCommandRegister.StateBytes];
        // 동일 PE 관측의 비0 초기 입력이며 기대 결과를 만들지 않는다.
        for (int index = 0; index < bytes.Length; index++) bytes[index] = unchecked((byte)(165 + index * 17));
        Put(bytes, count); return bytes;
    }
    /// <summary>두 원문 자료의 지원된 전체 물리/반환/trace와 음수 외부경계를 구분한다.</summary>
    [Theory]
    [InlineData("dot-command-register-x86.tsv", 250, 0)]
    [InlineData("dot-command-register-v2-x86.tsv", 8, 4)]
    public void ReplaysNativeRegisterObservations(string file, int supported, int negative)
    {
        int rows = 0, boundaries = 0;
        // 각각 최초 고정된 관측을 그대로 읽으며 v2를 기존 기대값에 덮지 않는다.
        foreach (string line in File.ReadLines(PathOf(file)))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] row = line.Split('\t'), fields = row[1].Split(','); Assert.Equal(5, fields.Length);
            if (row[2] == "name-copy-stop")
            {
                Assert.Equal("4294967295", fields[0]); Assert.Equal("COPY:-32:0:31", row[5]); Assert.Equal("COPY:531b10:31", row[6]); boundaries++; continue;
            }
            byte[] bytes = Input(Number(fields[0])), name = new byte[checked((int)Number(fields[1]) + 1)]; name.AsSpan(0, name.Length - 1).Fill((byte)'Q');
            string trace = "", completion = "return"; var hooks = new DotCommandRegisterHooks
            {
                Copy = (destination, source, count) =>
                {
                    Assert.True(bytes.AsSpan().Overlaps(destination, out int offset)); trace = $"COPY:{offset}:0:{count}"; RawDotCommandRegister.CopyDotCommandName(destination, source, count);
                    if (fields[4].StartsWith("copy-count", StringComparison.Ordinal)) Put(bytes, Number(fields[4][10..])); else if (fields[4] == "copy-stop") throw new CopyStop();
                },
            };
            uint result = 0;
            try { result = RawDotCommandRegister.RegisterDotCommand(bytes, name, Number(fields[3]), Number(fields[2]), hooks); }
            catch (CopyStop) { completion = "copy-stop"; }
            catch (ArgumentOutOfRangeException) { completion = "physical-store-stop"; }
            Assert.Equal(row[2], completion); Assert.Equal(row[4], Hex(bytes)); Assert.Equal(row[5], trace);
            if (completion == "return") Assert.Equal(Number(row[3]), result);
            else if (completion == "physical-store-stop") { Assert.Equal("STORE:532538:4:0", row[6]); Assert.Equal(66u, Count(bytes)); }
            rows++;
        }
        Assert.Equal(supported, rows); Assert.Equal(negative, boundaries);
    }
    /// <summary>signed 상한 생략·음수 물리 거부·선택 CRT 누락은 서로 다르다.</summary>
    [Fact]
    public void RegisterPreservesNaturalLimitAndSelectedPhysicalFailures()
    {
        byte[] bytes = Input(64), before = (byte[])bytes.Clone(); var hooks = new DotCommandRegisterHooks { Copy = null };
        Assert.Equal(0u, RawDotCommandRegister.RegisterDotCommand(bytes, [], uint.MaxValue, 1, hooks)); Assert.Equal(before, bytes);
        Put(bytes, 0); Assert.Throws<InvalidOperationException>(() => RawDotCommandRegister.RegisterDotCommand(bytes, [], 1, 1, hooks));
        // 지원 밖 음수 주소는 CRT를 실제 호출하기 전에 명시적으로 거부한다.
        foreach (uint value in new uint[] { uint.MaxValue, 0x80000000, 0x80000001 })
        {
            bytes = Input(value); before = (byte[])bytes.Clone(); int copies = 0;
            Assert.Throws<ArgumentOutOfRangeException>(() => RawDotCommandRegister.RegisterDotCommand(bytes, [], 1, 1, new() { Copy = (_, _, _) => copies++ })); Assert.Equal(0, copies); Assert.Equal(before, bytes);
        }
        byte[] target = new byte[31]; target.AsSpan().Fill(0xa5); RawDotCommandRegister.CopyDotCommandName(target, [(byte)'A', 0], 31); Assert.Equal((byte)'A', target[0]); Assert.All(target[1..], value => Assert.Equal(0, value));
    }
    /// <summary>최초 say/count1의 전체2568BYTE가 원본 정적 이미지와 같고 장기 검색 값도 유지된다.</summary>
    [Fact]
    public void StaticOwnerMatchesNativeImageAndKeepsDispatchStorage()
    {
        var owner = new ClientDotCommandGlobals(); Assert.Equal(File.ReadAllText(PathOf("dot-command-static-image.hex")).Trim(), Hex(owner.Bytes)); Assert.Equal(0u, owner.DispatchState.FallbackIndex); Assert.Equal(-1, owner.DispatchState.SeparatorOffset);
        byte[] bytes = owner.Bytes; owner.DispatchState.FallbackIndex = 7; owner.DispatchState.SeparatorOffset = 19; owner.Register([(byte)'X', 0], 0x1234, 1); Assert.Same(bytes, owner.Bytes); Assert.Equal(2u, Count(bytes)); Assert.Equal(7u, owner.DispatchState.FallbackIndex); Assert.Equal(19, owner.DispatchState.SeparatorOffset);
    }
    /// <summary>원문122입력의 NULL/32비트/인용/토큰 한도와 중복 등록을 대조한다.</summary>
    [Fact]
    public void ReplaysNativeIssueCommands()
    {
        int rows = 0;
        // 고정된 원문 CPU 출력에서만 기대 반환/효과를 읽는다.
        foreach (string line in File.ReadLines(PathOf("issue-commands-x86.tsv")))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] values = line.Split((char[]?)null, StringSplitOptions.RemoveEmptyEntries); Assert.Equal(6, values.Length); var events = new List<string>(); uint cheat = Number(values[2]);
            string input = values[1] is "-" or "~" ? "" : Encoding.Latin1.GetString(Convert.FromHexString(values[1]));
            if (values[0] == "Init") RawIssueCommands.RegisterIssueCommands((name, callback, flags) => events.Add($"R,{Hex(Encoding.Latin1.GetBytes(name))},{callback},{flags}"));
            else
            {
                uint? returned = RawIssueCommands.InvokeIssueCommand(values[0] == "Flash" ? 0x4d5300u : 0x4d5360u, values[1] == "-" ? null : input, ref cheat,
                    new() { Flash = value => events.Add("F," + unchecked((uint)value)), Sound = name => events.Add("S," + Hex(Encoding.Latin1.GetBytes(name)) + ",0,0,0,0,0") });
                Assert.Equal(Number(values[3]), returned);
            }
            Assert.Equal(Number(values[4]), cheat); Assert.Equal(values[5], events.Count == 0 ? "-" : string.Join(';', events)); rows++;
        }
        Assert.Equal(122, rows);
    }
    /// <summary>선택 전 미지 callback·flash 뒤 음향 누락·desc 인자를 실제 경계에서 처리한다.</summary>
    [Fact]
    public void IssueSelectionKeepsSuccessfulPrefixAndUnknownResult()
    {
        uint cheat = 17; Assert.Null(RawIssueCommands.InvokeIssueCommand(0, null, ref cheat, new())); int flashes = 0;
        Assert.Throws<InvalidOperationException>(() => RawIssueCommands.InvokeIssueCommand(0x4d5300, "7", ref cheat, new() { Flash = value => { Assert.Equal(7, value); flashes++; } })); Assert.Equal(1, flashes); Assert.Equal(17u, cheat);
        Assert.Equal(0u, RawIssueCommands.InvokeIssueCommand(0x4d5340, null, ref cheat, new() { Description = (owner, argument, flags) => { Assert.Equal(-1, owner); Assert.Null(argument); Assert.Equal(0u, flags); } })); Assert.Throws<ArgumentException>(() => RawIssueCommands.InvokeIssueCommand(0x4d5360, null, ref cheat, new()));
    }
    /// <summary>첫 메뉴 전과 같은 실제 장기 소유자가 Issue4→Chat3을 등록하고 현재 signed 키/표시 바이트를 읽는다.</summary>
    [Fact]
    public void ActualStartupSharesTableManagerSubscribersAndConfiguration()
    {
        var subscribers = new ClientSubscriberGlobals(); var game = new ClientGameplayGlobals(subscribers); var manager = new ClientChatManagerGlobals(); var commands = new ClientDotCommandGlobals(); var config = new ConfigInterface(_ => null);
        config.Set("-1chatFormat", "AB\0ignored"); config.Set("-1chatFormat2", "~c한글"); var failures = new List<uint>();
        ClientChatRuntime.InitializeClientIssueCommands(commands); Assert.Equal(5u, Count(commands.Bytes));
        ClientChatRuntime.InitializeClientChatManager(manager, commands, subscribers, game.CoreData.ActiveBox, config, failures.Add); Assert.Equal(8u, Count(commands.Bytes)); Assert.Empty(failures);
        uint token = manager.Current.Value; byte[] actual = manager.Bytes(token); Assert.Equal((byte)'A', actual[0x14]); Assert.Equal((byte)'B', actual[0x15]); Assert.Equal(0, actual[0x16]); Assert.Equal(0, actual[0x4f]); byte[] back = ExtendedText.EncodeDisplayText("~c한글"); Assert.Equal(back, actual.AsSpan(0x50, back.Length).ToArray()); Assert.Equal(0, actual[0x243]);
        uint[] expected = [0x42f1e0, 0x4d5300, 0x4d5300, 0x4d5340, 0x4d5360, 0x42ddf0, 0x42ddf0, 0x42def0];
        // 같은 인접 callback/표시 배열의 실제 원문 순서를 확인한다.
        for (int index = 0; index < expected.Length; index++) Assert.Equal(expected[index], BinaryPrimitives.ReadUInt32LittleEndian(commands.Bytes.AsSpan(0x800 + index * 4, 4)));
        Assert.Equal(RawDotCommandRegister.EmptyToken, BinaryPrimitives.ReadUInt32LittleEndian(commands.Bytes.AsSpan(0x904, 4))); Assert.Equal(RawDotCommandRegister.EmptyToken, BinaryPrimitives.ReadUInt32LittleEndian(commands.Bytes.AsSpan(0x908, 4)));
        subscribers.MySubscriber.Value = 9; config.Set("9chatFormat", "Next"); ClientChatRuntime.InitializeClientChatManager(manager, commands, subscribers, game.CoreData.ActiveBox, config, failures.Add); Assert.NotEqual(token, manager.Current.Value); Assert.Same(actual, manager.Bytes(token)); Assert.Equal(11u, Count(commands.Bytes)); Assert.Equal((byte)'N', manager.Bytes(manager.Current.Value)[0x14]); Assert.Equal(1u, game.World.LoginSubscriber);
    }
}
