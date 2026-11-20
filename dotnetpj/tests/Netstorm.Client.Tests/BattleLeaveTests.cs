using System.Buffers.Binary;
using Netstorm.Client.Input;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 독립 원문 전투 이탈의 직접 쓰기와 실제 로컬 큐/등록을 대조한다.
namespace Netstorm.Client.Tests;

/// <summary>52h 패킷/단일예약13입력·중단8입력·양수이유22입력과현재경계를검증한다.</summary>
public sealed class BattleLeaveTests
{
    /// <summary>원문 경계 중단의 실제 관측 이름이다.</summary>
    private sealed class Stop(string where) : Exception(where);
    /// <summary>독립 native와 같은 다섯 실제 블록/현재 전역 입력이다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제720바이트와 직접 쓰기 대상이다.</summary>
        public ClientLocalStateGlobals Local { get; } = new(new(), new());
        /// <summary>005c85ac의 별도 결과값이다.</summary>
        public UIntBox Outcome { get; } = new() { Value = 2 };
        /// <summary>현재 직접 읽기 선택 모드다.</summary>
        public bool Post = true, Meta, Battle = true;
        /// <summary>실제 editor DWORD이며 bool 사본으로 바꾸지 않는다.</summary>
        public uint Editor, Known = 1;
        /// <summary>다섯 서로 다른 같은144바이트를 순서대로 빌린다.</summary>
        public Memory<byte>[] Blocks => [Local.State.Old, Local.State.Phase, Local.State.Immediate, Local.State.Current, Local.State.Next];
        /// <summary>관찰기의 원시 input pattern만 공급한다.</summary>
        public Scene(byte pattern = 0xa5)
        {
            // 원문 전체720바이트에 동일한 입력 흔적을 남긴다.
            foreach (Memory<byte> block in Blocks) block.Span.Fill(pattern);
            Local.ScriptBreak.Value = 17; Local.SourceType.Value = 133; Local.AbortPending.Value = 19;
        }
        /// <summary>공통 중단은 현재 모드를 호출 위치마다 다시 읽는다.</summary>
        public BattleAbortStateGlobals Abort() => new() { State = Local.State, AbortPending = Local.AbortPending, Outcome = Outcome, MissionPost = () => Post, Editor = () => Editor, Metamap = () => Meta, Battle = () => Battle };
        /// <summary>양수 부모도 같은 실제 전역/공통 상태를 빌린다.</summary>
        public BattleLeaveStateGlobals Leave() => new() { Abort = Abort(), ScriptBreak = Local.ScriptBreak, SourceType = Local.SourceType, PlayerKnown = () => Known };
        /// <summary>다섯 블록 전체를 원문 최초 관측의 표현으로 읽는다.</summary>
        public string Bytes() => string.Join(':', Blocks.Select(block => Convert.ToHexString(block.Span).ToLowerInvariant()));
        /// <summary>경계 시점의 같은8DWORD를 native 순서로 읽는다.</summary>
        public string Snapshot(string ev) => ev + ',' + string.Join(',', new uint[] { Local.ScriptBreak.Value, Local.SourceType.Value, Local.AbortPending.Value, Outcome.Value, Word(Local.State.Next, 0), Word(Local.State.Current, 33), Word(Local.State.Immediate, 33), Word(Local.State.Next, 33) });
    }
    /// <summary>현재 실제 상태 DWORD를 읽는다.</summary>
    private static uint Word(ReadOnlyMemory<byte> bytes, int index) => BinaryPrimitives.ReadUInt32LittleEndian(bytes.Span[(index * 4)..]);
    /// <summary>실제 분배기와 연결 종료 장치를 제공한다.</summary>
    private static PacketDispatchHooks Hooks() => new() { Now = () => 100, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } };

    /// <summary>원본 헤더3/수신7/next3의 모든 바이트를 비교한다. 큰 메뉴 부모3행은 해당 부모에서 잇는다.</summary>
    [Fact]
    public void OriginalPacketReceiveAndSingleRequestThirteenInputsMatchAllBytes()
    {
        string[][] all = X86Fixture.Read("battle-leave"); Assert.Equal(16, all.Length); string[][] rows = all.Where(row => row[0] != "parent").ToArray(); Assert.Equal(13, rows.Length);
        // 큰 부모를 직접 실행했다고 가장하지 않고 이 모듈의 원문 직접 쓰기13행을 전부 대조한다.
        foreach (string[] row in rows)
        {
            if (row[0] == "header") { byte[] packet = Enumerable.Repeat((byte)X86Fixture.UInt(row[1]), 16).ToArray(); RawBattleLeave.InitializePacket(packet); Assert.Equal(row[2], Convert.ToHexString(packet).ToLowerInvariant()); }
            else if (row[0] == "receive")
            {
                byte[] players = Enumerable.Range(0, 1620).Select(index => unchecked((byte)(index * 19 + 7))).ToArray(); BinaryPrimitives.WriteUInt32LittleEndian(players.AsSpan(260), X86Fixture.UInt(row[1])); RawBattleLeave.Receive(1, players); Assert.Equal(row[2], Convert.ToHexString(players).ToLowerInvariant());
            }
            else { Assert.Equal("next", row[0]); var scene = new Scene((byte)X86Fixture.UInt(row[1])); RawBattleLeave.RequestMainMenu(scene.Local.State); Assert.Equal(row[2], scene.Bytes()); }
        }
    }

    /// <summary>원본 중단8입력에서 전체블록/결과/경계중단의 부분 효과를 대조한다.</summary>
    [Fact]
    public void OriginalAbortEightInputsKeepPartialWritesAtFileBoundaries()
    {
        string[][] rows = X86Fixture.Read("battle-abort-state"); Assert.Equal(8, rows.Length);
        // 원본의 명시 모드/초기값만 준비하고 기대상태는 파일에서 읽는다.
        foreach (string[] row in rows)
        {
            Assert.Equal(9, row.Length); var scene = new Scene((byte)X86Fixture.UInt(row[1])) { Editor = row[0] == "editor" ? 1u : 0u, Battle = row[0] != "editor" }; scene.Local.AbortPending.Value = 0xaabbccdd; scene.Outcome.Value = X86Fixture.UInt(row[2]); string cause = "normal", trace = "";
            try { RawBattleLeave.PrepareAbort(scene.Abort(), () => { trace = "path"; if (row[3] == "path") throw new Stop("path"); trace += ";delete"; if (row[3] == "delete") throw new Stop("delete"); }); } catch (Stop stop) { cause = stop.Message; }
            Assert.Equal(row[4], cause); Assert.Equal(X86Fixture.UInt(row[5]), scene.Local.AbortPending.Value); Assert.Equal(X86Fixture.UInt(row[6]), scene.Outcome.Value); Assert.Equal(row[7], scene.Bytes()); Assert.Equal(row[8], trace);
        }
    }

    /// <summary>양수이유 원본22입력의 두 삭제 사이와 현재 전역8개/720바이트를 대조한다.</summary>
    [Fact]
    public void OriginalPositiveReasonTwentyTwoInputsPreserveBothDeletesAndTrace()
    {
        string[][] rows = X86Fixture.Read("battle-leave-reason"); Assert.Equal(22, rows.Length);
        // 경계에 도달한 시점의 전역을 기록하고 원본 중단 위치만 외부 입력으로 공급한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(10, row.Length); var scene = new Scene { Editor = row[0] == "editor" ? 1u : 0u, Known = row[0] == "mission" ? 1u : 0u, Battle = row[0] == "mission" }; int paths = 0; string cause = "normal"; var trace = new List<string>();
            void Boundary(string name) { trace.Add(scene.Snapshot(name)); if (name == row[2]) throw new Stop(name); }
            try { RawBattleLeave.PrepareLeave(X86Fixture.Int(row[1]), scene.Leave(), () => { paths++; Boundary("path" + paths); Boundary("delete" + paths); }); } catch (Stop stop) { cause = stop.Message; }
            Assert.Equal(row[3], cause); Assert.Equal(X86Fixture.UInt(row[4]), scene.Local.ScriptBreak.Value); Assert.Equal(X86Fixture.UInt(row[5]), scene.Local.SourceType.Value); Assert.Equal(X86Fixture.UInt(row[6]), scene.Local.AbortPending.Value); Assert.Equal(X86Fixture.UInt(row[7]), scene.Outcome.Value); Assert.Equal(row[8], scene.Bytes()); Assert.Equal(row[9], trace.Count == 0 ? "-" : string.Join(';', trace));
        }
    }

    /// <summary>송신/flush는Player를바꾸지않고실제Poll의52h수신만flag4를쓴다.</summary>
    [Fact]
    public void ActualLocalQueueDeliversLeaveOnlyAtPollWithCurrentSender()
    {
        byte[] players = new byte[1620]; uint now = 100; int records = 0; var hooks = Hooks(); hooks.Now = () => ++now; hooks.Record = (node, packet, length) => { Assert.Equal(1u, node.Sender); Assert.Equal((byte)0x52, packet.Span[1]); Assert.Equal(5, length); records++; };
        using var wire = new LocalMissionTransport(players, () => true, hooks); wire.RegisterLogin((_, _, _) => throw new InvalidOperationException("선택되지않는 로그인")); RawBattleLeave.Register(wire.Dispatch, players); wire.Open("127.0.0.1", 6799); wire.Poll(); wire.AssignConnection(-1, 1);
        RawBattleLeave.SendForState((target, packet) => Assert.True(wire.Send(target, packet))); Assert.Equal((byte)0, players[260]); wire.Flush(); Assert.Equal((byte)0, players[260]); Assert.True(wire.Poll() > 0); Assert.Equal((byte)4, players[260]); Assert.Equal(1, records); wire.CloseAll();
    }

    /// <summary>기존고정F는동일객체/metadata를대여하고잘못된metadata는덮어쓰지않는다.</summary>
    [Fact]
    public void RegistrationBorrowsSameCanonicalValueAndChecksMetadata()
    {
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, Hooks()); byte[] players = new byte[1620]; int called = 0; dispatch.RegisterCanonicalDirect(0x52, 0x004a71a0, (sender, _, _) => { called++; RawBattleLeave.Receive(sender, players); }); dispatch.Observe(0x52, _ => { }); dispatch.Version(0x52, 1, 0); CanonicalPacketReceiver receiver = dispatch.Registration(0x52).CanonicalReceive!;
        var local = new ClientLocalStateGlobals(new(), new()); using var lease = dispatch.Registrations.BindSession(local); RawBattleLeave.Register(dispatch, players); Assert.Same(receiver, dispatch.Registration(0x52).CanonicalReceive); dispatch.InvokeDirect(0x52, 8, null, ReadOnlyMemory<byte>.Empty); Assert.Equal(1, called); Assert.Equal((byte)4, players[8 * 180 + 80]); dispatch.RequireLength(0x52, 6); Assert.Throws<InvalidOperationException>(() => RawBattleLeave.Register(dispatch, players)); Assert.Same(receiver, dispatch.Registration(0x52).CanonicalReceive);
    }

    /// <summary>원격하위뒤현재모드를다시읽고선행실패/잘못된이유는원문위치에서중단한다.</summary>
    [Fact]
    public void CurrentModeAndFailurePrefixesAreNotRolledBack()
    {
        var scene = new Scene { Post = false }; int deletes = 0; RawBattleLeave.PrepareLeave(9, scene.Leave(), () => deletes++, () => { Assert.Equal(17u, scene.Local.ScriptBreak.Value); scene.Post = true; }); Assert.Equal(2, deletes); Assert.Equal(0u, scene.Local.SourceType.Value); Assert.Equal(0u, scene.Outcome.Value); Assert.Equal(3u, Word(scene.Local.State.Next, 0));
        scene = new Scene { Post = false }; Assert.Throws<InvalidOperationException>(() => RawBattleLeave.PrepareLeave(1, scene.Leave(), () => { })); Assert.Equal(17u, scene.Local.ScriptBreak.Value); Assert.Equal(19u, scene.Local.AbortPending.Value);
        // signed0/음수는 확인창의영역이며 양수부모가 값을쓰지않는다.
        foreach (int reason in new[] { int.MinValue, -1, 0 }) { Assert.Throws<ArgumentOutOfRangeException>(() => RawBattleLeave.PrepareLeave(reason, scene.Leave(), null)); Assert.Equal(17u, scene.Local.ScriptBreak.Value); }
        Assert.Throws<ArgumentOutOfRangeException>(() => RawBattleLeave.Receive(9, new byte[1620])); Assert.Throws<ArgumentOutOfRangeException>(() => RawBattleLeave.InitializePacket(new byte[3]));
    }

    /// <summary>명령/뗌/signed이유별실제확인창선택을기존입력표현에서검증한다.</summary>
    [Fact]
    public void RetreatConfirmationConsumesReleasesAndSelectsOnlyNonpositiveReason()
    {
        var values = new List<string>(); Assert.False(RawBattleLeave.DispatchRetreatConfirmation(new(1, 0, 0), null)); Assert.True(RawBattleLeave.DispatchRetreatConfirmation(new(RawBattleLeave.LeaveBattleCommand | InputCode.Release, 0, 2), null)); Assert.False(RawBattleLeave.DispatchRetreatConfirmation(new(RawBattleLeave.LeaveBattleCommand, 0, 1), null));
        Assert.True(RawBattleLeave.DispatchRetreatConfirmation(new(RawBattleLeave.LeaveBattleCommand, 0, int.MinValue), values.Add)); Assert.True(RawBattleLeave.DispatchRetreatConfirmation(new(RawBattleLeave.LeaveBattleCommand, 0, 0), values.Add)); Assert.Equal(new[] { "ConfirmRetreat", "ConfirmRetreat" }, values);
    }
    /// <summary>현재일반/메타모드의다음상태선택과원문이쓰지않는결과값을검증한다.</summary>
    [Theory]
    [InlineData(false, true, 0u, 3u, 6u)]
    [InlineData(false, true, 5u, 5u, 6u)]
    [InlineData(false, false, 5u, 5u, 0xa5a5a5a5u)]
    [InlineData(true, true, 5u, 0u, 3u)]
    public void AbortCurrentModesChooseNextAndKeepUntouchedOutcome(bool meta, bool battle, uint before, uint outcome, uint next)
    {
        var scene = new Scene { Post = false, Meta = meta, Battle = battle }; scene.Outcome.Value = before; byte[] old = scene.Local.State.Old.ToArray(), phase = scene.Local.State.Phase.ToArray(); bool deleted = false;
        RawBattleLeave.PrepareAbort(scene.Abort(), () => { Assert.Equal(0u, Word(scene.Local.State.Next, 33)); Assert.Equal(0u, Word(scene.Local.State.Current, 33)); Assert.Equal(0u, Word(scene.Local.State.Immediate, 33)); deleted = true; });
        Assert.True(deleted); Assert.Equal(outcome, scene.Outcome.Value); Assert.Equal(next, Word(scene.Local.State.Next, 0)); Assert.Equal(old, scene.Local.State.Old.ToArray()); Assert.Equal(phase, scene.Local.State.Phase.ToArray()); Assert.Equal(17u, scene.Local.ScriptBreak.Value); Assert.Equal(133u, scene.Local.SourceType.Value);
    }

}
