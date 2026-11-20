using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 타이머/집계/Begin/중단의 독립 원본1984입력과 모든 현재 읽기·외부 효과를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>같은 실제 전역에서 원본 정상 반환과 재진입 뒤 쓰기 순서를 검증한다.</summary>
public sealed class MissionLoginTimerTests
{
    /// <summary>독립 native의 명시 flags16종이다.</summary>
    private static readonly uint[] Flags = [0, 1, 2, 3, 0x40, 0x41, 0x42, 0x43, 0x100, 0x101, 0x102, 0x103, 0x140, 0x142, 0x143, uint.MaxValue];
    /// <summary>native 모드 읽기 관찰의 실제 이름6개다.</summary>
    private static readonly string[] Modes = ["saved", "server", "mission", "post", "geysers", "asserts"];
    /// <summary>같은 물리 DWORD만 명시 입력으로 쓴다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>전체 원문 바이트의 fixture 표현이다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>현재double의8바이트 비트를 보존한다.</summary>
    private static string Double(double value) { byte[] bytes = new byte[8]; BinaryPrimitives.WriteUInt64LittleEndian(bytes, BitConverter.DoubleToUInt64Bits(value)); return Hex(bytes); }

    /// <summary>원본과 동일한 입력·실제 저장소·현재 getter/외부 변경을 제공하는 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>pattern/설정/현재 모드와 callback 변경의 명시14입력이다.</summary>
        public uint[] Numbers { get; }
        /// <summary>로그인과 타이머가 빌리는 같은 Player9 원문이다.</summary>
        public byte[] Players { get; } = new byte[1620];
        /// <summary>현재6모드의 실제 bool 저장소다.</summary>
        public bool[] CurrentModes { get; }
        /// <summary>외부 벽시각의 명시 입력이며 게임 틱으로 바꾸지 않는다.</summary>
        public double Wall { get; }
        /// <summary>현재 실제 타이머 전역이다.</summary>
        public MissionLoginTimerState State { get; } = new() { Timeout = new() { Value = 0x76543210 }, ScriptBreak = new() { Value = 0xabcdef01 }, RequestPhase = new() { Value = 0x11223344 }, Clock = new() { Value = 0x12345678 } };
        /// <summary>외부 명시 변경을 허용할 현재 잔여 횟수다.</summary>
        private uint _budget;
        /// <summary>실제 현재 모드 읽기/효과 진입의 전체 관찰이다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>같은 현재 Player9 집계다.</summary>
        public MissionLoginPlayerCounts Counts { get; }
        /// <summary>현재 모드 조회를 읽을 때마다 원본 trace에 남긴다.</summary>
        public MissionLoginTimerReads Reads { get; }
        /// <summary>명시 외부 효과/변경을 공급하는 실제 장치다.</summary>
        public MissionLoginTimerHooks Hooks { get; }
        /// <summary>원문 비교/분기/쓰기를 수행할 실제 부모다.</summary>
        public RawMissionLoginTimer Timer { get; }
        /// <summary>결과 모델 없이 native와 같은 입력/관찰만 준비한다.</summary>
        public Scene(string input)
        {
            string[] fields = input.Split('|'); Numbers = fields[0].Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(14, Numbers.Length);
            State.End.Value = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(X86Fixture.Bytes(fields[1]))); Wall = BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(X86Fixture.Bytes(fields[2]))); State.Begin.Value = Numbers[6]; _budget = Numbers[13];
            CurrentModes = [Numbers[2] != 0, Numbers[3] != 0, Numbers[4] != 0, Numbers[5] != 0, Numbers[7] != 0, Numbers[8] != 0];
            // 집계가 읽지 않는 stale BYTE도 native와 같은 명시 입력이다.
            for (int index = 0; index < Players.Length; index++) Players[index] = unchecked((byte)(index * 37 + Numbers[0] * 11));
            // Player0의 stale 입력은 실제 집계에서 제외한다.
            for (uint owner = 0; owner < 9; owner++) Player(owner, 0, 0); Player(0, 0xdeadbeef, uint.MaxValue);
            uint pattern = Numbers[0];
            if (pattern >= 16) { uint ordinal = pattern - 16; uint[] subs = [0, 7, uint.MaxValue]; Player(ordinal / 48 + 1, subs[ordinal / 16 % 3], Flags[ordinal % 16]); }
            else if (pattern == 1) Player(1, 7, 3);
            else if (pattern == 2) Player(1, 7, 0x43);
            else if (pattern == 3) { Player(1, 7, 3); Player(2, 8, 3); }
            else if (pattern == 4) { Player(1, 7, 0x43); Player(2, 8, 0x102); }
            else if (pattern == 5) Player(1, 7, 2);
            else if (pattern == 6) Player(1, 7, 0x100);
            else if (pattern is >= 7 and <= 9)
            {
                // 전체8명의 접속/Ready 입력과 subscriber0 stale flags를 구별한다.
                for (uint owner = 1; owner < 9; owner++) Player(owner, pattern == 9 ? 0u : owner, pattern == 7 ? 0u : pattern == 8 ? 0x43u : uint.MaxValue);
            }
            Counts = new(OriginalEdition.Patch1078, Players); Reads = new() { SavedGame = () => Mode(0), Server = () => Mode(1), Mission = () => Mode(2), MissionPost = () => Mode(3), InitialGeysers = () => Mode(4), CheckingAssertions = () => Mode(5) };
            Hooks = new()
            {
                ConfigInt = key => { Assert.Equal("loginTimeout", key); Event("Q"); return unchecked((int)Numbers[1]); }, WallSeconds = () => { Event("W", 10); return Wall; }, Clock = () => { Event("C", 1); return 0x87654321; },
                Connection = peer => { Assert.Equal(9, peer); Event("N,9", 2); return Numbers[9] != 0; }, RemoteStatus = (peer, first, second) => { Assert.Equal(9, peer); Assert.Equal(0u, first); Assert.Equal(0u, second); Event("R,9,0,0", 3); },
                Broadcast = packet => Event("B," + Hex(packet.Span), 4), PriestRefresh = owner => Event("P," + owner, 5), SaveReconnect = () => Event("S", 6), CloseProgress = () => Event("X", 7), ResetCamera = () => Event("V"), AbortBattle = () => Event("A", 11), Progress = () => Event("U"), ServerAssertion = () => Event("J", 9),
                Log = (kind, text) => Event($"L,{(kind == MissionLoginTimerLog.Status ? 1 : 0)},{Hex(System.Text.Encoding.Latin1.GetBytes(text))}", kind == MissionLoginTimerLog.Status ? 8u : 0),
            };
            Timer = new(OriginalEdition.Patch1078, State, Counts, Reads, Hooks);
        }
        /// <summary>현재 실제 기록의 명시 구독자/flags만 쓴다.</summary>
        private void Player(uint owner, uint subscriber, uint flags) { Put(Players, (int)owner * 180 + 0x2c, subscriber); Put(Players, (int)owner * 180 + 0x50, flags); }
        /// <summary>효과 진입 전 같은 실제 전역의 쓰기 순서를 관측한다.</summary>
        private string Snapshot() => $"{Double(State.End.Value)},{State.Begin.Value},{State.ScriptBreak.Value},{State.RequestPhase.Value},{State.Clock.Value}";
        /// <summary>native에 공급한 동일 외부 변경만 실제 효과 진입 뒤에 적용한다.</summary>
        private void Mutate(uint trigger)
        {
            if (trigger != Numbers[10] || _budget == 0) return; _budget--; uint action = Numbers[11], value = Numbers[12];
            if (action == 1) CurrentModes[1] = !CurrentModes[1]; else if (action == 2) CurrentModes[3] = !CurrentModes[3]; else if (action == 3) CurrentModes[0] = !CurrentModes[0]; else if (action == 4) CurrentModes[2] = !CurrentModes[2];
            else if (action == 5) State.Begin.Value = value; else if (action == 6) State.End.Value = value; else if (action == 7) Put(Players, 180 + 0x50, 0x43); else if (action == 8) State.RequestPhase.Value = value; else if (action == 9) State.ScriptBreak.Value = value;
        }
        /// <summary>외부 경계 진입 관찰 뒤 명시 변경을 공급한다.</summary>
        private void Event(string text, uint trigger = 0) { Trace.Add(text + ',' + Snapshot()); Mutate(trigger); }
        /// <summary>각 현재 모드 읽기 자체의 실제 순서를 기록한다.</summary>
        private bool Mode(int index) { Trace.Add($"G,{Modes[index]},{(CurrentModes[index] ? 1 : 0)}"); return CurrentModes[index]; }
        /// <summary>현재 모든 타이머/모드 저장소를 native 순서로 직렬화한다.</summary>
        public string Observe() => $"{Double(State.End.Value)}|{unchecked((uint)State.Timeout.Value)},{State.Begin.Value},{State.ScriptBreak.Value},{State.RequestPhase.Value},{State.Clock.Value}," + string.Join(',', CurrentModes.Select(mode => mode ? 1 : 0));
    }

    /// <summary>원본1984입력의 정상 반환·전체 현재 전역·모든 모드 읽기/외부 효과를 전수 대조한다.</summary>
    [Fact]
    public void OriginalTimer1984()
    {
        string[][] rows = X86Fixture.Read("mission-login-timer").Where(row => row[0] != "kind").ToArray(); Assert.Equal(1984, rows.Length); var seen = new HashSet<string>();
        // 각 원본 입력마다 실제 상태를 새로 구성한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0] + ':' + row[1])); var scene = new Scene(row[1]); string result = "-";
            if (row[0] == "C") { byte[] before = (byte[])scene.Players.Clone(); result = string.Join(',', new uint[] { scene.Counts.Subscribers(), scene.Counts.Participating(), scene.Counts.Disconnected(), scene.Counts.Ready(), scene.Counts.LoggedIn() }); Assert.Equal(before, scene.Players); }
            else if (row[0] == "E") result = scene.Timer.Expired() ? "1" : "0";
            else if (row[0] == "B") scene.Timer.Begin(); else if (row[0] == "A") scene.Timer.Abort(); else { Assert.Contains(row[0], new[] { "R", "S" }); scene.Timer.Run(row[0] == "S"); }
            Assert.Equal(row[2], result); Assert.Equal(row[3], scene.Observe()); Assert.Equal(row[4], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
        }
    }

    /// <summary>사제/저장 하위의 실제 시점과 미Ready 미션 후처리의 자연 완료·늦은 쓰기를 확인한다.</summary>
    [Fact]
    public void UnreadyMissionPostPreservesCallbacksAfterCompletionStores()
    {
        var scene = new Scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002540|0000000000002440"); uint priests = 0; var hooks = scene.Hooks with
        {
            PriestRefresh = owner => { Assert.Equal(++priests, owner); Assert.NotEqual(0u, scene.State.ScriptBreak.Value); scene.State.ScriptBreak.Value = 0x80000000; },
            SaveReconnect = () => { Assert.Equal(8u, priests); Assert.Equal(0u, scene.State.Begin.Value); Assert.Equal(0u, scene.State.ScriptBreak.Value); Assert.Equal(10.5, scene.State.End.Value); scene.State.ScriptBreak.Value = uint.MaxValue; },
            CloseProgress = () => { Assert.Equal(0d, scene.State.End.Value); Assert.Equal(uint.MaxValue, scene.State.ScriptBreak.Value); scene.State.ScriptBreak.Value = 0x80000000; },
        };
        var timer = new RawMissionLoginTimer(OriginalEdition.Patch1078, scene.State, scene.Counts, scene.Reads, hooks); timer.Run(false); Assert.Equal(8u, priests); Assert.Equal(0x80000000u, scene.State.ScriptBreak.Value); Assert.DoesNotContain(scene.Trace, item => item.StartsWith("B,") || item.StartsWith("U,"));
    }

    /// <summary>중단의 선행 실제 쓰기와 하위 변경 보존·누락 장치 뒤 선택되지 않은 꼬리를 구별한다.</summary>
    [Fact]
    public void AbortAndBeginPreserveSelectedFailurePrefixes()
    {
        var scene = new Scene("1,30,0,1,1,1,0,0,0,1,0,0,0,1|0000000000002440|0000000000002440"); int calls = 0;
        var timer = new RawMissionLoginTimer(OriginalEdition.Patch1078, scene.State, scene.Counts, scene.Reads, scene.Hooks with
        {
            AbortBattle = () => { Assert.Equal(1u, scene.State.ScriptBreak.Value); Assert.Equal(4u, scene.State.RequestPhase.Value); scene.State.RequestPhase.Value = 7; scene.State.Begin.Value = 29; calls++; },
            CloseProgress = () => { Assert.Equal(29u, scene.State.Begin.Value); Assert.Equal(7u, scene.State.RequestPhase.Value); calls++; },
        });
        timer.Abort(); Assert.Equal(2, calls); Assert.Equal(7u, scene.State.RequestPhase.Value);
        timer = new(OriginalEdition.Patch1078, scene.State, scene.Counts, scene.Reads, scene.Hooks with { AbortBattle = null, CloseProgress = () => Assert.Fail("선택되지 않은 꼬리") }); Assert.Throws<InvalidOperationException>(timer.Abort); Assert.Equal(1u, scene.State.ScriptBreak.Value); Assert.Equal(4u, scene.State.RequestPhase.Value);
        timer = new(OriginalEdition.Patch1078, scene.State, scene.Counts, scene.Reads, scene.Hooks with { Broadcast = null }); Assert.Throws<InvalidOperationException>(timer.Begin); Assert.Equal(0d, scene.State.End.Value); Assert.Equal(0u, scene.State.Begin.Value);
        Assert.Throws<ArgumentException>(() => new RawMissionLoginTimer(OriginalEdition.Patch1078, scene.State, scene.Counts, scene.Reads with { Server = null }, scene.Hooks));
    }

    /// <summary>다섯 실제 기록의 상태 문구·현재 제목과 네 인자 기본 진행 서식을 확인한다.</summary>
    [Fact]
    public void ProgressTextReadsCurrentRecordsAndOriginalStatusKeys()
    {
        byte[] players = new byte[1620]; uint[] flags = [0, 0x102, 2, 3, 0x43];
        // 서로 다른 현재 상태를 가진 실제 이름/구독자 입력을 준비한다.
        for (int owner = 1; owner <= 5; owner++) { players[owner * 180 + 12] = (byte)('A' + owner - 1); Put(players, owner * 180 + 0x2c, (uint)owner); Put(players, owner * 180 + 0x50, flags[owner - 1]); }
        var state = new MissionLoginTimerState { End = new() { Value = 20 }, Timeout = new() { Value = 30 } }; var reads = new MissionLoginTimerReads { SavedGame = () => false, Server = () => true, Mission = () => true, MissionPost = () => true, InitialGeysers = () => false, CheckingAssertions = () => false };
        var timer = new RawMissionLoginTimer(OriginalEdition.Patch1078, state, new(OriginalEdition.Patch1078, players), reads, new()); var keys = new List<string>(); string text = timer.ProgressText(10, key => { keys.Add(key); return key; });
        Assert.Equal("~.~3Becoming Server~*~.\n\nA: ~iConnecting~.\nB: ~iDisconnected~.\nC: ~iPending~.\nD: ~iTransferring~.\nE: ~iReady~.\n\nTime Remaining: ~i10~.", text);
        Assert.Equal(new[] { ": ~iConnecting~.", ": ~iDisconnected~.", ": ~iPending~.", ": ~iTransferring~.", ": ~iReady~.", "Becoming Server", "~.~3%s~*~.\n\n%s\nTime Remaining: ~i%d~.%s" }, keys);
    }

    /// <summary>프레임 시각/현재 end의64비트 절삭 하위DWORD와 음수0정규화·비유한을 구별한다.</summary>
    [Theory]
    [InlineData(3.75, 3)]
    [InlineData(-3.75, 0)]
    [InlineData(4294967299d, 3)]
    [InlineData(double.NaN, 0)]
    [InlineData(double.PositiveInfinity, 0)]
    public void ProgressRemainingUsesRawFtolLowDword(double end, int expected)
    {
        var scene = new Scene("0,30,0,0,1,0,0,0,0,1,0,0,0,1|0000000000000000|0000000000000000"); scene.State.End.Value = end;
        string text = scene.Timer.ProgressText(0, key => key); Assert.Contains($"Time Remaining: ~i{expected}~.", text); Assert.Equal(expected == 0, text.Contains("$ButtonInstant=Abort"));
    }

    /// <summary>고정 폭/정밀도·부호/0패딩·%%와 미공급 인자/버퍼초과 거부를 확인한다.</summary>
    [Fact]
    public void ProgressFormatterKeepsConstantPrintfFieldsAndRejectsUnsafeFormats()
    {
        var scene = new Scene("0,30,0,0,1,0,0,0,0,1,0,0,0,1|0000000000002440|0000000000000000");
        string text = scene.Timer.ProgressText(0, key => key.StartsWith("~.~3") ? "%8.3s|%-8.4s|%+06d|%.0s%%" : key); Assert.Equal("     Sta|        |+00010|%", text);
        // 번역이 허용된 네 인자 타입/범위를 벗어나면 실제 원문 경계에서 거부한다.
        foreach (string pattern in new[] { "%s%s%d", "%s%s%s%s", "%*s%s%d%s", "%s%s%d%s%s" }) Assert.Throws<ArgumentException>(() => scene.Timer.ProgressText(0, key => key.StartsWith("~.~3") ? pattern : key));
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Timer.ProgressText(0, key => key.StartsWith("~.~3") ? "%1024s%s%d%s" : key));
    }

    /// <summary>강제 시작 안내의 번역 뒤 현재 서버를 재조회하며 이미 선택한 안내와 현재 제목을 별도로 유지한다.</summary>
    [Fact]
    public void ProgressTranslationCanChangeCurrentServerBeforeTitleSelection()
    {
        var scene = new Scene("0,30,0,1,1,0,0,0,0,1,0,0,0,1|0000000000002440|0000000000000000"); var keys = new List<string>();
        string text = scene.Timer.ProgressText(0, key => { keys.Add(key); if (key.Contains("ServerForceBattleStart")) scene.CurrentModes[1] = false; return key; });
        Assert.StartsWith("~.~3Starting Game", text); Assert.Contains("$ButtonInstant=Start", text); Assert.Contains("Starting Game", keys); Assert.DoesNotContain("Becoming Server", keys);
    }
}
