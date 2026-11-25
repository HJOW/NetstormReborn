using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Client.Tests;

// Native 큰 메뉴 부모(State3)와 Source Editor/LAN/Meta 재인계를 대조한다.
// 2026-11-26 1.058에서 SL-5용으로 추가했다. 근거: cpppj BattleLeaveTests의 parent 3행이다.
public sealed class NativeMainMenuStateTests
{
    /// <summary>다섯144BYTE 로컬 상태를 만든다.</summary>
    /// <returns>빈 상태 저장소다.</returns>
    private static LocalMissionStateStorage Blocks() => new()
    {
        Old = new byte[144],
        Phase = new byte[144],
        Immediate = new byte[144],
        Current = new byte[144],
        Next = new byte[144],
    };

    /// <summary>DWORD 블록의 원문 little endian 값을 읽는다.</summary>
    private static uint Word(ReadOnlyMemory<byte> block, int index) => BinaryPrimitives.ReadUInt32LittleEndian(block.Span.Slice(index * 4, 4));

    /// <summary>DWORD 블록의 원문 little endian 값을 쓴다.</summary>
    private static void Word(Memory<byte> block, int index, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(block.Span.Slice(index * 4, 4), value);

    /// <summary>기록 갈고리와 전역을 묶는다.</summary>
    private static (NativeMainMenuStateGlobals Globals, NativeMainMenuStateHooks Hooks, List<string> Calls, List<string> Tells, List<string> Musics, Func<string> Sent) Scene(
        uint phase, bool battle, Func<string, int>? config = null, uint outcome = 0, uint firstMenu = 0, bool editor = false)
    {
        var state = Blocks();
        Word(state.Current, 0, 3); Word(state.Phase, 0, phase);
        var source = new UIntBox(); var result = new UIntBox { Value = outcome }; var first = new UIntBox { Value = firstMenu };
        var calls = new List<string>(); var tells = new List<string>(); var musics = new List<string>(); string sent = "";
        var globals = new NativeMainMenuStateGlobals(state, source, result, first, new char[80], new byte[256], [])
        {
            Battle = battle,
            Editor = editor,
            UserUpdated = () => 1u,
            TellTips = () => 1u,
        };
        var hooks = new NativeMainMenuStateHooks
        {
            ConfigInt = config ?? (_ => 1),
            Request = code => calls.Add("request:" + code),
            DeleteSavedState = () => calls.Add("delete"),
            RequestMusic = name => musics.Add(name),
            SourceBackground = _ => null,
            LeaveBattle = () => sent = "9:" + Convert.ToHexString(RawBattleLeave.MakePacket()).ToLowerInvariant(),
            Prefix = () => calls.Add("prefix"),
            HudPrefix = () => calls.Add("hud"),
            Title = () => calls.Add("title"),
            CloseGuide = () => calls.Add("close"),
            Tell = tells.Add,
            Startup = () => 0u,
            EditorPlayers = () => calls.Add("editor"),
            Tail = () => calls.Add("tail"),
            ChangedState = () => throw new InvalidOperationException("예상 밖 변경"),
        };
        return (globals, hooks, calls, tells, musics, () => sent);
    }

    /// <summary>원문 parent 3행을 실제 큰 메뉴 부모로 실행한다.</summary>
    [Fact]
    public void ParentThreeRowsCommitThroughRealParent()
    {
        string[][] rows = X86Fixture.Read("battle-leave"); string[][] parents = rows.Where(row => row[0] == "parent").ToArray();
        Assert.Equal(3, parents.Length);
        foreach (string[] row in parents)
        {
            uint phase = row[1] == "battle-fourteen" ? 14u : row[1] == "battle-other-phase" ? 12u : 4u;
            bool battle = row[1] != "default";
            var (globals, hooks, calls, tells, musics, sent) = Scene(phase, battle);
            Assert.Equal(LocalMissionParentResult.Commit, NativeMainMenuState.InstallNativeMainMenuState(globals, hooks));
            Assert.Contains("prefix", calls);
            Assert.Equal(row[2], sent());
            // outcome0·source0이면 음악 없이 제목 배경에 머물고 Blank만 알린다.
            Assert.Empty(musics);
            Assert.Equal("titleMenu", new string(globals.Background.Slice(0, 9).ToArray()));
            Assert.Equal(["Blank"], tells);
            Assert.Equal(0u, globals.Outcome.Value);
        }
    }

    /// <summary>intro0은 immediate39를 예약하고 공통 선택으로 복귀한다.</summary>
    [Fact]
    public void IntroZeroReservesImmediate39()
    {
        var (globals, hooks, calls, tells, musics, sent) = Scene(4, false, _ => 0);
        Assert.Equal(LocalMissionParentResult.Commit, NativeMainMenuState.InstallNativeMainMenuState(globals, hooks));
        Assert.Equal(["request:39"], calls);
        Assert.Equal(1u, Word(globals.State.Immediate, 35));
        Assert.Empty(tells);
        Assert.Empty(musics);
        Assert.Equal("", sent());
    }

    /// <summary>이전phase<=6·11이면 출처를 0으로 쓰고 현재 출처를 보관한다.</summary>
    [Fact]
    public void EntrySourceKeepsOrClearsSource()
    {
        var box = new UIntBox { Value = 7 };
        Assert.Equal(0u, NativeMainMenuState.ReadNativeFortEntrySource(6, box));
        box.Value = 7; Assert.Equal(7u, NativeMainMenuState.ReadNativeFortEntrySource(7, box));
        box.Value = 7; Assert.Equal(0u, NativeMainMenuState.ReadNativeFortEntrySource(11, box));
        box.Value = 7; Assert.Equal(7u, NativeMainMenuState.ReadNativeFortEntrySource(12, box));
        box.Value = 7; Assert.Equal(0u, NativeMainMenuState.ReadNativeFortEntrySource(0xffffffffu, box));
    }

    /// <summary>Editor 앞단은 current1/2·배경 계약을 지키고 출처·통지를 잇는다.</summary>
    [Fact]
    public void EditorEntryRequiresCurrentOneOrTwo()
    {
        var state = Blocks();
        Word(state.Current, 0, 1); Word(state.Phase, 0, 5);
        var source = new UIntBox { Value = 9 };
        char[] background = ['x'];
        bool left = false;
        NativeMainMenuState.PrepareNativeEditorEntry(state, source, background, false, () => left = true);
        Assert.Equal(0u, source.Value);
        Assert.Equal('\0', background[0]);
        Assert.False(left);
        Word(state.Current, 0, 3);
        Assert.Throws<InvalidOperationException>(() => NativeMainMenuState.PrepareNativeEditorEntry(state, source, background, false, null));
        Word(state.Current, 0, 2);
        Assert.Throws<ArgumentOutOfRangeException>(() => NativeMainMenuState.PrepareNativeEditorEntry(state, source, [], false, null));
        // battle·이전phase14이면 통지를 호출하며 없으면 거부된다.
        Word(state.Phase, 0, 14);
        Assert.Throws<InvalidOperationException>(() => NativeMainMenuState.PrepareNativeEditorEntry(state, source, background, true, null));
        NativeMainMenuState.PrepareNativeEditorEntry(state, source, background, true, () => left = true);
        Assert.True(left);
    }

    /// <summary>승패·출처 갈래가 삭제·음악·배경을 순서대로 선택한다.</summary>
    [Fact]
    public void OutcomeAndSourceBranchesSelectDeleteMusicAndBackground()
    {
        // phase 7은 출처를 유지하므로 승패 갈래와 출처 갈래가 둘 다 돈다.
        var (globals, hooks, calls, _, musics, _) = Scene(7, false, outcome: 1);
        globals.SourceType.Value = 5;
        int backgrounds = 0;
        hooks.SourceBackground = _ => { backgrounds++; return "custom"; };
        Assert.Equal(LocalMissionParentResult.Commit, NativeMainMenuState.InstallNativeMainMenuState(globals, hooks));
        Assert.Equal(new[] { "delete", "delete" }, calls.Where(item => item == "delete").ToArray());
        Assert.Equal(new[] { "fanfare.mus", "fanfare.mus" }, musics.ToArray());
        Assert.Equal("custom", new string(globals.Background.Slice(0, 6).ToArray()));
        Assert.Equal(0u, globals.Outcome.Value);
        Assert.Equal(1, backgrounds);
    }
}
