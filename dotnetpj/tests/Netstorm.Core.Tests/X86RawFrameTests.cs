using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 프레임 지정(<see cref="SquidFrame"/>)을 원본 x86 기대값에 대조한다.
/// 기대값은 setframe-x86.tsv(프레임 변경, 판본별 1,920회) 중 10.78 몫이다.
/// 각 호출 뒤 효과 순서와 대상 슬롯 50바이트를 비교한다.
/// 열 구성은 cpppj/tests/SetFrameTests.cpp 와 같다.
/// CD 10.72·10.37 행은 기준 판본이 아니므로 읽지 않는다 (W02 Frame 조각, 2026-10-20).
/// </summary>
public sealed class X86RawFrameTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD·10.37 이다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>판본별 Set 호출 수. 전체 5,760행 중 10.78 몫이다.</summary>
    private const int SetsPerEdition = 1920;

    /// <summary>판본별 Thunk 확인 수.</summary>
    private const int ThunksPerEdition = 4;

    /// <summary>관찰 대상 타입 번호.</summary>
    private const int ObjectType = 82;

    /// <summary>검사 대상 번호.</summary>
    private const int Target = 50;

    /// <summary>원본 관찰의 고정 객체 위치.</summary>
    private const float FixedX = 20.75f;

    /// <summary>원본 관찰의 고정 객체 위치.</summary>
    private const float FixedY = 21.9f;

    /// <summary>두 배 프레임을 보는 타입 플래그1 비트(원본 0x440000).</summary>
    private const uint DoubleFrames = 0x440000;

    /// <summary>대체 표시 갱신 플래그 비트.</summary>
    private const uint FrameUpdateAlternate = 0x2000;

    /// <summary>값을 리틀 엔디언으로 쓴다. 폭 밖의 이웃 필드는 보존한다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    /// <param name="width">쓰는 바이트 수.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다
        for (int i = 0; i < width; i++)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>타입 종류 한 줄(플래그1·플래그2·프레임 수).</summary>
    /// <param name="Flags1">타입 플래그1.</param>
    /// <param name="Flags2">타입 플래그2.</param>
    /// <param name="Count">프레임 수.</param>
    private sealed record Kind(uint Flags1, uint Flags2, int Count);

    /// <summary>한 행의 입력으로 풀·타입·효과 기록기를 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>슬롯 풀.</summary>
        public readonly RawSidPool Pool;

        /// <summary>타입 표(82번만 쓴다).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>효과 순서 기록.</summary>
        public readonly List<string> Events = [];

        /// <summary>프레임별 표시 크기 8개.</summary>
        public readonly List<float[]> Sizes = [];

        /// <summary>현재 행의 타입 종류.</summary>
        public Kind CurrentKind = new(0, 0, 0);

        /// <summary>번호 5~74번을 클라이언트 영역에 확보한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(24000, false);
            // 원본 관찰의 번호 범위를 그대로 확보한다
            for (int id = 5; id <= 74; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 모든 타입 번호에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
        }

        /// <summary>타입 종류·표시 레벨·현재 프레임으로 슬롯을 채운다.</summary>
        /// <param name="kind">타입 종류.</param>
        /// <param name="level">표시 레벨 바이트.</param>
        /// <param name="frame">현재 프레임 DWORD.</param>
        public void Prepare(Kind kind, uint level, uint frame)
        {
            CurrentKind = kind;
            Events.Clear();
            Types[ObjectType].Flags1 = kind.Flags1;
            Types[ObjectType].Flags2 = kind.Flags2;
            Span<byte> raw = Pool.AllocatedBytes(Target);
            raw.Clear();
            Put(raw, 0, 0x11010000);
            raw[RawSidPool.TypeOffset] = ObjectType;
            Put(raw, 14, BitConverter.SingleToUInt32Bits(FixedX));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(FixedY));
            Put(raw, 0x24, frame);
            raw[0x21] = (byte)level;
        }

        /// <summary>객체의 현재 프레임 번호를 10진 문자열로 읽는다.</summary>
        private string FrameNow()
        {
            ReadOnlySpan<byte> raw = Pool.Slot(Target);
            uint value = 0;
            // 낮은 바이트부터 조합한다
            for (int i = 0; i < 4; i++)
            {
                value |= (uint)raw[0x24 + i] << (8 * i);
            }
            return value.ToString();
        }

        /// <summary>원본과 같은 효과·호출 규약으로 입력·raw 변경을 공급하는 연결 지점.</summary>
        public SquidFrameHooks Hooks() => new()
        {
            FrameSize = (sid, frame) =>
            {
                int limit = CurrentKind.Count * ((CurrentKind.Flags1 & DoubleFrames) != 0 ? 2 : 1);
                // 표를 벗어난 프레임은 원본 00419850 assert 로 거부한다
                if (frame < 0 || frame >= limit)
                {
                    throw new ArgumentOutOfRangeException(nameof(frame), frame, "프레임이 표 밖입니다(00419850 assert)");
                }
                return frame < Sizes.Count ? Sizes[frame] : [0f, 0f];
            },
            Update = (sid, flags) => Events.Add($"{((flags & FrameUpdateAlternate) != 0 ? "E:" : "D:")}{sid}:{FrameNow()}"),
            Unpop = (sid, flags) => Events.Add($"U:{sid}:{flags}:{FrameNow()}"),
            Pop = (sid, x, y, flags) => Events.Add($"P:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{flags}:{FrameNow()}"),
        };

        /// <summary>효과 순서를 하나의 문자열로 만든다. 없으면 "-" 다.</summary>
        public string EventText() => Events.Count == 0 ? "-" : string.Join(';', Events);
    }

    /// <summary>
    /// 프레임 변경 fixture 의 10.78 몫 1,920회를 재생한다.
    /// 호출마다 효과 순서와 대상 슬롯 50바이트를 대조한다.
    /// </summary>
    [Fact]
    public void FrameLevelsAndEffectsMatchOriginalX86()
    {
        string[][] all = X86Fixture.ReadFile("setframe-x86.tsv");
        var sizes = new List<float[]>();
        var kinds = new List<Kind>();
        var rows = new List<string[]>();
        // 머리 3종(Sizes·Kind·Set·Thunk)을 나눈다
        foreach (string[] row in all)
        {
            if (row[0] == "Sizes")
            {
                // 세미콜론으로 나뉜 크기 항목을 하나씩 읽는다
                foreach (string entry in row[1].Split(';'))
                {
                    string[] pair = entry.Split(',');
                    sizes.Add([X86Fixture.FloatBits(pair[0]), X86Fixture.FloatBits(pair[1])]);
                }
            }
            else if (row[0] == "Kind")
            {
                kinds.Add(new Kind(X86Fixture.UInt(row[2]), X86Fixture.UInt(row[3]), X86Fixture.Int(row[4])));
            }
            else
            {
                rows.Add(row);
            }
        }
        Assert.Equal(8, sizes.Count);
        Assert.Equal(4, kinds.Count);
        int sets = 0;
        int thunks = 0;
        var scene = new Scene();
        scene.Sizes.AddRange(sizes);
        X86Fixture.CheckRows(rows.ToArray(), row =>
        {
            if (row[1] != PatchEdition)
            {
                return true;
            }
            if (row[0] == "Thunk")
            {
                // 표시 갱신 한 번을 받는 쪽과 같은 경로다
                Assert.Equal(4, row.Length);
                Assert.Equal("D:50:0", row[3]);
                thunks++;
                return true;
            }
            Assert.Equal(9, row.Length);
            Assert.Equal("Set", row[0]);
            scene.Prepare(kinds[X86Fixture.Int(row[2])], X86Fixture.UInt(row[3]), X86Fixture.UInt(row[4]));
            var frames = new SquidFrame(scene.Pool, scene.Types, scene.Hooks());
            frames.Set(Target, unchecked((int)X86Fixture.UInt(row[5])), X86Fixture.UInt(row[6]));
            string actual = scene.EventText() + '|' + Convert.ToHexStringLower(scene.Pool.Slot(Target));
            Assert.Equal(row[7] + '|' + row[8], actual);
            sets++;
            return true;
        });
        Assert.Equal(SetsPerEdition, sets);
        Assert.Equal(ThunksPerEdition, thunks);
    }

    /// <summary>
    /// 판본 assert 에 걸리는 플래그는 상태를 바꾸기 전에 거부한다.
    /// 빠진 효과와 표 밖 프레임도 풀을 바꾸지 않는다.
    /// </summary>
    [Fact]
    public void RejectsMissingHooksAndInvalidFlags()
    {
        string[][] all = X86Fixture.ReadFile("setframe-x86.tsv");
        var kinds = new List<Kind>();
        // 종류 4개는 판본 구분이 없다
        foreach (string[] row in all)
        {
            if (row[0] == "Kind")
            {
                kinds.Add(new Kind(X86Fixture.UInt(row[2]), X86Fixture.UInt(row[3]), X86Fixture.Int(row[4])));
            }
        }
        Assert.Equal(4, kinds.Count);
        var scene = new Scene();
        scene.Sizes.AddRange(Enumerable.Range(0, 8).Select(_ => new float[] { 1f, 1f }));
        scene.Prepare(kinds[2], 2, 3);
        SquidFrameHooks hooks = scene.Hooks();
        // 빠진 효과로는 만들지 않는다
        SquidFrameHooks broken = scene.Hooks();
        broken.Pop = null;
        Assert.ThrowsAny<Exception>(() => new SquidFrame(scene.Pool, scene.Types, broken));
        broken = scene.Hooks();
        broken.FrameSize = null;
        Assert.ThrowsAny<Exception>(() => new SquidFrame(scene.Pool, scene.Types, broken));
        var frames = new SquidFrame(scene.Pool, scene.Types, hooks);
        byte[] before = scene.Pool.Bytes.ToArray();
        // 0·0x2000 밖의 플래그는 거부한다
        Assert.ThrowsAny<Exception>(() => frames.Set(Target, 1, 1));
        Assert.ThrowsAny<Exception>(() => frames.Set(Target, 1, 0x2010));
        Assert.Empty(scene.Events);
        Assert.Equal(before, scene.Pool.Bytes.ToArray());
        // 종류 2의 표 길이를 넘기는 프레임은 00419850 assert 로 거부한다
        scene.Prepare(kinds[2], 2, 9);
        byte[] invalid = scene.Pool.Bytes.ToArray();
        Assert.ThrowsAny<Exception>(() => frames.Set(Target, 1, 0));
        Assert.Empty(scene.Events);
        Assert.Equal(invalid, scene.Pool.Bytes.ToArray());
    }
}
