// 0x20 참가자 생성 본체의 원문 필드·이름·상태·이미지·객체·이름표·입장 분기를 대조한다.
// cpppj/src/client/NativeMetaParticipantCreate.cpp 의 ConstructNativeMetaParticipant를 대조한다.
// 0.961에서 R4-6 메타 참가자 묶음용으로 추가됐다.
using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// 0x20 참가자 생성의 실제 장치 호출을 기록한다.
public sealed class ClientMetaParticipantConstructTests
{
    /// <summary>같은 실제 기록과 장치 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 게임 보조 owner다.</summary>
        public readonly RawGameAuxState GameAux = new();

        /// <summary>같은 실제 자기 구독자 번호 owner다.</summary>
        public readonly UIntBox Subscriber = new();

        /// <summary>실제 진단 호출이다.</summary>
        public readonly List<(string Expression, string File, uint Line)> Assertions = [];

        /// <summary>실제 종류 지정이다.</summary>
        public readonly List<(uint Object, uint Kind)> Kinds = [];

        /// <summary>실제 객체 물리다.</summary>
        public readonly Dictionary<uint, byte[]> Objects = [];

        /// <summary>다음 객체 번호다.</summary>
        public uint NextObject = 0x9000;

        /// <summary>실제 소유자 표시다.</summary>
        public readonly List<(uint Object, uint Owner)> Paints = [];

        /// <summary>실제 이동 호출이다.</summary>
        public readonly List<(uint Object, float X, float Y, uint Flags)> Moves = [];

        /// <summary>실제 문구 호출이다.</summary>
        public readonly List<(uint Label, string Text, uint Flags)> Texts = [];

        /// <summary>실제 일련이다.</summary>
        public readonly Dictionary<uint, uint> Serials = [];

        /// <summary>다음 이름표 번호다.</summary>
        public uint NextLabel = 0x7000;

        /// <summary>다음 일련 번호다.</summary>
        public uint NextSerial = 0x7100;

        /// <summary>실제 부착 호출이다.</summary>
        public readonly List<(uint Label, int A, int B, uint Root, uint Flags)> Attaches = [];

        /// <summary>실제 선택·스크롤·연결 호출이다.</summary>
        public readonly List<(uint Label, uint Value)> Selects = [];

        /// <summary>실제 길드 조회 결과다.</summary>
        public bool GuildFound = true;

        /// <summary>실제 번역 호출이다.</summary>
        public readonly List<string> Translates = [];

        /// <summary>실제 장부 갱신 횟수다.</summary>
        public int RefreshCores;

        /// <summary>실제 상태 호출이다.</summary>
        public readonly List<(int Kind, uint Participant, bool Enabled)> Statuses = [];

        /// <summary>실제 그림 호출이다.</summary>
        public readonly List<(int Length, int Identity)> Images = [];

        /// <summary>실제 생성 호출이다.</summary>
        public readonly List<(uint Type, uint Flags)> Creates = [];

        /// <summary>실제 입장 호출이다.</summary>
        public readonly List<(uint Object, bool Self)> Enters = [];

        /// <summary>실제 인트로 횟수다.</summary>
        public int Intros;

        /// <summary>실제 숨김 조회 결과다.</summary>
        public bool HideIntro;

        /// <summary>실제 Tell 호출이다.</summary>
        public readonly List<string> Tells = [];

        /// <summary>실제 감시 호출이다.</summary>
        public readonly List<int> Monitors = [];

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaParticipantConstructHooks Hooks() => new()
        {
            GameAux = GameAux,
            Subscriber = Subscriber,
            Display = new()
            {
                Assertion = (expression, file, line) => Assertions.Add((expression, file, line)),
                SetKind = (created, kind) => Kinds.Add((created, kind)),
                Raw = created => Objects.TryGetValue(created, out byte[]? raw) ? raw : throw new InvalidOperationException("표시 객체 미등록"),
                PaintOwner = (created, owner) => Paints.Add((created, owner)),
                Move = (created, x, y, flags) => Moves.Add((created, x, y, flags)),
                ReplaceText = (label, text, flags) => Texts.Add((label, text, flags)),
                Gump = label => Serials.TryGetValue(label, out uint serial) ? serial : throw new InvalidOperationException("이름표 일련 미등록"),
                Attach = (label, a, b, root, flags) => Attaches.Add((label, a, b, root, flags)),
                CreateName = () => { uint label = NextLabel++; Serials[label] = NextSerial++; return label; },
                Selectable = (label, value) => Selects.Add((label, value)),
                Scrollable = (label, value) => Selects.Add((label, unchecked((uint)(int)value + 0x10000))),
                Reference = (label, created) => Selects.Add((label, created)),
                Root = () => 0x6000,
            },
            Name = new()
            {
                Translate = text => { Translates.Add(text); return text; },
                Weight = _ => 42,
            },
            GuildChanged = _ => GuildFound,
            Guild = (_, destination) =>
            {
                byte[] guild = Encoding.ASCII.GetBytes("TAG");
                guild.CopyTo(destination.Span);
                destination.Span[guild.Length] = 0;
            },
            RefreshCore = () => RefreshCores++,
            Status = (kind, participant, enabled) => Statuses.Add((kind, participant, enabled)),
            Image = (header, identity) => { Images.Add((header.Length, identity.Length)); header.Span[0] = 1; },
            ParticipantType = () => 108,
            Create = (type, flags) => { Creates.Add((type, flags)); uint created = NextObject++; Objects[created] = new byte[50]; Objects[created][0xb] = 4; return created; },
            Enter = (created, self) => Enters.Add((created, self)),
            Intro = () => Intros++,
            NoIntro = () => HideIntro,
            Tell = section => Tells.Add(section),
            Monitor = name => Monitors.Add(name.Length),
        };

        /// <summary>원문 0x20 입력 패킷을 조립한다.</summary>
        public static byte[] Packet(byte subscriber, string name, uint flags = 0)
        {
            byte[] body = Encoding.ASCII.GetBytes(name);
            byte[] packet = new byte[0x35 + body.Length + 1];
            packet[1] = 0x20;
            packet[4] = 3;
            packet[5] = subscriber;
            packet[0x23] = (byte)flags;
            body.CopyTo(packet, 0x26);
            body.CopyTo(packet, 0x35);
            return packet;
        }
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => ClientMetaParticipantConstruct.Construct(new byte[460], new byte[0x40], null!));
        var bare = new ClientMetaParticipantConstructHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaParticipantConstruct.Construct(new byte[460], Scene.Packet(1, "Al"), bare));
    }

    /// <summary>필드·이름·장부를 원문 순서로 쓴다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void FieldsAndNameCopied()
    {
        var scene = new Scene();
        scene.GuildFound = false;
        scene.Subscriber.Value = 0x99;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "A?B"), scene.Hooks());
        Assert.Equal(0x11u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(4, 4)));
        Assert.Equal((byte)'A', record[0x26]);
        Assert.Equal((byte)'B', record[0x27]);
        Assert.Equal(0, record[0x28]);
        Assert.Equal(1, scene.RefreshCores);
        Assert.Equal(0u, scene.GameAux.SubscriberMarker);
        Assert.Empty(scene.Statuses);
        Assert.Empty(scene.Monitors);
    }

    /// <summary>길드는 버전 꼬리를 붙이고 이름표 접두사로 쓴다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void GuildAppendsVersionTag()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x99;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Bo"), scene.Hooks());
        string label = Encoding.Latin1.GetString(record.AsSpan(0xd8));
        label = label[..label.IndexOf('\0')];
        Assert.StartsWith("~j~aTAG~y::~wBo", label, StringComparison.Ordinal);
    }

    /// <summary>표식·자기는 상태·감시를 수행한다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void MarkerTriggersStatusAndMonitor()
    {
        var scene = new Scene();
        scene.GameAux.SubscriberMarker = 7;
        scene.Subscriber.Value = 0x77;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Al"), scene.Hooks());
        Assert.Equal([(5, 3u, false), (3, 3u, true)], scene.Statuses);
        Assert.Equal(1u, scene.GameAux.SubscriberMarker);
        Assert.Equal([422], scene.Monitors);
    }

    /// <summary>자기 참가자는 표식 없이도 상태·감시를 수행한다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void SelfTriggersStatusAndMonitor()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x11;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Al"), scene.Hooks());
        Assert.Equal([(5, 3u, false), (3, 3u, true)], scene.Statuses);
        Assert.Equal([422], scene.Monitors);
    }

    /// <summary>타인은 상태·감시를 생략한다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void StrangerSkipsStatusAndMonitor()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x99;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Al"), scene.Hooks());
        Assert.Empty(scene.Statuses);
        Assert.Empty(scene.Monitors);
        Assert.Equal(0u, scene.GameAux.SubscriberMarker);
    }

    /// <summary>그림·객체 본문이 없으면 진단한다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void ImageAndVoidAssert()
    {
        var scene = new Scene();
        var hooks = scene.Hooks();
        hooks.Image = (_, _) => { };
        hooks.Display!.Raw = _ => new byte[50];
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Al"), hooks);
        Assert.Equal(2, scene.Assertions.Count);
    }

    /// <summary>생성·표시·Rig 순서로 객체를 만든다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void CreateMoveSequenceAndLabel()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x99;
        byte[] record = new byte[460];
        byte[] packet = Scene.Packet(0x11, "Al");
        ClientMetaParticipantConstruct.Construct(record, packet, scene.Hooks());
        float expectedX = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(0x27, 4)));
        float expectedY = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(packet.AsSpan(0x2b, 4)));
        Assert.Equal([(108u, 2u)], scene.Creates);
        Assert.Equal([(0x9000u, 0x11u)], scene.Kinds);
        Assert.Equal([(0x9000u, expectedX, expectedY, 0u)], scene.Moves);
        Assert.Equal(0x9000u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(0xb8, 4)));
        Assert.Single(scene.Texts);
        Assert.Equal([(0x7000u, 0, 0, 0x6000u, 1u)], scene.Attaches);
        Assert.Equal(0x7000u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(0xd4, 4)));
        Assert.Equal(0x7100u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(0xd0, 4)));
    }

    /// <summary>packet 0x40이면 입장하지 않는다. 0.961에서 추가됐다.</summary>
    [Theory]
    [InlineData((byte)0x40, 0)]
    [InlineData((byte)0x00, 1)]
    public void EnterFollowsPacketBit(byte flags, int expected)
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x99;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Al", flags), scene.Hooks());
        Assert.Equal(expected, scene.Enters.Count);
    }

    /// <summary>자기는 인트로·Tell을 수행한다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void IntroTellWhenSelfAndNotHidden()
    {
        var scene = new Scene();
        scene.Subscriber.Value = 0x11;
        byte[] record = new byte[460];
        ClientMetaParticipantConstruct.Construct(record, Scene.Packet(0x11, "Al"), scene.Hooks());
        Assert.Equal(1, scene.Intros);
        Assert.Equal(["ChalIntro(1)"], scene.Tells);
        scene.HideIntro = true;
        ClientMetaParticipantConstruct.Construct(new byte[460], Scene.Packet(0x11, "Al"), scene.Hooks());
        Assert.Equal(2, scene.Intros);
        Assert.Single(scene.Tells);
        ClientMetaParticipantConstruct.Construct(new byte[460], Scene.Packet(0x22, "Al"), scene.Hooks());
        Assert.Equal(2, scene.Intros);
    }

    /// <summary>짧은 패킷·NUL 없는 길드는 명시 거부한다. 0.961에서 추가됐다.</summary>
    [Fact]
    public void ShortPacketsAndBadGuildThrow()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantConstruct.Construct(new byte[460], new byte[4], scene.Hooks()));
        var hooks = scene.Hooks();
        hooks.Guild = (_, destination) => destination.Span.Fill(0x41);
        Assert.Throws<ArgumentOutOfRangeException>(() => ClientMetaParticipantConstruct.Construct(new byte[460], Scene.Packet(0x11, "Al"), hooks));
    }
}
