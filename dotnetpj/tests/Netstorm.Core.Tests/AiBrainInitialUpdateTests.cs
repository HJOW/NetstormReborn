using System.Buffers.Binary;
using System.Text;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;

// 실제 AI초기갱신·기술·내용물의 원본 전체 출력을 대조한다.
namespace Netstorm.Tests;

/// <summary>cpppj/tests/AiBrainInitialUpdateTests.cpp의133입력·theme5추가16입력 검사다.</summary>
public sealed class AiBrainInitialUpdateTests
{
    /// <summary>선택한 원문 정수 폭을 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint result = 0;
        // 낮은 바이트부터 필요한 폭만 결합한다.
        for (int index = 0; index < width; index++) result |= (uint)raw[offset + index] << (index * 8);
        return result;
    }
    /// <summary>선택한 원문 정수 폭만 쓰고 나머지를 보존한다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value, int width = 4)
    {
        // 선택한 낮은 바이트만 쓴다.
        for (int index = 0; index < width; index++) raw[offset + index] = unchecked((byte)(value >> (index * 8)));
    }
    /// <summary>실제 전체바이트를 원본과 같은 소문자16진수로 쓴다.</summary>
    private static string Hex(ReadOnlySpan<byte> raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>기대 분류를 계산하지 않고 원본과 같은 내용물 타입 입력을 구성한다.</summary>
    private static uint[] Nodes(int pattern) => pattern switch
    {
        0 => [], 1 => [1, 2, 3, 4, 5, 133], 2 => [2], 3 => [2, 3],
        4 => Enumerable.Repeat(1u, 6).ToArray(),
        5 => Enumerable.Repeat(1u, 16).Append(4u).ToArray(), 6 => [1, 2, 3, 4], _ => [5, 1],
    };

    /// <summary>현재기술·Factory·내용물·AI메모리를 같은 한 장면에서 소유한다.</summary>
    private sealed class Scene
    {
        /// <summary>독립 입력7개다.</summary>
        public readonly long[] Input;
        /// <summary>모든 부모가 공유하는 실제PATCH풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);
        /// <summary>현재188TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>TYPE+a0의실제 별도 문자열이다.</summary>
        private readonly string[] _energy = Enumerable.Repeat("", 188).ToArray();
        /// <summary>같은 실제9Player원문이다.</summary>
        public readonly byte[] Records = new byte[1620];
        /// <summary>실제현재PlayerKnown/owner전역이다.</summary>
        private readonly PlayerWorldState _state = new() { PlayerKnown = 1, BattleOwner = 1 };
        /// <summary>실제기술부모조회·생성 부모다.</summary>
        public readonly RawPlayerWorld Player;
        /// <summary>실제내용물타입6·권한전역이다.</summary>
        public readonly ContainedFinderState FinderState = new();
        /// <summary>실제40개목록·Ensure기술처리다.</summary>
        public readonly RawPlayerTechnology Technology;
        /// <summary>생성자와 같은 실제메모리소유자다.</summary>
        public readonly RawAiBrainMemory Memory;
        /// <summary>동일자료의실제초기갱신기다.</summary>
        public readonly RawAiBrainInitialUpdate Runtime;
        /// <summary>현재미션모드다.</summary>
        public uint Mission;
        /// <summary>현재분류제외타입이다.</summary>
        public uint Excluded = 133;
        /// <summary>실제004709d0의현재워크샾표다.</summary>
        public readonly uint[] Workshops = [86, 87, 88, 85];
        /// <summary>실제본체토큰이다.</summary>
        public readonly uint Token;
        /// <summary>실제40바이트빌더목록토큰이다.</summary>
        public readonly uint Builders;
        /// <summary>독립600바이트설정입력이다.</summary>
        public readonly byte[] Profile = new byte[600];
        /// <summary>실제언어·타입·생성·부착요청순서다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>실제새Form할당순서다.</summary>
        private readonly List<int> _born = [];

        /// <summary>원본 하네스의 입력만 같은raw저장소에 준비한다.</summary>
        public Scene(string text)
        {
            Input = text.Split(',').Select(X86Fixture.Long).ToArray();
            Assert.Equal(7, Input.Length);
            Mission = (uint)Input[1];
            uint[] nodes = Nodes((int)Input[0]);
            Put(Records, 180 + 0x9c, Input[4] != 0 ? 50u : 0u);
            Encoding.ASCII.GetBytes("Old").CopyTo(Records, 180 + 12);
            // 보호PE의내장Form생성자주소만명시입력으로제공한다.
            foreach (int type in new[] { 1, 2, 3, 6 }) Types[type].ConstructorAddress = OriginalConstructors.Address(OriginalEdition.Patch1078, type);
            // 원본처럼5번풀할당머리는보존하고부모·내용물의명시입력영역만지운다.
            foreach (int sid in new[] { 50, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76 }) Pool.RawSlot(sid).Clear();
            if (Input[4] != 0) Put(Pool.RawSlot(50), 6, nodes.Length == 0 ? 0u : 60u, 2);
            // free/dead/비contained와flags조건을원본입력과같게섞는다.
            for (int index = 0; index < nodes.Length; index++)
            {
                Span<byte> raw = Pool.RawSlot(60 + index);
                Put(raw, 0, 0x12010000); raw[10] = Input[0] == 6 && index == 1 ? (byte)46 : (byte)6;
                raw[11] = (byte)(index % 3 == 0 ? 0 : index % 3 == 1 ? 8 : 9);
                Put(raw, 4, index + 1 < nodes.Length ? (uint)(61 + index) : 0, 2);
                Put(raw, 18, nodes[index], 2); Put(raw, 20, Input[0] == 6 && index == 2 ? 0u : 4u, 2);
            }
            // 실제메타데이터의이름·비용·원소·플래그만구성한다.
            for (int index = 0; index < Types.Length; index++) { Types[index].Name = "T" + index; Types[index].Theme = 0; }
            // 현재타입표에독립속성입력한줄을기록한다.
            void Metadata(int type, int theme, uint flags, float cost, string energy)
            { Types[type].Theme = theme; Types[type].Flags2 = flags; Types[type].Cost = cost; _energy[type] = energy; }
            Metadata(1, 0, 0x10000, 150.5f, "s"); Metadata(2, 0, 0x800, 100, "ss");
            Metadata(3, 1, 0x40000, 200.75f, "wz"); Metadata(4, 2, 0x800, 800.25f, "rrrr");
            Metadata(5, 3, 0, Input[0] == 7 ? BitConverter.UInt32BitsToSingle(0x7fc00000) : -1, "");
            Metadata(133, 5, 0x10000, 99, "s");
            Types[85].Cost = 200.25f; Types[86].Cost = 150.75f; Types[87].Cost = 500.5f; Types[88].Cost = 300;
            var post = new RawPostPop(Pool, Types, new RawPostPopState());
            var factory = new RawSquidFactory(Pool, Types);
            Player = new RawPlayerWorld(Pool, post, Records, _state, new PlayerWorldHooks
            {
                Assertion = _ => throw new InvalidOperationException("선택하지않은Player진단"),
                Find = (_, _, _) => 0,
                Create = (type, flags) =>
                {
                    int sid = factory.Create(type, (SidAllocation)flags);
                    Put(Pool.AllocatedBytes(sid), 6, nodes.Length == 0 ? 0u : 60u, 2); return sid;
                },
                SetOwner = (sid, owner) => Pool.AllocatedBytes(sid)[34] = (byte)owner,
                Pop = (sid, _, _, _) =>
                {
                    byte owner = Pool.Slot(sid)[34]; Put(Records, owner * 180 + 0x9c, (uint)sid); Trace.Add("J," + owner);
                },
            });
            var finder = new RawCarrierPreDestroy(Pool, FinderState, new CarrierPreDestroyHooks
            {
                ValidateDamageable = (_, _) => throw new InvalidOperationException("선택하지않은피해검증"),
                DamageablePre = (_, _) => throw new InvalidOperationException("선택하지않은피해후처리"),
                DependentFound = () => throw new InvalidOperationException("선택하지않은종속통지"),
            });
            var create = new RawContainedCreate(Pool, new ContainedCreateState(), new ContainedCreateHooks
            {
                Create = (type, flags) =>
                {
                    Trace.Add($"C,{type},{flags}"); Assert.Equal(6u, type);
                    int sid = factory.CreateContained(flags); _born.Add(sid); return sid;
                },
                NotifySurface = _ => throw new InvalidOperationException("선택하지않은표면통지"),
                Attach = (sid, target, flags) =>
                {
                    Trace.Add($"B,{sid},{target},{flags}"); Put(Pool.AllocatedBytes(sid), 4, Read(Pool.Slot(target), 6, 2), 2);
                    Put(Pool.RawSlot(target), 6, (uint)sid, 2);
                },
            });
            Technology = new RawPlayerTechnology(Player, finder, create, Types, new PlayerTechnologyHooks
            {
                FindType = name =>
                {
                    Trace.Add("F," + Hex(Encoding.Latin1.GetBytes(name)));
                    // 원본의명시입력앞48개이름만찾는다.
                    for (int index = 0; index < 48; index++) if (name == Types[index].Name) return index;
                    return -1;
                },
                Assertion = _ => throw new InvalidOperationException("선택하지않은기술진단"),
                Destroy = (_, _) => throw new InvalidOperationException("선택하지않은기술삭제"),
            });
            int allocation = 0;
            Memory = new RawAiBrainMemory(0x13000000, raw =>
            {
                // 본체는독립패턴입력이고후속실제목록은0입력이다.
                for (int index = 0; index < raw.Length; index++) raw[index] = allocation == 0 ? unchecked((byte)(index * 29 + Input[6])) : (byte)0;
                allocation++;
            });
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount); Builders = Memory.Allocate(40);
            byte[] body = Memory.Bytes(Token);
            // 실제본체의명시입력필드만원문과같은순서로쓴다.
            foreach ((int offset, uint value) in new (int, uint)[]
            {
                (0x260, 1), (0x284, Builders), (0x288, 10), (0x28c, (uint)Input[5]), (0x3b0, (uint)Input[5]),
                (0x394, Input[5] != 0 ? 85u : 0u), (0x390, 3), (0xad8, Input[5] != 0 ? 7u : 2u), (0x2d8, 999), (0x290, 0),
            }) Put(body, offset, value);
            if (Input[5] != 0)
            { Put(Memory.Bytes(Builders), 0, 99); Put(Memory.Bytes(Builders), 4, 98); Put(body, 0x398, 1); Put(body, 0x39c, 3); }
            // 같은600바이트입력에서이름·기술·시작금액만명시한다.
            for (int index = 0; index < Profile.Length; index++) Profile[index] = unchecked((byte)(index * 37 + Input[6]));
            if (Input[2] != 0) { Profile[0] = (byte)'A'; Profile[1] = (byte)'I'; Profile[2] = 0; } else Profile[0] = 0;
            byte[] tech = Encoding.Latin1.GetBytes(Input[3] != 0 ? "T1;T2" : "");
            tech.CopyTo(Profile, 30); Profile[30 + tech.Length] = 0; Put(Profile, 0x22c, 777);
            Runtime = new RawAiBrainInitialUpdate(Memory, Player, Technology, FinderState, Types, Globals(), Hooks());
        }

        /// <summary>각사용위치에서같은장기전역을조회한다.</summary>
        public AiBrainInitialUpdateGlobals Globals() => new() { MissionMode = () => Mission, ExcludedType = () => Excluded, Workshops = Workshops };
        /// <summary>원본외부언어·요구에너지요청만순서대로기록한다.</summary>
        public AiBrainInitialUpdateHooks Hooks(bool energy = true) => new()
        {
            DefaultName = () => { Trace.Add("D"); return "Default AI"; },
            RequiredEnergy = energy ? type => { Trace.Add("E," + type); return _energy[type]; } : null,
        };
        /// <summary>실제새Form전체를할당순서로읽는다.</summary>
        public string Born() => _born.Count == 0 ? "-" : string.Join(':', _born.Select(sid => Hex(Pool.Slot(sid))));
        /// <summary>실제본문·빌더40바이트·Player전체·새Form·조회순서를원본출력과비교한다.</summary>
        public bool Matches(string[] row, int offset) => Hex(Memory.Bytes(Token)) == row[offset]
            && Hex(Memory.Bytes(Builders)) == row[offset + 1] && Hex(Records) == row[offset + 2]
            && Born() == row[offset + 3] && (Trace.Count == 0 ? "-" : string.Join(';', Trace)) == row[offset + 4];
    }

    /// <summary>원본133고유입력의전체바이트·실제내용물생성·모든효과를대조하고지갑보존을확인한다.</summary>
    [Fact]
    public void Native133()
    {
        string[][] rows = X86Fixture.Read("ai-initial-update").Where(row => row[0] != "input").ToArray();
        Assert.Equal(133, rows.Length); Assert.Equal(133, rows.Select(row => row[0]).Distinct().Count());
        X86Fixture.CheckRows(rows, row =>
        {
            var scene = new Scene(row[0]); scene.Runtime.Update(scene.Token, scene.Profile);
            return scene.Matches(row, 1) && Read(scene.Memory.Bytes(scene.Token), 0x2d8) == 999;
        });
    }

    /// <summary>선택되지않은조회는요구하지않으며필수에너지조회실패를이미수행한빌더등록뒤에보고한다.</summary>
    [Fact]
    public void MissingSelectedMetadataAndWalletPreserved()
    {
        var empty = new Scene("0,0,0,0,1,0,19");
        var inactive = new RawAiBrainInitialUpdate(empty.Memory, empty.Player, empty.Technology, empty.FinderState, empty.Types, empty.Globals(), new AiBrainInitialUpdateHooks());
        inactive.Update(empty.Token, empty.Profile);
        Assert.Equal(999u, Read(empty.Memory.Bytes(empty.Token), 0x2d8)); Assert.Equal(777u, Read(empty.Memory.Bytes(empty.Token), 0x22c));
        var missing = new Scene("1,1,1,0,1,0,19");
        var incomplete = new RawAiBrainInitialUpdate(missing.Memory, missing.Player, missing.Technology, missing.FinderState, missing.Types, missing.Globals(), missing.Hooks(false));
        Assert.Throws<InvalidOperationException>(() => incomplete.Update(missing.Token, missing.Profile));
        Assert.Equal(1u, Read(missing.Memory.Bytes(missing.Token), 0x28c)); Assert.Equal(999u, Read(missing.Memory.Bytes(missing.Token), 0x2d8));
    }

    /// <summary>theme5와sun battery의공유stack개수·후속중립화를원본16입력과대조한다.</summary>
    [Fact]
    public void NativeTheme5AndSharedCounter()
    {
        string[][] rows = X86Fixture.Read("ai-initial-update-theme5"); Assert.Equal(16, rows.Length);
        X86Fixture.CheckRows(rows, row =>
        {
            var scene = new Scene(row[1]); scene.Types[1].Theme = X86Fixture.Int(row[0]);
            scene.Runtime.Update(scene.Token, scene.Profile); return scene.Matches(row, 2);
        });
    }
}
