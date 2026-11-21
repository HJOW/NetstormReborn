using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Client.Scripting;

// 원문512입력의 현재 표/가변 입력/공유 검색/외부 효과를 독립 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>고정 문법과 실제 중첩 분배의 장기 상태 검증이다.</summary>
public sealed class DotCommandDispatchTests
{
    /// <summary>격리 원문 장치의 callback 식별자 시작이다.</summary>
    private const uint NativeCallback = 0x20000100;
    /// <summary>원시 입력이나 명시 장치 변경을 같은 네BYTE에 쓴다.</summary>
    private static void Put(byte[] bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at, 4), value);
    /// <summary>부모 계산과 독립된 원문 표 입력이다.</summary>
    private static byte[] Table(uint scene)
    {
        byte[] result = new byte[RawDotCommandRegister.StateBytes]; string[] names = ["say", scene == 3 ? "echo" : "clear", "echo", "front", new('x', 31)];
        // 중복 이름과 실제 주소/함수/count를 명시 위치에 공급한다.
        for (int index = 0; index < names.Length; index++)
        {
            Encoding.Latin1.GetBytes(names[index]).CopyTo(result, index * 32); Put(result, 0x800 + index * 4, NativeCallback + (uint)index * 16); Put(result, 0x900 + index * 4, 0x531b30u + (uint)index * 32);
        }
        Put(result, 0xa04, scene == 1 ? 0 : scene == 2 ? uint.MaxValue : 5); return result;
    }
    /// <summary>첫NUL과32BYTE 보호 꼬리를 가진 실제 가변 입력을 만든다.</summary>
    private static byte[] Input(ReadOnlySpan<byte> text)
    {
        byte[] result = new byte[text.Length + 33]; text.CopyTo(result); result.AsSpan(text.Length + 1).Fill(0xd7); return result;
    }
    /// <summary>원문과 같은 생략 없는 소문자 hex다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>실제 장치 인자의 첫NUL 앞 BYTE만 직렬화한다.</summary>
    private static string Argument(ReadOnlySpan<byte> bytes)
    {
        int end = bytes.IndexOf((byte)0); Assert.True(end >= 0); return Hex(bytes[..end]);
    }
    /// <summary>512관측의 전체 표/보호 꼬리/현재 전역/trace를 빠짐 없이 대조한다.</summary>
    [Fact]
    public void ReplaysIndependentNativeObservations()
    {
        int rows = 0;
        // 원문 입력 열만 장치를 결정하며 기대 결과로 상태를 역산하지 않는다.
        foreach (string line in File.ReadLines(Path.Combine(AppContext.BaseDirectory, "X86", "dot-command-dispatch-x86.tsv")).Skip(1))
        {
            if (line.Length == 0 || line[0] == '#') continue;
            string[] row = line.Split('\t'); Assert.Equal(9, row.Length); uint scene = uint.Parse(row[1], CultureInfo.InvariantCulture), mode = uint.Parse(row[2], CultureInfo.InvariantCulture);
            byte[] table = Table(scene), input = Input(row[0] == "-" ? [] : Convert.FromHexString(row[0])); var state = new DotCommandDispatchState(); int calls = 0; var trace = new List<string>();
            var hooks = new DotCommandDispatchHooks
            {
                Invoke = (token, argument) =>
                {
                    calls++; trace.Add($"C:{unchecked(token - NativeCallback)}:{Argument(argument)}");
                    if (calls == 1)
                    {
                        if (mode == 3) { Encoding.Latin1.GetBytes("changed\0").CopyTo(table, 32); Put(table, 0x808, NativeCallback + 48); }
                        else if (mode == 4) { state.FallbackIndex = 1; Encoding.Latin1.GetBytes("new-default\0").CopyTo(table, 32); }
                        else if (mode == 5 && argument[0] != 0) argument[0] = (byte)'Z';
                        else if (mode == 6 && state.SeparatorOffset != -1) state.SeparatorOffset = 2;
                    }
                    return mode == 2 ? 0x80000001u : mode is 1 or 3 or 4 or 7 ? 0x1234u : 0;
                },
                Error = (name, code) => { trace.Add($"E:{Hex(name)}:{code:x}"); if (mode == 7 && state.SeparatorOffset != -1) state.SeparatorOffset = 2; },
            };
            Assert.Equal(uint.Parse(row[3], CultureInfo.InvariantCulture), RawDotCommandDispatch.DispatchDotCommand(table, state, input, hooks)); Assert.Equal(row[4], Hex(table)); Assert.Equal(row[5], state.FallbackIndex.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[6], state.SeparatorOffset.ToString(CultureInfo.InvariantCulture)); Assert.Equal(row[7], Hex(input)); Assert.Equal(row[8], trace.Count == 0 ? "-" : string.Join(';', trace)); rows++;
        }
        Assert.Equal(512, rows);
    }
    /// <summary>빈 입력은 표/장치를 읽지 않고 실제 도달한 Invoke/Error/물리 경계만 요구한다.</summary>
    [Fact]
    public void LazyDevicesKeepEmptyInputStateAndSelectedFailures()
    {
        var state = new DotCommandDispatchState { FallbackIndex = 123, SeparatorOffset = 77 }; Assert.Equal(1u, RawDotCommandDispatch.DispatchDotCommand([], state, [0], new())); Assert.Equal(123u, state.FallbackIndex); Assert.Equal(77, state.SeparatorOffset);
        byte[] table = Table(0), input = Input(".clear"u8); state = new(); Assert.Throws<InvalidOperationException>(() => RawDotCommandDispatch.DispatchDotCommand(table, state, input, new()));
        Assert.Throws<InvalidOperationException>(() => RawDotCommandDispatch.DispatchDotCommand(table, state, input, new() { Invoke = (_, _) => 1 })); Assert.Equal(1u, RawDotCommandDispatch.DispatchDotCommand(table, state, input, new() { Invoke = (_, _) => 0 }));
        Assert.Throws<ArgumentOutOfRangeException>(() => RawDotCommandDispatch.DispatchDotCommand(table, state, [(byte)'h', (byte)'i'], new()));
    }
    /// <summary>같은 장기 검색 상태의 실제 중첩 호출이 바깥 다음 위치를 바꾼다.</summary>
    [Fact]
    public void ReentrantCallbackUsesCurrentSharedSeparator()
    {
        byte[] table = Table(0), outer = Input(".echo abc;tail"u8), inner = Input(".clear;"u8); var state = new DotCommandDispatchState(); var hooks = new DotCommandDispatchHooks(); var trace = new List<string>();
        hooks.Invoke = (token, argument) => { trace.Add($"{unchecked(token - NativeCallback)}:{Argument(argument)}"); if (token == NativeCallback + 32) Assert.Equal(1u, RawDotCommandDispatch.DispatchDotCommand(table, state, inner, hooks)); return 0; };
        Assert.Equal(1u, RawDotCommandDispatch.DispatchDotCommand(table, state, outer, hooks)); Assert.Equal(new[] { "32:20616263", "16:", "0:6162633b7461696c" }, trace); Assert.Equal(-1, state.SeparatorOffset);
    }
    /// <summary>실제 시작 표의 첫 중복과 현재 fallback/name 재조회·가변 입력 쓰기가 같은 owner에서 보인다.</summary>
    [Fact]
    public void ActualOwnerDispatchUsesSameTableSharedStateAndMutableFallbackInput()
    {
        var owner = new ClientDotCommandGlobals(); ClientChatRuntime.InitializeClientIssueCommands(owner); byte[] table = owner.Bytes; var shared = owner.DispatchState;
        Put(table, 0x804, 0x12345678); int calls = 0;
        Assert.Equal(1u, owner.Dispatch(Input(".FlAsH"u8), new() { Invoke = (token, argument) => { Assert.Equal(0x12345678u, token); Assert.Equal("", Argument(argument)); calls++; return 0; } })); Assert.Equal(1, calls);
        byte[] input = Input("words"u8); var errors = new List<string>();
        Assert.Equal(1u, owner.Dispatch(input, new()
        {
            Invoke = (token, argument) => { Assert.Equal(0x42f1e0u, token); argument[0] = (byte)'Z'; shared.FallbackIndex = 3; Encoding.Latin1.GetBytes("current\0").CopyTo(table, 96); return 0x80000001; },
            Error = (name, result) => errors.Add($"{Encoding.Latin1.GetString(name)}:{result:x}"),
        }));
        Assert.Same(table, owner.Bytes); Assert.Same(shared, owner.DispatchState); Assert.Equal(3u, shared.FallbackIndex); Assert.Equal(-1, shared.SeparatorOffset); Assert.Equal((byte)'Z', input[0]); Assert.Equal(new[] { "current:80000001" }, errors);
    }
    /// <summary>원문 string_view와 같이 오류 callback 안에서 표를 바꾸면 같은 이름 뷰도 즉시 바뀐다.</summary>
    [Fact]
    public void ErrorCallbackBorrowsLiveTableNameInsteadOfStringSnapshot()
    {
        byte[] table = Table(0), input = Input(".clear"u8); int errors = 0;
        Assert.Equal(1u, RawDotCommandDispatch.DispatchDotCommand(table, new(), input, new()
        {
            Invoke = (_, _) => 1,
            Error = (name, result) => { Assert.Equal(1u, result); table[32] = (byte)'C'; Assert.Equal("Clear", Encoding.Latin1.GetString(name)); errors++; },
        }));
        Assert.Equal(1, errors); Assert.Equal((byte)'C', table[32]);
    }
}
