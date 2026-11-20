using System.Buffers.Binary;
using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Tests;

// 원본의 밀어내기·섬 정리 탐색과 모든 효과 인자를 불변 PE 관측에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>세 PE 각각1178행에서 실제 탐색기·지도·물리 슬롯을 사용한다.</summary>
public sealed class X86RawConstructionEffectsTests
{
    /// <summary>경로 TYPE 조회의 독립 응답 토큰이다.</summary>
    private const uint PathToken = 0x12345678;

    /// <summary>원본 실행기의 명시 입력과 하위 효과 기록만 공급한다.</summary>
    private sealed class Scene
    {
        /// <summary>후보의 같은 실제 물리 슬롯이다.</summary>
        public readonly RawSidPool Pool;
        /// <summary>네 단계의 실제 탐색 해시다.</summary>
        public readonly RawSquidHash Hash = new();
        /// <summary>현재genus·발자국 표다.</summary>
        public readonly OriginalTypeRecord[] Types;
        /// <summary>현재 점유·통행 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>현재 전역·관계 표의 입력이다.</summary>
        public readonly ConstructionEffectsState State = new();
        /// <summary>한 행의15개DWORD 입력이다.</summary>
        public uint[] Input = [];
        /// <summary>호출 순서대로 기록한 효과다.</summary>
        public readonly List<string> Events = [];
        /// <summary>원문 판본의 상태다.</summary>
        private bool Patch => Pool.Edition == OriginalEdition.Patch1078;

        /// <summary>한 판본의 실제 풀·해시·표를 마련한다.</summary>
        public Scene(OriginalEdition edition)
        {
            Pool = new(24000, true, null, edition);
            Types = Enumerable.Range(0, edition == OriginalEdition.Patch1078 ? 188 : 171).Select(_ => new OriginalTypeRecord()).ToArray();
        }
        /// <summary>기대 결과를 계산하지 않고 행의 지도·TYPE·관계 입력을 공급한다.</summary>
        public void Prepare(string text)
        {
            Input = text.Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(15, Input.Length);
            Hash.Reset(); Events.Clear();
            // C++ 영초기화 TYPE 표와 같은 발자국0·genus0 입력을 매행 다시 넣는다.
            foreach (OriginalTypeRecord type in Types) { type.FootX = 0; type.FootY = 0; type.Flags2 = 0; }
            Types[Input[0]].FootX = (int)Input[3]; Types[Input[0]].FootY = (int)Input[4];
            Types[94].FootX = (int)Input[3]; Types[94].FootY = (int)Input[4];
            State.Blocked = Input[7] != 0; State.Server = Input[8] != 0; Array.Fill(State.Alliances, Input[12]);
            uint profile = Input[9]; Array.Fill(Spots, profile is >= 7 and <= 13 ? (byte)0 : (byte)profile);
            if (profile is >= 7 and <= 13)
            {
                (int X, int Y)[] positions = [(20, 22), (19, 22), (20, 24), (21, 21), (23, 21), (19, 23), (1, 2)];
                (int x, int y) = positions[profile - 7]; Spots[y * 256 + x] = 2;
            }
        }
        /// <summary>명시 후보를 같은 단계의 해시 머리에 등록한다.</summary>
        public void Node(string text)
        {
            uint[] node = text.Split(',').Select(X86Fixture.UInt).ToArray(); Assert.Equal(9, node.Length);
            int sid = (int)node[0]; Span<byte> raw = Pool.RawSlot(sid); raw.Clear();
            Put(sid, 0, 0x11010000); raw[10] = (byte)node[1]; raw[Patch ? 40 : 35] = (byte)node[2];
            Put(sid, 14, node[3]); Put(sid, 18, node[4]); raw[Patch ? 34 : 32] = (byte)node[5];
            if (Patch) Put(sid, 36, node[6]); else raw[34] = (byte)node[6];
            Types[node[1]].Flags2 = node[8];
            if (node[1] != 94) Types[node[1]].FootX = Types[node[1]].FootY = 1;
            ref ushort head = ref Hash.Bucket((int)node[7], Float(node[3]), Float(node[4]));
            BinaryPrimitives.WriteUInt16LittleEndian(raw.Slice(4, 2), head); head = (ushort)sid;
        }
        /// <summary>관측 실행기의 물리 입력DWORD만 덮어쓴다.</summary>
        private void Put(int sid, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(Pool.RawSlot(sid).Slice(offset, 4), value);
        /// <summary>원문 십진수DWORD 인자를 순서대로 기록한다.</summary>
        private void Event(string label, params uint[] values) => Events.Add(label + string.Concat(values.Select(value => ":" + value.ToString(CultureInfo.InvariantCulture))));
        /// <summary>실제 하위의 선택 인자와 경계 뒤 변동만 공급한다.</summary>
        public ConstructionEffectsHooks Hooks() => new()
        {
            PathType = type => { Event("R", type); return PathToken; },
            ClearPath = (sid, type) => Event("C", (uint)sid, type, 0, 1),
            Unpop = (sid, flags) => Event("U", (uint)sid, flags),
            Pop = (sid, x, y, flags) => Event("P", (uint)sid, Bits(x), Bits(y), flags),
            Fall = sid => { Event("F", (uint)sid); if (Input[14] == 1) { Put(sid, 14, Bits(1.25f)); Put(sid, 18, Bits(2.5f)); } },
            Sound = (x, y) => Event("S", Bits(x), Bits(y), 0),
            Destroy = (sid, flags) => { Event("K", (uint)sid, flags); if (Input[14] == 2) Types[Pool.Slot(sid)[10]].Flags2 = 0x10000; },
            Notify = sid => Event("N", (uint)sid),
            Broadcast = (x, y, flags, player) => { Event("I"); Event("B", x, y, flags, player); },
            Assertion = (expression, line) =>
            {
                Assert.Equal(line == 0x7b ? "noIslandTypeCount == 9" : line == 0x7c ? "islandTypeCount == 1" : "islandStalagTypeCount == 1", expression);
                Event("E", (uint)line);
            },
            Scan = area => Event("A", unchecked((uint)area.Left), unchecked((uint)area.Top), unchecked((uint)area.Right), unchecked((uint)area.Bottom)),
        };
        /// <summary>같은 현재 월드·입력으로 본체를 만든다.</summary>
        public RawConstructionEffects Make(ConstructionEffectsHooks? hooks = null) => new(Pool, Hash, Types, Spots, State, hooks ?? Hooks());
    }
    /// <summary>원문float 입력 비트를 복원한다.</summary>
    private static float Float(uint value) => BitConverter.UInt32BitsToSingle(value);
    /// <summary>호출 좌표를원문DWORD 비트로 기록한다.</summary>
    private static uint Bits(float value) => BitConverter.SingleToUInt32Bits(value);

    /// <summary>독립1178행씩 반환·탐색 범위·모든 효과 순서/인자를 실제 탐색기로 대조한다.</summary>
    [Theory]
    [InlineData("originals")]
    [InlineData("originalCD")]
    [InlineData("original1037")]
    public void Original1178(string edition)
    {
        string[][] all = X86Fixture.Read("constructioneffects"); Assert.Equal(3534, all.Length);
        string[][] rows = X86Fixture.Section(all, edition); Assert.Equal(1178, rows.Length);
        var scene = new Scene(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
        X86Fixture.CheckRows(rows, row =>
        {
            scene.Prepare(row[2]);
            if (row[3] != "-")
            {
                // 원문의 후보 입력 순서대로 해시 체인에 넣는다.
                foreach (string node in row[3].Split(';')) scene.Node(node);
            }
            uint[] input = scene.Input; bool result = true; RawConstructionEffects effects = scene.Make();
            if (row[1] == "Displace") result = effects.Displace(Float(input[1]), Float(input[2]), input[0], input[5]);
            else { Assert.Equal("Drop", row[1]); effects.DropIsland(Float(input[1]), Float(input[2]), input[6], input[5]); }
            if (row[4] != "-") Assert.Equal(X86Fixture.UInt(row[4]) != 0, result);
            Assert.Equal(row[5], scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events)); return true;
        });
    }

    /// <summary>원본Dude의foot0을허용하고음수발자국은첫효과전에거부한다.</summary>
    [Fact]
    public void ZeroFootprintAndInvalidInput()
    {
        var scene = new Scene(OriginalEdition.Patch1078); scene.Prepare("70,1101004800,1101529088,0,0,3,0,0,1,2,0,3,0,0,0");
        Assert.True(scene.Make().Displace(20, 21, 70, 3)); Assert.Equal("A:20:21:21:22", Assert.Single(scene.Events));
        scene.Events.Clear(); scene.Types[70].FootX = -1;
        Assert.Throws<ArgumentOutOfRangeException>(() => scene.Make().Displace(20, 21, 70, 3)); Assert.Empty(scene.Events);
        scene.State.Blocked = true; scene.Make().DropIsland(float.NaN, 21, 0, 3); Assert.Empty(scene.Events);
    }

    /// <summary>서버현재값은독립boss와무관하며진입억제뒤에만방송을실행한다.</summary>
    [Fact]
    public void SharedGlobalsControlBroadcastBeforeValidation()
    {
        var scene = new Scene(OriginalEdition.Patch1078); scene.Prepare("94,0,0,1,1,1,0,0,0,2,0,1,0,0,0");
        bool server = false; uint blocked = 0; scene.State.SharedServer = () => server; scene.State.SharedBlocked = () => blocked;
        RawConstructionEffects effects = scene.Make(); effects.DropIsland(20, 21, 0, 1);
        Assert.Equal("A:20:21:20:21", Assert.Single(scene.Events));
        scene.Events.Clear(); server = true; effects.DropIsland(20, 21, 0, 1);
        Assert.Equal("I;B:1310720:1376256:0:1;A:20:21:20:21", string.Join(';', scene.Events));
        scene.Events.Clear(); blocked = 0x80000000; effects.DropIsland(float.NaN, 21, 0, 1); Assert.Empty(scene.Events);
    }

    /// <summary>방송효과뒤현재섬TYPE·발자국을읽고삭제뒤현재genus를통지한다.</summary>
    [Fact]
    public void ReadsCurrentInputsAfterEffects()
    {
        var scene = new Scene(OriginalEdition.Patch1078); scene.Prepare("100,0,0,3,3,3,0,0,1,2,0,3,0,0,0");
        scene.Node($"50,157,0,{Bits(20)},{Bits(21)},3,0,0,0");
        ConstructionEffectsHooks hooks = scene.Hooks();
        hooks.Broadcast = (_, _, _, _) => scene.Types[94].FootX = 4;
        hooks.Fall = sid => { BinaryPrimitives.WriteSingleLittleEndian(scene.Pool.RawSlot(sid).Slice(14, 4), 1.25f); BinaryPrimitives.WriteSingleLittleEndian(scene.Pool.RawSlot(sid).Slice(18, 4), 2.5f); };
        hooks.Sound = (x, y) => { Assert.Equal(1.25f, x); Assert.Equal(2.5f, y); scene.Events.Add("sound"); };
        hooks.Destroy = (sid, flags) => { Assert.Equal(0x48u, flags); scene.Types[scene.Pool.Slot(sid)[10]].Flags2 = 0x10000; };
        hooks.Notify = sid => { Assert.Equal(50, sid); scene.Events.Add("notify"); };
        scene.Make(hooks).DropIsland(20, 21, 0, 3);
        Assert.Equal("A:17:19:20:21", scene.Events[0]); Assert.Contains("sound", scene.Events); Assert.Contains("notify", scene.Events);
    }
}
