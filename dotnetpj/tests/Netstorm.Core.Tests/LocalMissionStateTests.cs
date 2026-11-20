using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 실제 상태 저장소와 독립 native 기록을 사용해 원문 복사/확정과 비동기 경계를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>상태7~10/13/14·Poll/Request의 전체 바이트와 재진입 순서를 대조한다.</summary>
public sealed class LocalMissionStateTests
{
    /// <summary>원본 관찰기의 opaque Zone 핸들이며 호스트 포인터로 해석하지 않는다.</summary>
    private const uint Zone = 0x12500000;
    /// <summary>실제 지정 DWORD를 읽는다.</summary>
    private static uint Word(ReadOnlySpan<byte> bytes, int index) => BinaryPrimitives.ReadUInt32LittleEndian(bytes[(index * 4)..]);
    /// <summary>지정한 DWORD만 실제 입력 저장소에 쓴다.</summary>
    private static void Word(Span<byte> bytes, int index, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[(index * 4)..], value);
    /// <summary>모든 바이트를 fixture의 소문자16진수로 표현한다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>실제 double의64비트를 보존한다.</summary>
    private static string Double(double value) { byte[] bytes = new byte[8]; BinaryPrimitives.WriteUInt64LittleEndian(bytes, BitConverter.DoubleToUInt64Bits(value)); return Hex(bytes); }

    /// <summary>기대 결과를 공급하지 않고 원본 입력/현재 전역/외부 효과만 제공하는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>old/phase/immediate/current/next의 서로 다른 실제720바이트다.</summary>
        public byte[][] Blocks { get; } = Enumerable.Range(0, 5).Select(_ => new byte[144]).ToArray();
        /// <summary>현재 실제 모드이며 callback에서 바꿀 수 있다.</summary>
        public bool Server, Boss = true, Battle, Post = true, Saved = true, Mission = true, Simul, Meta;
        /// <summary>같은 두owner와 known의 실제 부모다.</summary>
        public PlayerWorldState Player { get; } = new();
        /// <summary>독립 원본 DWORD 전역들이다.</summary>
        public UIntBox Pending { get; } = new();
        /// <summary>정리 전 보존할 현재 게임 틱이다.</summary>
        public UIntBox Ticks { get; } = new() { Value = 42 };
        /// <summary>현재 시작 출처다.</summary>
        public UIntBox Source { get; } = new() { Value = 17 };
        /// <summary>실제 CD 확인 결과 저장소다.</summary>
        public UIntBox Cd { get; } = new() { Value = 88 };
        /// <summary>Poll이 현재 재조회하는 메타 갱신 표식이다.</summary>
        public UIntBox Refresh { get; } = new();
        /// <summary>Poll이 현재 재조회하는 메타 재구축 표식이다.</summary>
        public UIntBox Rebuild { get; } = new();
        /// <summary>같은 제목 문자열의 첫BYTE다.</summary>
        public byte[] Title { get; } = [90];
        /// <summary>원본의 독립 연결 기한이다.</summary>
        public DoubleBox End { get; } = new() { Value = 120 };
        /// <summary>원본의 독립 주변음 기한이다.</summary>
        public DoubleBox Activity { get; } = new() { Value = 77 };
        /// <summary>현재 게임 시각과 벽시각의 독립 입력이다.</summary>
        public double Seconds = 45.75, Wall = 100.5;
        /// <summary>세 실제 Template Core의 인접 바이트도 보존한다.</summary>
        public byte[] Cores { get; } = new byte[192];
        /// <summary>현재 실제 연결 주소20바이트다.</summary>
        public byte[] Address { get; } = new byte[20];
        /// <summary>독립 native가 제공한 경로 비트와 외부 변경 종류다.</summary>
        public uint Variant { get; }
        /// <summary>선택된 callback의 명시 변경 종류다.</summary>
        public uint Mutation { get; }
        /// <summary>현재 경계 진입 순서만 관찰한다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>실제 다섯 블록을 빌리는 상태 부모다.</summary>
        public RawLocalMissionState Body { get; set; }
        /// <summary>원본과 동일한 명시 입력을 준비한다. 출력은 fixture에서만 읽는다.</summary>
        public Scene(string kind, uint selected, uint variant, uint mutation)
        {
            Variant = variant; Mutation = mutation;
            // stale DWORD도 실제 서로 다른 입력으로 전체 비교한다.
            for (int block = 0; block < Blocks.Length; block++)
            {
                // 각 블록의 인접 필드가 바뀌지 않았는지 확인할 pattern이다.
                for (int index = 0; index < 36; index++) Word(Blocks[block], index, (uint)(block * 1000 + index));
            }
            byte[] input = Blocks[kind == "Poll" ? 4 : 3]; Word(input, 0, selected); Word(input, 32, Zone); Word(input, 33, (variant & 2) != 0 ? 1u : 0u); Word(input, 34, 1250);
            if (kind == "Poll") Word(Blocks[3], 0, 0);
            if (kind == "Request") Word(Blocks[2], 0, (variant & 1) != 0 ? 7u : 0u);
            Server = (variant & 1) != 0; Pending.Value = (variant & 4) != 0 ? 1u : 0u; Meta = (variant & 256) != 0; Rebuild.Value = Meta ? 1u : 0u;
            Player.PlayerKnown = 9; End.Value = (variant & 1024) != 0 ? 0 : (variant & 32) != 0 ? 90 : 120;
            // native의 실제 Core 입력이며 결과를 미리 계산하지 않는다.
            for (int index = 0; index < Cores.Length; index++) Cores[index] = unchecked((byte)(index * 3 + 5));
            // 사용하지 않는 인접 주소 바이트도 원본 입력을 유지한다.
            for (int index = 0; index < Address.Length; index++) Address[index] = (byte)index;
            BinaryPrimitives.WriteUInt16LittleEndian(Address, (ushort)((variant & 128) != 0 ? 6 : 2)); Address[2] = 0x12; Address[3] = 0x34; Address[12] = 0x56; Address[13] = 0x78;
            Body = Create(Hooks());
        }
        /// <summary>같은 실제 다섯 블록의 순서를 유지한다.</summary>
        public LocalMissionStateStorage Storage() => new() { Old = Blocks[0], Phase = Blocks[1], Immediate = Blocks[2], Current = Blocks[3], Next = Blocks[4] };
        /// <summary>메뉴창도 immediate+84의 같은4바이트를 참조한다.</summary>
        public LocalMissionStateGlobals Globals() => new()
        {
            Server = () => Server, Boss = () => Boss, SetInBattle = value => Battle = value, MissionPost = () => Post, SavedGame = () => Saved, SetSavedGame = value => Saved = value,
            MissionWorld = () => Mission, Simulconnect = () => Simul, Metamap = () => Meta, Player = Player, ZonePending = Pending, GameTicks = Ticks, SourceType = Source, CdPresent = Cd,
            MenuWindow = new(Blocks[2].AsMemory(132, 4)), MetaRefresh = Refresh, MetaRebuild = Rebuild, TitleInitial = Title, BattleByte = () => 7, ConnectEnd = End, ActivityEnd = Activity, GameSeconds = () => Seconds, WallCached = () => Wall,
        };
        /// <summary>같은 상태/전역을 실제 제품 부모에 연결한다.</summary>
        public RawLocalMissionState Create(LocalMissionStateHooks hooks) => new(OriginalEdition.Patch1078, Storage(), Globals(), hooks);
        /// <summary>실제 외부 효과 진입만 기록한다.</summary>
        private void Event(string text) => Trace.Add(text);
        /// <summary>native와 동일한 명시 외부 효과/현재 변경을 제공한다.</summary>
        public LocalMissionStateHooks Hooks() => new()
        {
            LogState = state => { Event($"State,{state}"); if (Mutation == 4) Word(Blocks[3], 0, 8); }, Assertion = () => Event("Assert"), InvalidState = _ => throw new InvalidOperationException("native 선택 밖 상태"),
            SelectDefaultDenominator = () => Event("Selection"),
            CleanupBattle = reason => { Event($"Cleanup,{reason}"); Server = false; Boss = false; Post = true; Pending.Value = 0; Saved = false; Ticks.Value = 0; Source.Value = 0; if (Mutation == 7) Word(Blocks[3], 33, 0); },
            CdPresent = () => { Event("Seed"); return 123; }, RefreshCommands = () => Event("Commands"), StopMusic = () => Event("Music"), RefreshMission = () => Event("Mission"),
            InstallZone = zone => { Event($"Zone,{zone}"); Server = true; Boss = true; if (Mutation == 2) Word(Blocks[3], 32, Zone + 16); }, FreeZone = zone => Event($"Free,{zone}"), InstallMapHandlers = () => Event("MapHandlers"),
            InitializeRenderer = () => Event("Cursor"), UiMode = mode => Event($"UI,{mode}"), WallSeconds = () => { Event("Wall"); if (Mutation == 1) End.Value = 99; return Wall; }, LoadReconnectFort = () => Event("ReconnectFort"),
            Progress = (progress, remaining) => Event($"Progress,{progress},{remaining}"), BattlePalette = () => { Event("Palette"); return 77; }, ApplyPalette = value => Event($"ApplyPalette,{value}"),
            ConnectServer = (bytes, value) => Event($"Connect,{Hex(bytes.Span)},{value}"),
            ConnectionStatus = target => { Event($"Status,{target}"); if (Mutation == 5) Server = true; return target == -1 ? ((Variant & 512) != 0 ? 1 : 0) : (Variant & 2048) != 0 ? -1 : (Variant & 8) != 0 ? 0 : 1; },
            ConnectionOpen = target => { Event($"Open,{target}"); return (Variant & 16) == 0; }, CloseConnection = target => Event($"Close,{target}"), RenameConnection = (from, to) => Event($"Rename,{from},{to}"), LogConnection = (line, value) => Event($"ConnLog,{line},{value}"),
            SendLogin = () => Event("SendLogin"), ConnectedProgress = () => Event("ConnectedProgress"), CloseProgress = () => Event("CloseProgress"), ServerConnection = () => { Event("ServerConnection"); return 55; }, AttachCommander = token => Event($"Commander,{token}"),
            FortAvailable = _ => (Variant & 64) == 0, LoadCurrentFort = () => { Event("FortName"); Event($"LoadFort,{Player.TemplateOwner}"); }, FortCore = owner => { Event($"Core,{owner}"); return Cores.AsMemory(checked((int)owner * 64), 64); },
            LocalAddress = () => { Event("Address"); if (Mutation == 3) Player.TemplateOwner = 2; return Address; }, TouchFort = owner => Event($"Touch,{owner}"), RefreshFort = () => Event("RefreshFort"), SetReconnectSp = money => Event($"Sp,{BitConverter.SingleToUInt32Bits(money)}"),
            LoginTimer = start => Event($"Timer,{(start ? 1 : 0)}"), InputMode = value => Event($"Input,{(value ? 1 : 0)}"), MetaFirst = () => Event("MetaFirst"), MetaSecond = () => Event("MetaSecond"), MetaThird = () => Event("MetaThird"),
        };
        /// <summary>실제720바이트 전체의 fixture 표현이다.</summary>
        public string BlockBytes() => string.Concat(Blocks.Select(bytes => Hex(bytes)));
        /// <summary>같은 현재 전역15개를 독립 기록 순서로 표현한다.</summary>
        public string GlobalBytes() => string.Join(',', new uint[] { Server ? 1u : 0u, Boss ? 1u : 0u, Battle ? 1u : 0u, Post ? 1u : 0u, Saved ? 1u : 0u, Player.PlayerKnown, Player.BattleOwner, Player.TemplateOwner, Pending.Value, Ticks.Value, Source.Value, Cd.Value, Refresh.Value, Rebuild.Value, Title[0] });
    }

    /// <summary>원본33입력의 모든 블록/전역/타이머/Core/경계 순서를 비교한다.</summary>
    [Fact]
    public void OriginalThirtyThreeInputsPreserveEveryByteAndEffect()
    {
        string[][] rows = X86Fixture.Read("local-mission-state"); Assert.Equal(new[] { "input", "blocks", "globals", "timers", "cores", "trace" }, rows[0]); Assert.Equal(34, rows.Length);
        // 정확한 헤더만 건너뛰고 원본 기록33입력을 모두 실행한다.
        foreach (string[] row in rows.Skip(1))
        {
            Assert.Equal(6, row.Length); string[] input = row[0].Split(','); var scene = new Scene(input[0], X86Fixture.UInt(input[1]), X86Fixture.UInt(input[2]), X86Fixture.UInt(input[3]));
            if (input[0] == "Request") scene.Body.Request(X86Fixture.UInt(input[1])); else if (input[0] == "Poll") scene.Body.Poll(); else scene.Body.Run();
            Assert.Equal(row[1], scene.BlockBytes()); Assert.Equal(row[2], scene.GlobalBytes()); Assert.Equal(row[3], Double(scene.End.Value) + Double(scene.Activity.Value)); Assert.Equal(row[4], Hex(scene.Cores)); Assert.Equal(row[5], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
        }
    }

    /// <summary>송신 중 phase는 이전 값이며 실제 수신은 phase10 확정 뒤까지 미룬다.</summary>
    [Fact]
    public void QueuedReplyRunsAfterPhaseTenCommitAndCanScheduleNextReceive()
    {
        var scene = new Scene("Run", 9, 1, 0); bool sent = false, received = false, tail = false;
        scene.Body = scene.Create(scene.Hooks() with { SendLogin = () =>
        {
            Assert.Equal(1000u, Word(scene.Blocks[1], 0)); Assert.Equal(9u, Word(scene.Blocks[3], 0)); sent = true;
            scene.Body.EnqueueReceive(() => { Assert.Equal(10u, Word(scene.Blocks[1], 0)); Assert.Equal(0u, Word(scene.Blocks[3], 0)); received = true; Word(scene.Blocks[3], 0, 13); scene.Body.Run(); scene.Body.EnqueueReceive(() => tail = true); });
            Assert.Throws<InvalidOperationException>(scene.Body.DispatchQueued); Assert.Throws<InvalidOperationException>(scene.Body.Run);
        } });
        scene.Body.Run(); Assert.True(sent); Assert.False(received); Assert.Equal(10u, Word(scene.Blocks[1], 0)); scene.Body.DispatchQueued(); Assert.True(received && tail); Assert.Equal(14u, Word(scene.Blocks[1], 0)); Assert.Equal(0u, Word(scene.Blocks[3], 0));
        byte[] phase = scene.Blocks[1].ToArray(); scene.Body.DispatchQueued(); Assert.Equal(phase, scene.Blocks[1]);
    }

    /// <summary>외부 DWORD의 Value와 기존ref 소비자가 같은 실제 메모리를 즉시 바꾸고 인접 canary를 보존한다.</summary>
    [Fact]
    public void BorrowedWordSharesValueReferenceAndExternalChanges()
    {
        byte[] bytes = Enumerable.Repeat((byte)0xa5, 12).ToArray(); var box = new UIntBox(bytes.AsMemory(4, 4)); box.Value = 0x12345678; Assert.Equal(0x12345678u, Word(bytes, 1)); ref uint value = ref box.Storage; value++; Assert.Equal(0x12345679u, box.Value);
        Word(bytes, 1, 0xfedcba98); Assert.Equal(0xfedcba98u, value); Assert.Equal(Enumerable.Repeat((byte)0xa5, 4), bytes[..4]); Assert.Equal(Enumerable.Repeat((byte)0xa5, 4), bytes[8..]);
        var owned = new UIntBox { Value = 9 }; owned.Storage++; Assert.Equal(10u, owned.Value); Assert.Throws<ArgumentException>(() => new UIntBox(new byte[3]));
    }

    /// <summary>실제 블록 별칭과 필수 효과 누락을 검사하고 실패 전 쓰기를 보존한다.</summary>
    [Fact]
    public void AliasesAndMissingConsumerPreserveEarlierWritesAndAllowRetry()
    {
        var scene = new Scene("Run", 7, 4, 0); byte[] phase = scene.Blocks[1].ToArray(); scene.Body = scene.Create(scene.Hooks() with { CleanupBattle = null }); Assert.Throws<InvalidOperationException>(scene.Body.Run); Assert.Equal(phase, scene.Blocks[1]); Assert.Equal((byte)0, scene.Title[0]); Assert.Equal(7u, Word(scene.Blocks[3], 0));
        Word(scene.Blocks[3], 0, 14); scene.Body.Run(); Assert.Equal(14u, Word(scene.Blocks[1], 0));
        Assert.Throws<ArgumentException>(() => new RawLocalMissionState(OriginalEdition.Cd1072, scene.Storage(), scene.Globals(), scene.Hooks()));
        Assert.Throws<ArgumentException>(() => new RawLocalMissionState(OriginalEdition.Patch1078, scene.Storage() with { Current = scene.Blocks[1] }, scene.Globals(), scene.Hooks()));
        Assert.Throws<ArgumentException>(() => new RawLocalMissionState(OriginalEdition.Patch1078, scene.Storage(), scene.Globals() with { MenuWindow = new() }, scene.Hooks()));
        Assert.Throws<ArgumentException>(() => new RawLocalMissionState(OriginalEdition.Patch1078, scene.Storage() with { Old = new byte[143] }, scene.Globals(), scene.Hooks()));
    }

    /// <summary>메타 callback 뒤 현재 flags/next를 다시 읽고 큰 부모의 조기 반환은 공통 확정을 건너뛴다.</summary>
    [Fact]
    public void MetaEffectsRereadCurrentFlagsAndParentReturnIsPreserved()
    {
        var scene = new Scene("Poll", 0, 256, 0); scene.Body = scene.Create(scene.Hooks() with
        {
            MetaFirst = () => { scene.Trace.Add("first"); scene.Refresh.Value = 3; Word(scene.Blocks[4], 0, 19); }, MetaSecond = () => { scene.Trace.Add("second"); scene.Refresh.Value = 99; }, MetaThird = () => scene.Trace.Add("third"), ParentState = state => { Assert.Equal(19u, state); return LocalMissionParentResult.ReturnWithoutCommit; },
        });
        byte[] phase = scene.Blocks[1].ToArray(); scene.Body.Poll(); Assert.Equal(new[] { "first", "second", "third", "State,19" }, scene.Trace); Assert.Equal(0u, scene.Refresh.Value); Assert.Equal(0u, scene.Rebuild.Value); Assert.Equal(19u, Word(scene.Blocks[3], 0)); Assert.Equal(phase, scene.Blocks[1]);
        scene.Body = scene.Create(scene.Hooks() with { ParentState = _ => LocalMissionParentResult.Commit }); scene.Body.Run(); Assert.Equal(19u, Word(scene.Blocks[1], 0));
    }

    /// <summary>실제 호스트 상태7도 같은 Poll/Run 공통부와 현재 상태 계약을 사용한다.</summary>
    [Fact]
    public void ExternalStartingParentUsesSameCopyCommitAndActualAddressMemory()
    {
        var scene = new Scene("Poll", 7, 4, 0); bool connected = false;
        scene.Body = scene.Create(scene.Hooks() with { ConnectServer = (address, port) =>
        {
            Assert.True(address.Equals((ReadOnlyMemory<byte>)scene.Blocks[3].AsMemory(84, 20))); Word(scene.Blocks[3], 21, 0x87654321); Assert.Equal(0x87654321u, BinaryPrimitives.ReadUInt32LittleEndian(address.Span)); connected = true;
        } });
        scene.Body.PollStarting(() => { Assert.Equal(7u, Word(scene.Blocks[3], 0)); Assert.Equal(0u, Word(scene.Blocks[4], 0)); scene.Body.Request(8); }); Assert.True(connected); Assert.Equal(8u, Word(scene.Blocks[1], 0)); Assert.Equal(9u, Word(scene.Blocks[4], 0));
        Assert.Throws<InvalidOperationException>(() => scene.Body.PollStarting(() => { })); Assert.Throws<InvalidOperationException>(() => scene.Body.RunStarting(() => { }));
    }

    /// <summary>큰 부모가 요청한 상태16과 공통부만 있는 상태도 전체 블록을 확정한다.</summary>
    [Theory]
    [InlineData(4u)]
    [InlineData(12u)]
    [InlineData(14u)]
    [InlineData(16u)]
    public void CommonOnlyStatesCommitAllBytes(uint state)
    {
        var scene = new Scene("Run", state, 0, 0); byte[] expected = scene.Blocks[3].ToArray(); scene.Body.Run(); Assert.Equal(expected, scene.Blocks[1]); Assert.Equal(0u, Word(scene.Blocks[3], 0));
        Word(scene.Blocks[3], 0, 15); scene.Body = scene.Create(scene.Hooks() with { TakeServer = () => scene.Body.Request(16) }); scene.Body.Run(); Assert.Equal(16u, Word(scene.Blocks[1], 0));
    }
    /// <summary>일반 전투의 현재 모드와 동시접속 listener·밀도·두 Chat 채널 순서를 확인한다.</summary>
    [Fact]
    public void GeneralBattleReadsModesAfterCleanupAndUsesActualAmbientDeadline()
    {
        var scene = new Scene("Run", 7, 0, 0) { Simul = true }; LocalMissionStateHooks original = scene.Hooks();
        scene.Body = scene.Create(original with
        {
            CleanupBattle = reason => { original.CleanupBattle!(reason); scene.Post = false; scene.Server = true; },
            ListenServer = () => scene.Trace.Add("Listen"), InitializeDensity = () => scene.Trace.Add("Density"), ClearChat = channel => scene.Trace.Add($"Chat,{channel}"),
        });
        scene.Body.Run(); Assert.True(scene.Battle); Assert.Equal(110.5, scene.Activity.Value); Assert.False(scene.Saved);
        Assert.Equal(new[] { "State,7", "Selection", "Cleanup,1", "Seed", "Commands", "Music", "Listen", "Cursor", "UI,2", "Wall", "Density", "Chat,64", "Chat,128", "Progress,ConnectingStarting,0", "Palette", "ApplyPalette,77", "State,8" }, scene.Trace.Take(17));
        Assert.Equal(8u, Word(scene.Blocks[1], 0)); Assert.Equal(9u, Word(scene.Blocks[4], 0));
    }

}
