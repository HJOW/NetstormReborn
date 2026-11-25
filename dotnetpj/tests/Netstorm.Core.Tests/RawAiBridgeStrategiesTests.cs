// 네 원본 전략의 실제 전체 본체/목록/난수/하위 호출 순서를 독립 대조한다.
using System.Buffers.Binary;
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// AI 다리 전략을 검사하는 클래스다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.
// 근거: cpppj tests/AiBridgeStrategiesTests.cpp의 5건 전부다.
public sealed class RawAiBridgeStrategiesTests
{
    /// <summary>원문 fixture와 같은 소문자 16진 표현이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="raw">바꿀 바이트다.</param>
    /// <returns>소문자 16진이다.</returns>
    private static string Hex(byte[] raw) => Convert.ToHexString(raw).ToLowerInvariant();

    /// <summary>현재 원문 DWORD를 little endian으로 쓴다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="raw">쓸 바이트 범위다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> raw, int offset, uint value) =>
        BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(offset, 4), value);

    /// <summary>현재 원문 DWORD를 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint Get(byte[] raw, int offset) =>
        BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(offset, 4));

    /// <summary>현재 두 float를 호출 순서 비교용 원문 DWORD로 나타낸다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    /// <param name="point">바꿀 좌표다.</param>
    /// <returns>두 DWORD의 문자열이다.</returns>
    private static string XY(RawPathPoint point) => BitConverter.SingleToUInt32Bits(point.X) + "," + BitConverter.SingleToUInt32Bits(point.Y);

    /// <summary>명시 하위 입력과 원본 본문/목록/슬롯만 설치한다. 전략 결과를 미리 만들지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    private sealed class StrategyScene
    {
        /// <summary>원본 시험과 같은 opaque 값의 실제 소유 블록이다.</summary>
        public readonly RawAiBrainMemory Memory = new(0x10000000u, bytes => bytes.Fill(0xcd));

        /// <summary>실제 0xb08 본체다.</summary>
        public readonly uint Token;

        /// <summary>actor/연결/미연결/포기 목록의 실제 소유 핸들이다.</summary>
        public readonly uint[] Lists = new uint[4];

        /// <summary>실제 50BYTE 풀이다.</summary>
        public readonly RawSidPool Pool = new(32768);

        /// <summary>원본 fixture의 실제 genus/발자국 입력이다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();

        /// <summary>중심 계산과 계획 모듈이 공유할 현재 코드표다.</summary>
        public readonly RiftTypeFrames[] Frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();

        /// <summary>같은 실제 현재 해시다.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>같은 실제 지도 입력이다.</summary>
        public readonly byte[] Spots = new byte[65536];

        /// <summary>계획 모듈이 빌리는 같은 hash0 입력이다.</summary>
        public readonly ushort[] Surfaces = new ushort[65536];

        /// <summary>모든 실제 난수 계산이 공유하는 상태다.</summary>
        public readonly GameRandom Random = new();

        /// <summary>현재 QWORD 게임 시각이다.</summary>
        public double Now;

        /// <summary>현재 유효 좌표 상한이다.</summary>
        public int MapLimit = 256;

        /// <summary>현재 간헐천 로그 종류다.</summary>
        public uint LogKind = 29;

        /// <summary>관측 도구가 주입한 명시 하위 반환/전역 입력 14개다.</summary>
        public readonly List<ulong> Cfg = [];

        /// <summary>도달한 실제 하위의 순서/인자만 관측한다.</summary>
        public readonly List<string> Trace = [];

        /// <summary>현재 명시 효과 경계이며 계산의 기대값을 반환하지 않는다.</summary>
        public AiBridgeStrategyHooks Hooks = new();

        /// <summary>독립 fixture 행의 입력 바이트와 현재 하위 입력을 구성한다.</summary>
        /// <param name="row">9칸 fixture 행이다.</param>
        public StrategyScene(string[] row)
        {
            Token = Memory.Allocate(RawAiBrainConstructor.BytesCount);
            // 네 actual 소유 목록을 원본 입력과 같은 핸들 순서로 만드는 반복문이다.
            for (int index = 0; index < Lists.Length; index++)
            {
                Lists[index] = Memory.Allocate(64);
            }

            foreach (string value in row[1].Split(','))
            {
                Cfg.Add(X86Fixture.ULong(value));
            }

            byte[] body = Convert.FromHexString(row[2]);
            byte[] listBytes = Convert.FromHexString(row[3]);
            byte[] slots = Convert.FromHexString(row[4]);
            body.CopyTo(Memory.Bytes(Token), 0);
            // 각 목록의 여유 꼬리도 그대로 설치하는 반복문이다.
            for (int index = 0; index < Lists.Length; index++)
            {
                listBytes.AsSpan(index * 64, 64).CopyTo(Memory.Bytes(Lists[index]));
            }

            // 타입/생사/좌표 등 전체 슬롯 입력이다.
            for (ushort index = 0; index < 6; index++)
            {
                slots.AsSpan(index * 50, 50).CopyTo(Pool.RawSlot((ushort)(50 + index)));
            }

            Types[97].Flags2 = 0x10000000;
            Types[98].Flags2 = 0x200;
            Types[97].FootX = 3;
            Types[97].FootY = 3;
            Types[98].FootX = 3;
            Types[98].FootY = 3;
            Types[99].FootX = 1;
            Types[99].FootY = 1;
            Now = BitConverter.UInt64BitsToDouble(Cfg[0]);
            Random.SetState((uint)Cfg[1]);
            Bind();
        }

        /// <summary>각 명시 하위의 입력/반환을 관측 도구와 같이 공급한다. 목록/전략 계산은 제품이 수행한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
        public void Bind()
        {
            Hooks.Technology = owner =>
            {
                Trace.Add("technology," + owner);
                return new Sid(50);
            };
            Hooks.Refresh = (_, parent) =>
            {
                Trace.Add("refresh," + parent.Value);
                if (Cfg[13] != 0)
                {
                    Field(0x2d4, (uint)Cfg[13]);
                }
            };
            Hooks.Graph = sid =>
            {
                Trace.Add("graph," + sid.Value);
                return (uint)Cfg[4];
            };
            Hooks.Center = sid =>
            {
                Trace.Add("center," + sid.Value);
                var empty = Enumerable.Range(0, 188).Select(_ => (IReadOnlyList<FrameCode>)Array.Empty<FrameCode>()).ToArray();
                var neighbors = new RawSquidNeighbors(Pool, Hash, Spots, Types, empty);
                float[] center = neighbors.Center(sid.Value);
                return new RawPathPoint(center[0], center[1]);
            };
            Hooks.Connected = (_, sid) =>
            {
                Trace.Add("connected," + sid.Value);
                return Cfg[5] != 0;
            };
            Hooks.Advance = (_, point) =>
            {
                Trace.Add("advance," + XY(point));
                return Cfg[6] != 0;
            };
            Hooks.Home = _ =>
            {
                Trace.Add("home");
                return new RawPathPoint(101.25f, 102.75f);
            };
            Hooks.Reclassify = _ => Trace.Add("reclassify");
            Hooks.Select = (_, point, excluded, required) =>
            {
                Trace.Add("select," + XY(point) + "," + excluded + "," + required);
                return new Sid((ushort)Cfg[12]);
            };
            Hooks.RandomStart = (_, bounds) =>
            {
                Trace.Add("random," + XY(new RawPathPoint(bounds.Left, bounds.Top)) + "," + XY(new RawPathPoint(bounds.Right, bounds.Bottom)));
                return new RawPathPoint(BitConverter.UInt32BitsToSingle((uint)Cfg[8]), BitConverter.UInt32BitsToSingle((uint)Cfg[9]));
            };
            Hooks.Extend = (_, source, target) =>
            {
                Trace.Add("extend," + XY(source) + "," + XY(target));
                return new RawPathPoint(BitConverter.UInt32BitsToSingle((uint)Cfg[10]), BitConverter.UInt32BitsToSingle((uint)Cfg[11]));
            };
            Hooks.Rectangle = (left, top, right, bottom) =>
            {
                Trace.Add("rectangle," + (uint)left + "," + (uint)top + "," + (uint)right + "," + (uint)bottom);
                return Cfg[7] != 0;
            };
            Hooks.Log = (_, kind, logEvent, sid) => Trace.Add("log," + kind + "," + (int)logEvent + "," + sid);
            Hooks.Assertion = (_, line) => Trace.Add("assert," + line);
            Hooks.Delay = current =>
            {
                Trace.Add("delay");
                var candidates = new RawAiBridgeCandidates(Memory, Pool, Types, Frames, Surfaces, Spots, new AiBridgeCandidateTypes
                {
                    Bridge = () => 82,
                    Fencemark = () => 90,
                    WindArcher = () => 140,
                });
                var planning = new RawAiBridgePlanning(candidates, Random, () => Now, null);
                planning.DelayRetry(current);
            };
        }

        /// <summary>같은 실제 메모리와 현재 관문을 제품의 네 전략에 전달한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
        /// <returns>검사할 다리 전략 몸체다.</returns>
        public RawAiBridgeStrategies Make() => new(Memory, Pool, Types, Random, new AiBridgeStrategyGlobals
        {
            Now = () => Now,
            MapLimit = () => MapLimit,
            InvalidGraph = () => 254,
            Checking = () => Cfg[2] != 0,
            Disabled = () => Cfg[3] != 0,
            GeyserLog = () => LogKind,
        }, Hooks);

        /// <summary>현재 본체 DWORD를 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
        /// <param name="at">읽을 위치다.</param>
        /// <returns>읽은 값이다.</returns>
        public uint Field(int at) => Get(Memory.Bytes(Token), at);

        /// <summary>현재 본체 DWORD를 쓴다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
        /// <param name="at">쓸 위치다.</param>
        /// <param name="value">쓸 값이다.</param>
        public void Field(int at, uint value) => Put(Memory.Bytes(Token), at, value);

        /// <summary>실제 목록 네 개의 미사용 꼬리까지 순서대로 비교한다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
        /// <returns>이어 붙인 16진이다.</returns>
        public string ListsHex()
        {
            string text = "";
            // 입력과 같은 소유 핸들 순서다.
            foreach (uint list in Lists)
            {
                text += Hex(Memory.Bytes(list));
            }

            return text;
        }

        /// <summary>도달한 현재 하위 호출들을 원문과 같은 구분자로 나타낸다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
        /// <returns>세미콜론 구분 호출열이다.</returns>
        public string TraceText()
        {
            string text = "";
            // 첫 항목 앞에는 구분자를 붙이지 않는다.
            foreach (string item in Trace)
            {
                if (text.Length != 0)
                {
                    text += ';';
                }

                text += item;
            }

            return text;
        }
    }

    /// <summary>네 case 전체/공통 관문/NaN/재고 뒤 목표 재조회/중복 목록/기한/좌표의 독립 원문이다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    [Fact]
    public void Native198WholeBodiesListsRandomAndOrder()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-strategies");
        int count = 0;
        // 원본 입력만 설치하고 전체 제품 몸체를 한 번 호출하는 반복문이다.
        foreach (string[] row in rows)
        {
            Assert.Equal(9, row.Length);
            var scene = new StrategyScene(row);
            scene.Make().Update(scene.Token);
            bool same = Hex(scene.Memory.Bytes(scene.Token)) == row[5] && scene.ListsHex() == row[6] && scene.Random.State() == X86Fixture.UInt(row[7]) && scene.TraceText() == row[8];
            Assert.True(same);
            count++;
        }

        Assert.Equal(198, count);
    }

    /// <summary>checking이 거짓이면 disabled 장치를 읽지 않고 재고 효과 뒤 현재 목표/좌표를 다시 쓴다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    [Fact]
    public void CurrentGoalAndCoordinateAfterEffects()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-strategies");
        var scene = new StrategyScene(rows[5]);
        int disabled = 0;
        scene.Hooks.Refresh = (_, _) => scene.Field(0x2d4, 4);
        scene.Hooks.Rectangle = (_, _, _, _) =>
        {
            scene.Field(0xaf0, BitConverter.SingleToUInt32Bits(111f));
            scene.Field(0xaf4, BitConverter.SingleToUInt32Bits(112f));
            return true;
        };
        scene.Hooks.Advance = (_, point) =>
        {
            Assert.Equal(111f, point.X);
            Assert.Equal(112f, point.Y);
            return false;
        };
        var strategy = new RawAiBridgeStrategies(scene.Memory, scene.Pool, scene.Types, scene.Random, new AiBridgeStrategyGlobals
        {
            Now = () => scene.Now,
            MapLimit = () => scene.MapLimit,
            InvalidGraph = () => 254,
            Checking = () => false,
            Disabled = () => { disabled++; return true; },
            GeyserLog = () => scene.LogKind,
        }, scene.Hooks);
        strategy.Update(scene.Token);
        Assert.Equal(0, disabled);
        Assert.Equal(0xc61c4000u, scene.Field(0xaf0));
        Assert.Equal(0xc61c4000u, scene.Field(0xaf4));
    }

    /// <summary>graph 하위가 대상을 바꾸면 중심도 변경 후 현재 SID에서 다시 읽는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    [Fact]
    public void TempleReadsCurrentTargetAfterGraph()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-strategies");
        var scene = new StrategyScene(rows[5]);
        scene.Hooks.Graph = sid =>
        {
            Assert.Equal(new Sid(51), sid);
            scene.Field(0x3cc, 52);
            return 8u;
        };
        bool advanced = false;
        scene.Hooks.Advance = (_, point) =>
        {
            Assert.Equal(125f, point.X);
            Assert.Equal(124f, point.Y);
            advanced = true;
            return true;
        };
        scene.Make().Update(scene.Token);
        Assert.True(advanced);
        Assert.Equal("technology,2;refresh,50;center,52", scene.TraceText());
    }

    /// <summary>실제 Point 동등 비교와 같이 연장 결과가 시작점과 같으면 확장 잔여수를 소비하지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    [Fact]
    public void UnchangedExtensionKeepsBudget()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-strategies");
        var scene = new StrategyScene(rows[13]);
        scene.Field(0x2f0, 0);
        scene.Cfg[10] = scene.Cfg[8];
        scene.Cfg[11] = scene.Cfg[9];
        uint before = scene.Field(0x3c8);
        scene.Make().Update(scene.Token);
        Assert.Equal(before, scene.Field(0x3c8));
        Assert.Equal((uint)scene.Cfg[8], scene.Field(0x3d0));
        Assert.Equal((uint)scene.Cfg[9], scene.Field(0x3d4));
        Assert.Contains("extend,", scene.TraceText());
    }

    /// <summary>기한이 NaN이면 네 전략 모두 현재 재고 갱신까지만 하고 선택/배치는 실행하지 않는다. 2026-11-25 R4-5b 다리 전략 이식에서 추가됐다.</summary>
    [Fact]
    public void UnorderedDeadlineSkipsEveryCase()
    {
        string[][] rows = X86Fixture.Read("ai-bridge-strategies");
        // 각 전략의 현재 기한 필드만 quiet NaN으로 공급하는 반복문이다.
        for (uint goal = 1; goal <= 4; goal++)
        {
            var scene = new StrategyScene(rows[(int)goal * 4 + 1]);
            int at = goal <= 2 ? 0x5a8 : 0x5b0;
            scene.Field(at, 0);
            scene.Field(at + 4, 0x7ff80000u);
            scene.Make().Update(scene.Token);
            Assert.Equal("technology,2;refresh,50", scene.TraceText());
        }
    }
}
