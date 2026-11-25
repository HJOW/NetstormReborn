using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Display;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Client.Tests;

// R4-7b Player 월드 1,680행 대조다. cpppj PlayerWorldTests의 PlayerReplayScene과 같은 실제 입력으로
// 같은 Factory/Pop/Owner/ProcessForm/Kernel 호스트를 연결하고 실행 뒤 전체 9기록·슬롯·장부·공간·Form·Kernel을 비교한다.
// CD 판본 840행은 DN 범위(10.78 전용) 밖이라 originals 840행만 대조한다. 0.998에서 추가됐다.
public sealed class PlayerWorldReplayTests
{
    /// <summary>실제 게임 타입·셰이프와 같은 장기 장부를 소유한다.</summary>
    private sealed class Assets
    {
        /// <summary>같은 실제 188 타입 표다.</summary>
        public readonly IReadOnlyList<OriginalTypeRecord> Types;
        /// <summary>같은 실제 표시 셰이프다.</summary>
        public readonly List<SquidDisplayShape> Shapes;
        /// <summary>실제 파일에서 타입·셰이프를 읽는다. 판본 188개를 요구한다.</summary>
        public Assets()
        {
            string? directory = GameDataLocator.FindDataDirectory();
            Assert.NotNull(directory);
            var files = GameFileSystem.Open(directory!);
            Types = MissionFortLoad.LoadTypeTable(files).Types;
            Assert.Equal(188, Types.Count);
            IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(files);
            byte[] shpRaw = files.ReadAllBytes("d/_shapes.shp");
            Shapes = TerrainDisplayShapes.Build(definitions, new ShapeDatabase(shpRaw));
            Assert.Equal(Types.Count, Shapes.Count);
        }
    }

    /// <summary>같은 원본 표시 변경 표를 빌리는 실제 저장소 없는 그리기다.</summary>
    private sealed class NullSink : SquidDisplaySink
    {
        /// <summary>다시 그리기가 필요 없다고 본다.</summary>
        public override bool Suppressed() => false;
        /// <summary>표시 영역 변경을 버린다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags)
        {
        }
    }

    /// <summary>원본 unsigned 십진 DWORD 목록이다. 시각(10번째)은 ulong으로 따로 읽는다.</summary>
    private static uint[] Numbers(string value) => value.Split(',').Take(9).Select(part => uint.Parse(part, CultureInfo.InvariantCulture)).ToArray();

    /// <summary>원본 시각 64비트를 double 비트로 읽는다.</summary>
    private static double NowBits(string value) => BitConverter.UInt64BitsToDouble(ulong.Parse(value.Split(',')[9], CultureInfo.InvariantCulture));

    /// <summary>같은 원본 미복원 후속 효과를 명시 실패로 둔다.</summary>
    private static void Missing(string name) => throw new InvalidOperationException("Player 선택 범위 밖의 실제 후속 효과 미연결: " + name);

    /// <summary>같은 실제 장기 장치를 빌리며 어떤 상태도 초기화하지 않는다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 풀이다. 서버 저장소에서 SID 5부터 배정한다.</summary>
        public readonly RawSidPool Pool = new(32768, true, null, OriginalEdition.Patch1078);
        /// <summary>같은 Pop/finder의 실제 머리 목록이다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>현재 빈 원본 점유 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>같은 실제 비용/세 표/목록/깊이다.</summary>
        public readonly RawPostPopState Book = new();
        /// <summary>같은 실제 장식 사용 표시 저장소다.</summary>
        public readonly NullSink Sink = new();
        /// <summary>같은 실제 표시 계산기다.</summary>
        public readonly SquidDisplay Display;
        /// <summary>동일 실제 공간 해제 몸체다.</summary>
        public readonly RawUnpop Unpop;
        /// <summary>같은 실제 장부 후처리 몸체다.</summary>
        public readonly RawPostPop Post;
        /// <summary>같은 실제 첫 Pop 몸체다.</summary>
        public readonly RawPop Pop;
        /// <summary>같은 실제 팩토리 몸체다.</summary>
        public readonly RawSquidFactory Factory;
        /// <summary>같은 실제 소유자 모드다.</summary>
        public readonly SquidOwnerMode Mode = new();
        /// <summary>같은 실제 소유자 몸체다.</summary>
        public readonly SquidOwner Owner;
        /// <summary>같은 Pool/Hash/spot을 직접 조회한다.</summary>
        public readonly RawSquidNeighbors Neighbors;
        /// <summary>단일 타입 검색의 현재 전역 계약이다.</summary>
        public readonly BridgeConnectState Bridge = new();
        /// <summary>같은 실제 전체 타입 검색 몸체다.</summary>
        public readonly RawBridgeConnect Finder;
        /// <summary>같은 실제 ProcessForm 수명 몸체다.</summary>
        public readonly RawSquidDestroy Destroy;
        /// <summary>같은 실제 프로세스 슬롯 소유자다.</summary>
        public readonly Kernel Kernel = new(KernelShape.ProcessCapacity);
        /// <summary>같은 실제 현재 시각이다.</summary>
        public readonly SquidProcessState Clock = new();
        /// <summary>같은 실제 Host 몸체다.</summary>
        public readonly SquidProcessHost Host;
        /// <summary>현재 9개 Player 바이트다.</summary>
        public readonly byte[] Records = new byte[9 * 0xb4];
        /// <summary>현재 알려진 소유자·fort·special이다.</summary>
        public readonly PlayerWorldState State = new();
        /// <summary>진단/힙 요청/actual attach를 같은 순서로 기록한다.</summary>
        public readonly List<string> Events = [];
        /// <summary>실제 생성된 Host 소유 프로세스다.</summary>
        public RegularProcess? Regular;
        /// <summary>같은 실제 Player 본체다.</summary>
        public readonly RawPlayerWorld Player;

        /// <summary>같은 실제 장기 장치와 현재 입력을 연결한다. 기대값은 실행 뒤에만 비교한다.</summary>
        public Scene(Assets assets, string[] row)
        {
            uint[] input = Numbers(row[1]);
            Assert.Equal(9, input.Length);
            Display = new(OriginalEdition.Patch1078, assets.Types, assets.Shapes, Sink, new() { CameraX = 0, CameraY = 0, Zoom = 65536, Viewport = new DisplayRect(0, 0, 1024, 768) });
            Unpop = new(Pool, Hash, Spots, Display);
            Post = new(Pool, assets.Types, Book);
            Pop = new(Pool, Hash, Spots, Display, Post);
            Factory = new(Pool, assets.Types, false, Unpop);
            Owner = new(Pool, assets.Types, Book, Mode);
            Neighbors = new(Pool, Hash, Spots, assets.Types, new RiftFrameCodeTables(Enumerable.Repeat(new RiftTypeFrames([]), assets.Types.Count).ToList()));
            Finder = new(Pool, Neighbors, Bridge, new()
            {
                Create = (_, _) => { Missing("create"); return 0; },
                SetOwner = (_, _) => Missing("setOwner"),
                Pop = (_, _, _, _) => Missing("pop"),
                SetFrame = (_, _, _) => Missing("setFrame"),
                NotifySurface = (_) => Missing("notify"),
            });
            Destroy = new(Pool, Unpop, assets.Types);
            Host = new(Pool, assets.Types, Kernel, Destroy, Clock, (_, _, _, _) => throw new InvalidOperationException("Player 선택 범위 밖의 실제 후속 효과 미연결"));
            State.PlayerKnown = input[4];
            State.BattleOwner = input[2];
            State.TemplateOwner = input[3];
            State.PlayerType = 152;
            State.FortMode = input[5] != 0;
            State.Special = input[6];
            Mode.Fort = State.FortMode;
            Mode.Battle = !State.FortMode;
            Book.LocalOwner = unchecked((byte)State.BattleOwner);
            Clock.Now = NowBits(row[1]);
            for (uint owner = 0; owner < 9; owner++)
            {
                Put(Records, unchecked((int)(owner * 0xb4)), 0x12003000u + owner * 0x100u);
                Put(Records, unchecked((int)(owner * 0xb4 + 0x9c)), 0xfedc0000u + owner);
            }
            uint[][] tables = [Book.LocalCounts, Book.LocalSecondaryCounts, Book.GlobalCounts];
            for (int table = 0; table < tables.Length; table++)
                for (int index = 0; index < tables[table].Length; index++)
                    tables[table][index] = unchecked((uint)index * 2654435761u + (uint)table * 17u);
            var hooks = new PlayerWorldHooks
            {
                Assertion = assertion => Events.Add("A:" + (assertion == PlayerWorldAssertion.PlayerAlreadyExists ? "852" : input[0] == 3 ? "669" : "849")),
                Find = (x, y, type) => Finder.FindTypeAt(x, y, type),
                Create = (type, flags) => Factory.Create(type, (SidAllocation)flags),
                SetOwner = (sid, value) => Owner.Set(sid, value),
                Pop = (sid, x, y, flags) =>
                {
                    float[] size = Display.FrameSize(OriginalEdition.Patch1078, Pool.Slot(sid), 0);
                    if (Pop.Pop(sid, assets.Types[152], size[0], size[1], x, y, flags) != RawPopResult.Registered) throw new InvalidOperationException("Player 실제 첫 공간 Pop 실패");
                },
                ReserveRegular = () => { Events.Add("N:40"); return input[7] != 0; },
                AddRegular = (sid, ev, payload) => { Regular = Host.AddRegular(sid, ev, payload); Events.Add("B:46:" + sid + ":0:80"); },
            };
            Player = new(Pool, Post, Records, State, hooks);
            Post.SetPlayerPostPop(Player);
        }

        /// <summary>현재 DWORD를 리틀 엔디언으로 쓴다.</summary>
        private static void Put(byte[] bytes, int at, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(at, 4), value);

        /// <summary>네 실제 몸체를 실행한 뒤 전체 9기록·슬롯·장부·공간·Form·Kernel을 원문 열 순서로 직렬화한다.</summary>
        public string[] Run(uint kind, uint[] input)
        {
            int sid = 5;
            uint result = 0;
            int assertions = 0;
            if (kind == 0) { sid = Player.Create(unchecked((int)input[1])); result = unchecked((uint)sid); }
            else if (kind == 1)
            {
                sid = Factory.Create(152, (SidAllocation)2);
                Pool.AllocatedBytes(sid)[34] = unchecked((byte)input[1]);
                Book.Depth = 1;
                Player.PostPop(sid, input[8]);
            }
            else result = kind == 2 ? Player.CurrentTechnology() : Player.Technology(unchecked((int)input[1]));
            string actions = "";
            foreach (string line in Events)
            {
                if (actions.Length != 0) actions += ';';
                actions += line;
                if (line.StartsWith("A:", StringComparison.Ordinal)) assertions++;
            }
            string counts = "0,0";
            foreach (RawPostPopList list in Book.OwnerFactories) counts += "," + list.Count.ToString(CultureInfo.InvariantCulture);
            string active = "";
            for (uint pid = 1; pid <= Kernel.Capacity(); pid++)
                if (Kernel.Get(pid) is not null)
                {
                    if (active.Length != 0) active += ',';
                    active += pid.ToString(CultureInfo.InvariantCulture);
                }
            string fields = "-";
            if (Regular is not null) fields = BitConverter.DoubleToUInt64Bits(Regular.Time()).ToString(CultureInfo.InvariantCulture) + ',' + Regular.Event().ToString(CultureInfo.InvariantCulture) + ',' + BitConverter.SingleToUInt32Bits(Regular.Payload()).ToString(CultureInfo.InvariantCulture) + ',' + Regular.Count().ToString(CultureInfo.InvariantCulture);
            int form = Regular?.Form() ?? 0;
            var heads = new List<ushort>();
            for (int level = 0; level < 4; level++) heads.AddRange(Hash.Entries(level).ToArray());
            return [result.ToString(CultureInfo.InvariantCulture), actions.Length == 0 ? "-" : actions, Hex(Records), Hex(Pool.Slot(sid)), WordAdler(heads).ToString(CultureInfo.InvariantCulture), X86Fixture.Adler32(Spots).ToString(CultureInfo.InvariantCulture),
                Book.TotalCost.ToString(CultureInfo.InvariantCulture), Book.ProductionDirty.ToString(CultureInfo.InvariantCulture), Book.Depth.ToString(CultureInfo.InvariantCulture), WordAdler(Book.LocalCounts).ToString(CultureInfo.InvariantCulture), WordAdler(Book.LocalSecondaryCounts).ToString(CultureInfo.InvariantCulture), WordAdler(Book.GlobalCounts).ToString(CultureInfo.InvariantCulture),
                counts, "1", fields, form.ToString(CultureInfo.InvariantCulture), form != 0 ? Hex(Pool.Slot(form)) : "-", active.Length == 0 ? "-" : active, Clock.TypeRotation.ToString(CultureInfo.InvariantCulture), assertions.ToString(CultureInfo.InvariantCulture)];
        }

        /// <summary>같은 실제 바이트를 원문 소문자로 직렬화한다.</summary>
        private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();

        /// <summary>원본 WORD 표를 리틀 엔디언으로 직렬화한 Adler-32다.</summary>
        private static uint WordAdler(IReadOnlyList<ushort> words)
        {
            byte[] bytes = new byte[words.Count * 2];
            for (int index = 0; index < words.Count; index++) { bytes[2 * index] = (byte)words[index]; bytes[2 * index + 1] = (byte)(words[index] >> 8); }
            return X86Fixture.Adler32(bytes);
        }
        /// <summary>원본 DWORD 표를 리틀 엔디언으로 직렬화한 Adler-32다.</summary>
        private static uint WordAdler(IReadOnlyList<uint> words)
        {
            byte[] bytes = new byte[words.Count * 4];
            for (int index = 0; index < words.Count; index++)
            {
                bytes[4 * index] = (byte)words[index];
                bytes[4 * index + 1] = (byte)(words[index] >> 8);
                bytes[4 * index + 2] = (byte)(words[index] >> 16);
                bytes[4 * index + 3] = (byte)(words[index] >> 24);
            }
            return X86Fixture.Adler32(bytes);
        }
    }

    /// <summary>원본 패치 840행을 같은 실제 재현 장면에서 실행하고 전체 열을 대조한다.</summary>
    [Fact]
    public void ReplayOriginalsRows()
    {
        var assets = new Assets();
        string[][] rows = X86Fixture.Read("player-world").Where(row => row[0] == "originals").ToArray();
        Assert.Equal(840, rows.Length);
        var failures = new List<string>();
        int count = 0;
        foreach (string[] row in rows)
        {
            Assert.True(row.Length == 22, "행 열 수: " + row.Length);
            var scene = new Scene(assets, row);
            uint[] input = Numbers(row[1]);
            string[] output = scene.Run(input[0], input);
            string[] expected = row[2..];
            if (!output.SequenceEqual(expected))
            {
                for (int index = 0; index < expected.Length && failures.Count < 5; index++)
                    if (output[index] != expected[index])
                        failures.Add($"행{count} 열{index} 기대={expected[index]} 실제={output[index]}");
                if (failures.Count >= 5) break;
            }
            count++;
        }
        Assert.True(failures.Count == 0 && count == 840, string.Join("\n", failures));
    }
}
