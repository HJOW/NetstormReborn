using System.Globalization;
using System.Text;
using Netstorm.Client;
using Netstorm.Client.Chat;
using Netstorm.Client.Gump;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 장기 창 상태와 실제 편집/공유 로그/서버 설정이 같은 저장소를 쓰는지 확인한다.
namespace Netstorm.Client.Tests;

/// <summary>창 없이 실제 공통 문자·로그 본체와 원문189관측을 대조한다.</summary>
public sealed class ClientChatWindowRuntimeTests
{
    /// <summary>원문 빈 표시와 BYTE 입력을 손실 없이 복원한다.</summary>
    private static byte[] Bytes(string text) => text == "-" ? [] : Convert.FromHexString(text);
    /// <summary>원문 BYTE를 소문자hex로 표시하며 빈 문자열은 '-'다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => bytes.IsEmpty ? "-" : Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>원문 trace의 좁은 문자열 표현이다.</summary>
    private static string Hex(string text) => Hex(Encoding.Latin1.GetBytes(text));
    /// <summary>물리 상태의 끝0만 줄이고 내부NUL은 보존한다.</summary>
    private static string State(byte[] bytes)
    {
        int end = bytes.Length;
        // 원문 직렬화와 같은 BSS 끝0 축약이다.
        while (end > 0 && bytes[end - 1] == 0) end--;
        return Hex(bytes.AsSpan(0, end));
    }
    /// <summary>실제 현재100행을 사용하는 독립 원문189행의 연속 BSS와 trace를 대조한다.</summary>
    [Fact]
    public void ReplaysNativeCompletion189WithPersistentClientState()
    {
        var globals = new ClientChatWindowGlobals(); var chat = new ClientChatGlobals(OriginalEdition.Patch1078, () => 100); int rows = 0;
        // 일반 헤더 뒤의 각 원문 입력만 적용하며 기대상태는 원문에서 읽는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "chat-completion-x86.tsv")).Skip(1))
        {
            string[] row = line.Split('\t'); Assert.Equal(11, row.Length); if (row[1] == "1") globals.Completion.Bytes.AsSpan().Clear(); chat.Records.AsSpan().Clear(); Bytes(row[2]).CopyTo(chat.Records, 0); int mode = int.Parse(row[3], CultureInfo.InvariantCulture); var trace = new List<string>();
            var runtime = new ClientChatWindowRuntime(globals, chat, key => { trace.Add("T:" + (key.Length == 0 ? "" : Hex(key))); if (mode == 1) return key switch { "BATTLE" => "Area", "EVERYONE" => "Tous", "ALLIES" => "Friends", _ => "Eyes" }; if (mode == 2) return "dup"; if (mode == 3 && key == "ALLIES") return ""; if ((mode == 4 || mode == 5) && key == "BATTLE") return new string('Z', mode == 4 ? 127 : 128); return key; }, number => trace.Add("A:" + number));
            string? result = runtime.Complete(0x42e540, row[4] == "~" ? null : Encoding.Latin1.GetString(Bytes(row[4])), int.Parse(row[5], CultureInfo.InvariantCulture), row[6] == "1");
            Assert.Equal(row[7], result is null ? "~" : Hex(result)); Assert.Equal(row[8], State(globals.Completion.Bytes)); Assert.Equal(row[9], trace.Count == 0 ? "-" : string.Join(';', trace)); Assert.True(int.Parse(row[10], CultureInfo.InvariantCulture) > 0); rows++;
        }
        Assert.Equal(189, rows);
    }
    /// <summary>원문 BSS/제한 초기값과 안정된 문자열이 창 교체/입력 고리에 의존하지 않는다.</summary>
    [Fact]
    public void GlobalsShareCurrentStorageAndStablePayloads()
    {
        var globals = new ClientChatWindowGlobals(); Assert.Same(globals.Current, globals.Window.Current); Assert.Equal(0u, globals.Current.Value); Assert.Equal(0u, globals.Window.Registered); Assert.Equal(0, globals.Window.CompletionReady); Assert.False(globals.Log.Initialized); Assert.Equal((60, 3), (globals.AllianceRequests.WindowSeconds, globals.AllianceRequests.Limit)); Assert.Equal(128, globals.NewsKey.Length); Assert.All(globals.Completion.Bytes, value => Assert.Equal(0, value));
        uint first = globals.Store("~[C.0]");
        // 후보가 입력80칸보다 많아져도 이전 토큰은 보존한다.
        for (int index = 0; index < 600; index++) globals.Store("payload:" + index);
        Assert.Equal(0x71000000u, first); Assert.Equal(first, globals.Store("~[C.0]")); Assert.Equal("~[C.0]", globals.Strings[first]); globals.Current.Value = 91; Assert.Equal(91u, globals.Window.Current.Value); globals.Window.Current.Value = 0; globals.ReceivedMask.Value = 128; globals.Ignored.Add(200); Assert.Equal(128u, globals.ReceivedMask.Value); Assert.Equal(new uint[] { 200 }, globals.Ignored);
        var configuration = new ConfigInterface(_ => null); globals.InputLineOff.Value = 3; globals.ReadOptions(configuration); Assert.Equal(3, globals.InputLineOff.Value); configuration.SetInt("inputLineOff", -1); globals.ReadOptions(configuration); Assert.Equal(-1, globals.InputLineOff.Value);
    }
    /// <summary>이미 생성한 실제 Console에 늦게 연결해 Tab/Shift와 창 교체 후 순환을 검증한다.</summary>
    [Fact]
    public void ActualConsoleUsesLateBindingAndPersistentCompletion()
    {
        var globals = new ClientChatWindowGlobals(); var chat = new ClientChatGlobals(OriginalEdition.Patch1078, () => 100); var runtime = new ClientChatWindowRuntime(globals, chat, key => key); var scene = new TextEntryWidgetScene(); scene.Construct(); scene.Node.Autocomplete = 0x42e540; scene.Hooks.Complete = runtime.CompleteEntry; scene.Handle(9); Assert.Equal("BATTLE", Encoding.Latin1.GetString(scene.Buffer.Text())); scene.Handle(9); Assert.Equal("EVERYONE", Encoding.Latin1.GetString(scene.Buffer.Text())); scene.Handle(0x20000009); Assert.Equal("BATTLE", Encoding.Latin1.GetString(scene.Buffer.Text()));
        globals.Current.Value = 0; var replacement = new ClientChatWindowRuntime(globals, chat); Assert.Equal("OBSERVER", replacement.Complete(0x42e540, null, 999, true)); Assert.Throws<InvalidOperationException>(() => replacement.Complete(7, null, 0, false));
        chat.Channels.Add(200, "Peer", 0); Assert.Equal("Peer", runtime.Complete(0x42e540, "P", 1, false)); chat.Channels.ResetOccupied(); Assert.Equal("Peer", replacement.Complete(0x42e540, null, 0, false)); Assert.Null(runtime.Complete(0x42e540, "P", 1, false));
    }
    /// <summary>실제 공유128줄의 조각/상태를 빌리고 다른 owner의 이중 대여/반납을 거부한다.</summary>
    [Fact]
    public void SharedLogBorrowsSamePhysicalStateAndReleasesAfterOwner()
    {
        var globals = new ClientChatWindowGlobals(); var chat = new ClientChatGlobals(OriginalEdition.Patch1078, () => 100); var runtime = new ClientChatWindowRuntime(globals, chat); var scene = new StyleBufferScene(); var buffers = new StyleTextBufferStore(scene.Fragments, scene.Style, scene.BufferHooks, _ => { }); object owner = new(), other = new(); runtime.BindSharedLog(owner, buffers); Assert.Same(globals.Log, buffers.Binding(0x548758).State); Assert.Equal(128, globals.Log.MaxLines); runtime.BindSharedLog(owner, buffers); Assert.Throws<InvalidOperationException>(() => runtime.BindSharedLog(other, buffers)); buffers.Binding(0x548758).Handler.AppendText("A\n\0"u8, scene.Context); globals.Current.Value = 0; Assert.Equal(1, globals.Log.LineCount); runtime.ReleaseSharedLog(other, buffers); Assert.True(globals.Log.Initialized); runtime.ReleaseSharedLog(owner, buffers); Assert.False(globals.Log.Initialized); Assert.Null(globals.UiOwner); Assert.Throws<InvalidOperationException>(() => buffers.Binding(0x548758)); Assert.Empty(scene.Fragments.Fragments);
    }
    /// <summary>파일0/무효 관문과 valid callback 뒤 현재 파일 재조회·NUL 경계를 보존한다.</summary>
    [Fact]
    public void LogCallbackKeepsLazyFileGateAndLateCurrentToken()
    {
        var globals = new ClientChatWindowGlobals(); var runtime = new ClientChatWindowRuntime(globals, new(OriginalEdition.Patch1078, () => 100)); runtime.InstallLogCallback(null, null); globals.Log.OnAppend!([65]); globals.LogFile.Value = 9; Assert.Throws<InvalidOperationException>(() => globals.Log.OnAppend!([65, 0])); int validCalls = 0; uint written = 0; byte[] output = []; runtime.InstallLogCallback(file => { validCalls++; Assert.Equal(9u, file); globals.LogFile.Value = 11; return true; }, (file, bytes) => { written = file; output = bytes.ToArray(); }); globals.Log.OnAppend!([65, 0, 66]); Assert.Equal(1, validCalls); Assert.Equal(11u, written); Assert.Equal(new byte[] { 65 }, output);
        runtime.InstallLogCallback(_ => false, (_, _) => throw new InvalidOperationException()); globals.Log.OnAppend!([65]); runtime.InstallLogCallback(_ => true, (_, _) => { }); Assert.Throws<InvalidOperationException>(() => globals.Log.OnAppend!([65]));
    }
    /// <summary>키의 점/NUL/꼬리·물리 경계와 설정 미연결 전 키 작성 순서를 확인한다.</summary>
    [Fact]
    public void NewsKeyPreservesTailAndPrecedesSelectedRead()
    {
        byte[] key = new byte[128]; key.AsSpan().Fill(0xa5); var value = new UIntBox { Value = 7 }; var globals = new SystemNewsTimeGlobals(value, key); var hooks = new SystemNewsTimeHooks { ServerName = () => "A.B-C\0ignored" }; Assert.Equal("A_B-CNewsTime", SystemNewsTime.BuildKey(globals, hooks)); Assert.Equal(0, key[13]); Assert.Equal(0xa5, key[14]); Assert.Throws<InvalidOperationException>(() => SystemNewsTime.Read(globals, new() { ServerName = () => "New.Name" })); Assert.Equal("New_NameNewsTime", Encoding.Latin1.GetString(key, 0, 16)); Assert.Equal(7u, value.Value); byte[] before = key.ToArray(); Assert.Throws<ArgumentOutOfRangeException>(() => SystemNewsTime.BuildKey(globals, new() { ServerName = () => new string('X', 120) })); Assert.Equal(before, key);
    }
    /// <summary>쓰기값은 서버 getter 전에 캡처하며 signed 비트/문화권을 그대로 보존한다.</summary>
    [Fact]
    public void NewsWriteCapturesValueBeforeServerGetter()
    {
        var value = new UIntBox { Value = uint.MaxValue }; var globals = new SystemNewsTimeGlobals(value, new byte[128]); string expression = ""; CultureInfo prior = CultureInfo.CurrentCulture; var culture = (CultureInfo)CultureInfo.InvariantCulture.Clone(); culture.NumberFormat.NegativeSign = "negative:";
        try { CultureInfo.CurrentCulture = culture; SystemNewsTime.Write(globals, new() { ServerName = () => { value.Value = 42; return "A.B"; }, Configure = text => expression = text }); }
        finally { CultureInfo.CurrentCulture = prior; }
        Assert.Equal("A_BNewsTime=\"-1\"", expression); Assert.Equal(42u, value.Value); SystemNewsTime.Read(globals, new() { ServerName = () => "A.B", Read = text => { Assert.Equal("A_BNewsTime", text); return int.MinValue; } }); Assert.Equal(0x80000000u, value.Value);
    }
    /// <summary>실제 서버/UTF8 설정/표시 BYTE 변환을 같은 뉴스 DWORD와 변경 알림에 연결한다.</summary>
    [Fact]
    public void ActualServerAndUtf8ConfigurationShareNewsTime()
    {
        var globals = new ClientChatWindowGlobals(); var runtime = new ClientChatWindowRuntime(globals, new(OriginalEdition.Patch1078, () => 100), _ => Encoding.Latin1.GetString(ExtendedText.EncodeDisplayText("한국.Server"))); var servers = new ClientNsInfoGlobals(); servers.ServerSelection.Value = -1; var configuration = new ConfigInterface(_ => null); configuration.SetInt("한국_ServerNewsTime", -7); runtime.LoadNewsTime(servers, configuration); Assert.Equal(unchecked((uint)-7), globals.NewsTime.Value); globals.NewsTime.Value = 0x80000000; runtime.SaveNewsTime(servers, configuration); Assert.Equal(int.MinValue, configuration.GetInt("한국_ServerNewsTime")); Assert.Equal("한국_ServerNewsTime", ExtendedText.DecodeDisplayText(globals.NewsKey.AsSpan(0, Array.IndexOf(globals.NewsKey, (byte)0))));
    }
}
