using System.Buffers.Binary;
using System.Text;
using Netstorm.Assets;

namespace Netstorm.Client.Mission;

/// <summary>튜토리얼 초기화의 현재 이름과 80바이트 지역 저장소다.</summary>
public sealed class TutorialInitState
{
    public string Name { get; set; } = "";
    public uint NamePrefix { get; set; }
    public byte[] Scratch { get; } = new byte[80];
    public uint Flags { get; set; }
    public uint Demo { get; set; }
}

/// <summary>초기화가 공유하는 현재 전역이다.</summary>
public sealed class TutorialInitGlobals
{
    public uint TutorialNumber { get; set; }
    public uint MissionNumber { get; set; }
    public uint TutorialSeen { get; set; }
    public uint LoginSubscriber { get; set; }
    public uint InitialSubscriber { get; set; }
}

/// <summary>초기화가 순서대로 호출하는 현재 클라이언트의 실제 효과다.</summary>
public sealed class TutorialInitHooks
{
    public Action? ClearTemplates { get; init; }
    public Func<string, string?, string>? Get { get; init; }
    public Action? ResetOptions { get; init; }
    public Action<string, string>? Set { get; init; }
    public Action<string>? RequestFort { get; init; }
    public Action? Header { get; init; }
}

/// <summary>원본 004c3290 튜토리얼 Init의 순서와 현재 상태를 재현한다.</summary>
public static class TutorialInit
{
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("Tutorial Init 실제 효과 누락");
    private static string CString(string value) { int end = value.IndexOf('\0'); return end < 0 ? value : value[..end]; }

    /// <summary>현재 설정과 전역을 갱신하고 요새 요청 후 기술 헤더를 적용한다.</summary>
    public static void Initialize(TutorialInitState state, TutorialInitGlobals globals, TutorialInitHooks hooks)
    {
        Need(hooks.ClearTemplates)();
        globals.TutorialNumber = unchecked((uint)ConfigNumbers.ParseLong(Need(hooks.Get)("tutorialNumber", "0")));
        if (globals.TutorialNumber != 0) globals.TutorialSeen = 1;
        globals.MissionNumber = unchecked((uint)ConfigNumbers.ParseLong(Need(hooks.Get)("missionNumber", "0")));
        string demo = Need(hooks.Get)("demo", null);
        if (CString(demo).Length != 0) state.Demo = unchecked((uint)ConfigNumbers.ParseLong(demo));
        Need(hooks.ResetOptions)();
        string name = CString(state.Name);
        byte[] bytes = Encoding.Latin1.GetBytes(name);
        if (bytes.Length >= state.Scratch.Length) throw new InvalidOperationException("Tutorial Init 이름80BYTE 초과");
        bytes.CopyTo(state.Scratch, 0); state.Scratch[bytes.Length] = 0;
        // C locale에서 ASCII 대문자만 소문자로 바꾸고 첫 글자는 다시 대문자로 만든다.
        for (int i = 0; i < bytes.Length; i++) if (state.Scratch[i] is >= (byte)'A' and <= (byte)'Z') state.Scratch[i] += 32;
        if (bytes.Length != 0 && state.Scratch[0] is >= (byte)'a' and <= (byte)'z') state.Scratch[0] -= 32;
        state.NamePrefix = BinaryPrimitives.ReadUInt32LittleEndian(state.Scratch);
        Need(hooks.Set)("fileName", Encoding.Latin1.GetString(state.Scratch, 0, bytes.Length));
        globals.LoginSubscriber = globals.InitialSubscriber;
        string fort = CString(Need(hooks.Get)("loadFort", CString(state.Name)));
        if (Encoding.Latin1.GetByteCount(fort) >= 256) throw new InvalidOperationException("Tutorial Init loadFort256BYTE 초과");
        Need(hooks.RequestFort)(fort); Need(hooks.Header)(); state.Flags |= 1;
    }
}
