using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 독립 원본8종 입력을 실제 Player/방향동맹/분산SP/공유 옵션 부모에 공급한다.
namespace Netstorm.Core.Tests;

/// <summary>원본157입력의 전체 현재값과 callback 뒤 재조회·실제 별칭을 검증한다.</summary>
public sealed class MissionClientPacketsTests
{
    /// <summary>같은 실제 저장소의 DWORD 입력/관측을 기록한다.</summary>
    private static void Put(Span<byte> bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes[at..], value);
    /// <summary>관측한 단일바이트 문자열의 원문 비트를 보존한다.</summary>
    private static string Hex(string text) => Hex(Encoding.Latin1.GetBytes(text));
    /// <summary>전체 바이트를 fixture 표현과 같은 소문자hex로 쓴다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

    /// <summary>한 원본 입력에 필요한 실제 Player/참여/SP/전역과 외부 효과 관찰자다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 Player9다.</summary>
        public byte[] Players { get; } = new byte[1620];
        /// <summary>같은 현재 옵션87바이트다.</summary>
        public byte[] Options { get; } = new byte[87];
        /// <summary>같은 방향별81DWORD다.</summary>
        public uint[] Allies { get; } = new uint[81];
        /// <summary>현재 주소6바이트다.</summary>
        public byte[] Zone { get; } = [0x11, 0x12, 0x13, 0x14, 0x15, 0x16];
        /// <summary>같은10바이트 캐시다.</summary>
        public byte[] Cached { get; } = new byte[10];
        /// <summary>현재 선택된 실제 CoreData 별칭이다.</summary>
        public Memory<byte> Core { get; set; }
        /// <summary>현재 서버 관문이며 채널 callback이 변경할 수 있다.</summary>
        public bool Server { get; set; }
        /// <summary>현재 방장 관문이다.</summary>
        public bool Boss { get; set; }
        /// <summary>현재 미션 후처리 관문이다.</summary>
        public bool Mission { get; set; }
        /// <summary>현재 phase이며 벽시각 callback이 변경할 수 있다.</summary>
        public uint Phase { get; set; }
        /// <summary>현재 로그인 마감이며 송신 callback이 변경할 수 있다.</summary>
        public double LoginEnd { get; set; }
        /// <summary>독립 원본의 variant 명시 입력이다.</summary>
        public uint Variant { get; }
        /// <summary>독립 원본의 callback 변경 명시 입력이다.</summary>
        public uint Mutate { get; }
        /// <summary>한 번만 실행할 변경을 이미 공급했는지다.</summary>
        private bool _mutated;
        /// <summary>실제 비용 TYPE 쓰기의 현재float32 비트다.</summary>
        public uint Energy { get; private set; }
        /// <summary>같은 현재 수신 패킷이며 callback에서 직접 변경한다.</summary>
        public byte[] Packet { get; }
        /// <summary>실제 외부 호출 순서만 기록한다.</summary>
        public List<string> Trace { get; } = [];
        /// <summary>실제 두 owner/known 저장소다.</summary>
        public PlayerWorldState Player { get; } = new();
        /// <summary>같은 난수와 실제 분산SP 저장소다.</summary>
        public RawSpStore Sp { get; } = new(new GameRandom(42), () => 9);
        /// <summary>같은 Player/방향동맹/옵션 안 숨김 표를 소비하는 실제 참여 모듈이다.</summary>
        public RawPlayerParticipation Participation { get; }
        /// <summary>수신기가 직접 읽고 쓰는 현재 실제 전역 뷰다.</summary>
        public MissionClientPacketState State { get; }
        /// <summary>큰 하위의 실제 인자만 관찰하는 외부 검사 장치다.</summary>
        public MissionClientPacketHooks Hooks { get; }
        /// <summary>동일 저장소를 빌리는 실제8종 수신 본체다.</summary>
        public RawMissionClientPackets Packets { get; }

        /// <summary>기대 결과를 계산하지 않고 독립 원본에 공급했던 같은 입력만 준비한다.</summary>
        public Scene(uint[] input, byte[] packet)
        {
            Packet = packet; Server = input[1] != 0; Boss = input[2] != 0; Mission = input[3] != 0; Player.PlayerKnown = input[4]; Player.BattleOwner = Player.TemplateOwner = input[5]; Variant = input[6]; Mutate = input[7];
            Phase = new uint[] { 10, 16, 9 }[(Variant >> 1) % 3]; LoginEnd = (Variant & 1) != 0 ? 0 : 30; Array.Fill(Options, (byte)0xa5); Array.Fill(Cached, (byte)0x55); Core = Players.AsMemory(180 + 0x6c, 10); Sp.Initialize();
            // 원본의 명시 Player 이름/flags/동맹과 초기SP만 공급한다.
            for (int owner = 0; owner < 9; owner++)
            {
                Encoding.Latin1.GetBytes("N" + owner).CopyTo(Players, owner * 180 + 12); Put(Players, owner * 180 + 0x50, 0x200); Put(Players, owner * 180 + 0x54, 1u << owner); Allies[owner * 9 + owner] = 1; Sp.Set((uint)owner, BitConverter.SingleToUInt32Bits(900f + owner));
            }
            Participation = new(OriginalEdition.Patch1078, Players, Allies, Options.AsMemory(0x3e, 9), new(), new()
            {
                ClearChat = flags => Trace.Add("Clear," + flags), AddChat = Chat, Assertion = _ => throw new InvalidOperationException("예상 밖 참여 진단"),
            });
            Participation.BindCurrentInputs(new() { LocalOwner = () => Player.BattleOwner, PlayerKnown = () => Player.PlayerKnown, Battle = () => true, Metamap = () => false, LocalTransport = () => false });
            State = new()
            {
                Server = () => Server, Boss = () => Boss, MissionPost = () => Mission, SavedGame = () => true, Metamap = () => false, InBattle = () => true, Editor = () => Variant & 1, Outpost = () => Variant & 1,
                Player = Player, BeginSeed = new() { Value = 7 }, BattleSeed = new() { Value = 8 }, Revision = new() { Value = 9 }, OptionValue = new() { Value = 10 },
                ClockStamp = new() { Value = 11 }, BaseClock = new() { Value = 12 }, Phase = () => Phase, CurrentState = new() { Value = 13 }, CurrentAux = new() { Value = 14 }, PendingState = new() { Value = 15 }, SourceTicks = new() { Value = 16 }, CoreDataActive = new() { Value = 17 },
                CaptureEnd = new() { Value = 77.25 }, LoginEnd = () => LoginEnd, ReadCoreData = () => Core, StoreCoreData = value => Core = value, CachedCoreData = Cached, ZoneAddress = Zone, BattleOptions = Options, CurrentHandle = "My Name\0"u8.ToArray(), Hidden = Options.AsMemory(0x3e, 9),
            };
            Hooks = new()
            {
                Assertion = text => Trace.Add("Assert," + Hex(text)), ConfigInt = key => { Trace.Add("Config," + key); return (Variant & 32) != 0 ? 1 : 0; }, Tell = text => Trace.Add("Tell," + Hex(text)),
                RunState = () => Trace.Add("State," + State.CurrentState.Value), ResetCamera = () => Trace.Add("Camera"), RefreshSp = () => { Trace.Add("Refresh"); if (Mutate == 3) Put(Packet, 4, 2); },
                WallSeconds = () => { Trace.Add("Wall"); if (Mutate == 1) Phase = 16; return 100.5; }, Clock = () => { Trace.Add("Clock"); return 7501; },
                OutpostOptions = (sender, _) => Trace.Add("Outpost," + sender), UpdateRange = () => Trace.Add("Range"), RequiredEnergy = () => { Trace.Add("Energy"); return 1251; }, SetEnergyType = value => { Trace.Add("Type,122"); Energy = BitConverter.SingleToUInt32Bits(value); }, ReadOptionValue = () => { Trace.Add("Option"); return 17; }, ReconnectUi = () => Trace.Add("ReconnectUI"),
                OpenFortImage = owner => { Trace.Add("ImageName," + owner); Trace.Add("Open," + owner); }, ReadFortImage = (owner, image) => Trace.Add($"Image,{owner},{Hex(image.Span)}"), BuildFort = owner => { Trace.Add("Builder"); Trace.Add("Attach," + owner); Trace.Add("Build"); },
                PeerAddress = owner => { Trace.Add("IP," + owner); return "127.0.0.1"; }, AddChat = Chat, RemoveChat = (subscriber, flags) => Trace.Add($"Remove,{subscriber},{flags}"), RestoreCoreData = owner => Trace.Add("Restore," + owner), PlayerUiBusy = () => { Trace.Add("Busy"); return (Variant & 64) != 0; }, SaveReconnect = () => Trace.Add("Save"), PlayerNotice = (kind, owner, argument) => Trace.Add($"Notice,{kind},{owner},{argument}"),
                Send = Send, SendReady = target => RawMissionReady.Send(target, Players, Boss, text => Trace.Add("Assert," + Hex(text)), bytes => Send(0, bytes), (destination, bytes) => Send(destination, bytes)),
            };
            Packets = new(Players, Participation, Sp, State, Hooks);
        }
        /// <summary>실제 채널 호출을 관찰하고 지정된 현재 서버 변경만 공급한다.</summary>
        private void Chat(uint subscriber, string name, uint flags)
        {
            Trace.Add($"Chat,{subscriber},{Hex(name)},{flags}"); if (Mutate == 2 && !_mutated) { Server = false; _mutated = true; }
        }
        /// <summary>현재 실제 송신 내용을 기록하고 원본 관찰에 지정한 타이머 변경만 공급한다.</summary>
        private void Send(int target, ReadOnlyMemory<byte> packet) { Trace.Add($"Send,{target},{Hex(packet.Span)}"); if (Mutate == 4) LoginEnd = 0; }
        /// <summary>CoreData의 실제 배열/offset을 원본 관측의 주소 차이 형식으로 표현한다.</summary>
        private uint CoreIdentity()
        {
            if (Core.IsEmpty) return 0; Assert.True(MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)Core, out ArraySegment<byte> segment));
            if (ReferenceEquals(segment.Array, Cached)) return 0xfffffffe; Assert.Same(Players, segment.Array); return (uint)segment.Offset + 1;
        }
        /// <summary>같은 현재 전역20개와double 원시 비트를 직접 직렬화한다.</summary>
        public string StateText() => string.Join(',', new ulong[] { Player.BattleOwner, Player.TemplateOwner, Player.PlayerKnown, State.BeginSeed.Value, State.BattleSeed.Value, State.Revision.Value, State.OptionValue.Value, State.Begin.Value, State.ClockStamp.Value, State.BaseClock.Value, Phase, State.CurrentState.Value, State.CurrentAux.Value, State.PendingState.Value, State.SourceTicks.Value, State.CoreDataActive.Value, Server ? 1u : 0, CoreIdentity(), BitConverter.DoubleToUInt64Bits(State.CaptureEnd.Value), BitConverter.DoubleToUInt64Bits(LoginEnd) });
        /// <summary>현재 주소/옵션/cache/TYPE쓰기 전체를 원문 순서로 직렬화한다.</summary>
        public string StorageText() { byte[] energy = new byte[4]; Put(energy, 0, Energy); return Hex(Zone) + Hex(Options) + Hex(Cached) + Hex(energy); }
        /// <summary>실제 분산SP에서 현재9소유자의float32 비트를 읽는다.</summary>
        public string SpText()
        {
            byte[] bytes = new byte[36];
            // 실제 분산 저장소 읽기이며 기대 모델의 float 배열을 사용하지 않는다.
            for (uint owner = 0; owner < 9; owner++) Put(bytes, (int)owner * 4, Sp.Get(owner)); return Hex(bytes);
        }
        /// <summary>현재 방향별81DWORD 전체를 little endian으로 직렬화한다.</summary>
        public string AlliesText()
        {
            byte[] bytes = new byte[324];
            // 양방향을 합치지 않고 실제 전체 행렬을 비교한다.
            for (int index = 0; index < Allies.Length; index++) Put(bytes, index * 4, Allies[index]); return Hex(bytes);
        }
    }

    /// <summary>원본157입력의 전체 Player/동맹/전역/storage/SP/외부효과를 전수 대조한다.</summary>
    [Fact]
    public void OriginalClientPackets157()
    {
        string[][] rows = X86Fixture.Read("mission-client-packets").Where(row => row[0] != "input").ToArray(); Assert.Equal(157, rows.Length); var seen = new HashSet<string>();
        // 기대 fixture는 읽기만 하며 동일 입력마다 새 실제 저장소를 구성한다.
        foreach (string[] row in rows)
        {
            Assert.True(seen.Add(row[0])); uint[] input = row[0].Split(',').Select(X86Fixture.UInt).ToArray(); var scene = new Scene(input, X86Fixture.Bytes(row[1])); scene.Packets.Receive(input[8], scene.Packet);
            Assert.Equal(row[2], Hex(scene.Players)); Assert.Equal(row[3], scene.AlliesText()); Assert.Equal(row[4], scene.StateText()); Assert.Equal(row[5], scene.StorageText()); Assert.Equal(row[6], scene.SpText());
            string expected = string.Join(';', row[7].Split(';').Where(item => !item.StartsWith("Set,"))); // 분산SP 하위 쓰기는 위의 전체SP 비트로 비교하며 가짜callback을 넣지 않는다.
            Assert.Equal(string.IsNullOrEmpty(expected) ? "-" : expected, scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
        }
    }

    /// <summary>현재 표의8종 등록과 고정F는 packet[1] 변경 뒤에도 선택한 실제 몸체를 유지한다.</summary>
    [Fact]
    public void RegistrationUsesOriginalLengthsAndFixedReceiverIdentity()
    {
        var scene = new Scene([132, 1, 1, 1, 1, 1, 0, 0, 9], new byte[] { 1, 0x6e, 8, 0, 2, 0, 0, 0 });
        var dispatch = new RawPacketDispatch(OriginalEdition.Patch1078, new() { Now = () => 0, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } }); MissionClientPacketRegistration.Register(dispatch, scene.Packets);
        // 전체 원문 길이와 version/정책·양방향 observer의 존재를 대조한다.
        foreach ((uint type, ushort length) in new (uint, ushort)[] { (0x29, 34), (0x67, 65535), (0x2b, 65535), (0x2a, 65535), (0x28, 18), (0x81, 65535), (0x84, 8), (0x6e, 8) })
        {
            PacketRegistration entry = dispatch.Registration(type); Assert.Equal(1, entry.Version); Assert.Equal(0, entry.Mismatch); Assert.Equal(length, entry.Length); Assert.NotNull(entry.Receive);
            Assert.True(entry.Observer is not null || entry.PacketObserver is not null); entry.NotifyObserver(Memory<byte>.Empty, 0);
        }
        dispatch.InvokeDirect(0x84, 9, new(), scene.Packet); Assert.Equal(new[] { "Clock" }, scene.Trace); Assert.Equal(5501u, scene.State.BaseClock.Value);
        Assert.Throws<ArgumentException>(() => scene.Packets.ReceiveFixed(0, 9, scene.Packet)); Assert.Throws<ArgumentException>(() => scene.Packets.Receive(9, new byte[] { 1, 0, 4, 0 }));
    }

    /// <summary>owner0은 기존 실제10바이트를 캐시에 복사하고 다른 owner는 Player/메타 캐시 별칭을 선택한다.</summary>
    [Fact]
    public void LoginBeginPreservesActualCoreAliasesAndSharedSeed()
    {
        var scene = new Scene([41, 0, 0, 1, 1, 1, 0, 0, 9], new byte[34]); scene.Core.Span.Fill(0x72); Put(scene.Packet, 22, 123); Put(scene.Packet, 30, uint.MaxValue); var shared = new IntBox();
        var packets = new RawMissionClientPackets(scene.Players, scene.Participation, scene.Sp, scene.State with { SharedBattleSeed = shared }, scene.Hooks);
        packets.ReceiveFixed(0x004b3b40, 9, scene.Packet); Assert.True(scene.Core.IsEmpty); Assert.All(scene.Cached, value => Assert.Equal(0x72, value)); Assert.Equal(0u, scene.State.CoreDataActive.Value); Assert.Equal(-1, shared.Value); Assert.Equal(8u, scene.State.BattleSeed.Value);
        Put(scene.Packet, 4, 2); packets.ReceiveFixed(0x004b3b40, 9, scene.Packet); Assert.True(scene.Core.Equals(scene.Players.AsMemory(360 + 0x6c, 10))); scene.Core.Span[0] = 88; Assert.Equal(88, scene.Players[360 + 0x6c]);
        var meta = new RawMissionClientPackets(scene.Players, scene.Participation, scene.Sp, scene.State with { Metamap = () => true }, scene.Hooks); meta.ReceiveFixed(0x004b3b40, 9, scene.Packet); Assert.True(scene.Core.Equals(scene.Cached.AsMemory()));
        Assert.Throws<ArgumentException>(() => new RawMissionClientPackets(scene.Players, scene.Participation, scene.Sp, scene.State with { Hidden = new byte[9] }, scene.Hooks));
    }

    /// <summary>실제 이미지 하위는 현재 수신 패킷의 같은 이미지 구간을 빌리며 누락된 옵션 효과 전 복사를 보존한다.</summary>
    [Fact]
    public void FortImageBorrowsActualReceiveBufferAndMissingOptionEffectPreservesPrefix()
    {
        byte[] packet = new byte[22]; Put(packet, 4, 1); Put(packet, 12, 5); packet[17] = 77;
        var scene = new Scene([42, 0, 0, 1, 1, 1, 0, 0, 9], packet); bool borrowed = false;
        var packets = new RawMissionClientPackets(scene.Players, scene.Participation, scene.Sp, scene.State, scene.Hooks with
        {
            ReadFortImage = (owner, image) => { Assert.Equal(1u, owner); Assert.True(image.Equals(packet.AsMemory(17, 5))); Assert.True(MemoryMarshal.TryGetArray(image, out ArraySegment<byte> view)); Assert.Same(packet, view.Array); borrowed = true; },
        });
        packets.ReceiveFixed(0x004b3d30, 9, packet); Assert.True(borrowed); Assert.Equal(10u, scene.State.Revision.Value);
        packets = new(scene.Players, scene.Participation, scene.Sp, scene.State, scene.Hooks with { UpdateRange = null });
        byte[] option = [1, 0x67, 8, 0, 0, 1, 0, 0x77]; Assert.Throws<InvalidOperationException>(() => packets.Options(9, option)); Assert.Equal(0x77, scene.Options[0]); Assert.Equal(10u, scene.State.OptionValue.Value);
    }

    /// <summary>Ready writer는 진단 복귀 뒤 현재 낮은BYTE만 읽고 실제 전체분배/지정송신을 구별한다.</summary>
    [Fact]
    public void ReadyWriterReadsCurrentFlagsAfterAssertionAndUsesRealTarget()
    {
        byte[] players = new byte[1620]; players[0x50] = 0x40; players[180 + 0x51] = 0x40; int assertions = 0, broadcasts = 0;
        RawMissionReady.Send(0, players, false, text => { assertions++; Assert.Equal("boss", text); players[360 + 0x50] = 0x40; }, packet => { broadcasts++; Assert.Equal(new byte[] { 1, 0x6e, 8, 0, 4, 0, 0, 0 }, packet.ToArray()); }, null);
        Assert.Equal(1, assertions); Assert.Equal(1, broadcasts); RawMissionReady.Send(uint.MaxValue, players, true, null, null, (target, packet) => { Assert.Equal(-1, target); Assert.Equal(4u, BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..])); });
    }

    /// <summary>참여 모듈의 숨김 채널은 생성 뒤 변경한 같은 옵션 구간을 다음 재구성에서 직접 읽는다.</summary>
    [Fact]
    public void ParticipationReadsSameHiddenSliceAfterOptionMutation()
    {
        var scene = new Scene([43, 0, 0, 1, 1, 1, 0, 0, 9], []); scene.Participation.SetAlliance(1, 2);
        Assert.Contains("Chat,0,4e30,128", scene.Trace); scene.Trace.Clear(); scene.Options.AsSpan(0x3e, 9).Clear(); scene.Participation.SetAlliance(1, 2);
        Assert.DoesNotContain(scene.Trace, item => item.StartsWith("Chat,") && item.EndsWith(",128"));
    }
}
