using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 피해가능객체(Damageable) 삭제 후 처리를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/DamageablePostDestroyTests.cpp 의 DamageablePostReplayScene 과 같다: 실제 슬롯 풀·공간 해시·
/// 타입 표·프레임 표 위에서 드롭·섬 정리·주변 HP 예약·다리 약화와 공통 post 를 실행한다.
/// 호출마다 외부 경계 순서·관찰 대상 7개 슬롯의 전체 바이트를 비교한다.
/// 기준 판본은 10.78 이며 originalCD 행은 읽지 않는다 (W03 삭제·수명 일곱째 조각, 2026-10-21).
/// </summary>
public sealed class X86RawDamageablePostDestroyTests
{
    /// <summary>10.78 행의 판본 이름. 그 밖은 CD 10.72 다.</summary>
    private const string PatchEdition = "originals";

    /// <summary>10.78 한 판본의 행 수(Post 1,606 + Drops 640).</summary>
    private const int RowsPerEdition = 2246;

    /// <summary>관찰 대상 슬롯 번호.</summary>
    private static readonly int[] PostIds = [50, 60, 61, 62, 63, 64, 65];

    /// <summary>실행기 안의 합성 가상 표 기록값. 호스트 주소로 역참조하지 않는다.</summary>
    private const uint PostVtable = 0x11307000;

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
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

    /// <summary>슬롯 안의 값을 읽는다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    /// <param name="width">읽는 바이트 수.</param>
    private static uint Get(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 필요한 폭만 낮은 바이트부터 조합한다
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (i * 8);
        }
        return value;
    }

    /// <summary>한 삭제 후 처리 행의 풀·해시·타입 표를 소유하는 재생 장면.</summary>
    private sealed class Scene
    {
        /// <summary>삭제 대상 슬롯 풀(32,768칸, 서버 아님).</summary>
        public readonly RawSidPool Pool;

        /// <summary>4단계 공간 해시.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>칸별 점유 지도(65,536칸).</summary>
        public readonly byte[] Spots = new byte[RawSquidHash.WorldCells * RawSquidHash.WorldCells];

        /// <summary>타입 표(10.78 188개).</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>타입별 프레임 코드 표(10.78 188개).</summary>
        public readonly FrameCode[][] Frames;

        /// <summary>공유 장부.</summary>
        public readonly RawPostPopState Bookkeeping = new();

        /// <summary>HP 조회 전역.</summary>
        public readonly RawHitPointState HitPointState = new();

        /// <summary>삭제 후 처리 전역.</summary>
        public readonly DamageablePostDestroyState State = new();

        /// <summary>나간 외부 경계 순서.</summary>
        public readonly List<string> Events = [];

        /// <summary>고정 fixture 입력 18개.</summary>
        public uint[] Input { get; private set; } = new uint[18];

        /// <summary>장면을 만든다. 실제 할당 영역 5~127 을 확보한다.</summary>
        public Scene()
        {
            Pool = new RawSidPool(32768, false);
            // 원본 분석 장면의 대상을 실제 할당 영역으로 준비한다
            for (int id = 5; id <= 127; id++)
            {
                Assert.Equal(id, Pool.Allocate(SidAllocation.Client));
            }
            Types = new OriginalTypeRecord[OriginalConstructors.TypeCount];
            // 타입 번호 전체에 빈 타입 레코드를 채운다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            Frames = new FrameCode[Types.Length][];
            // 프레임은 같은 실제 코드 표 순서로 금간 첫 검색과 단단한 판정을 공급한다
            FrameCode[] codes =
            [
                new FrameCode('A', 'P', 1, FrameCodeFlags.None),
                new FrameCode('A', 'P', 11, (FrameCodeFlags)0x20),
                new FrameCode('A', 'P', 21, (FrameCodeFlags)0x40),
                new FrameCode('A', 'P', 11, (FrameCodeFlags)0x60),
            ];
            for (int i = 0; i < Frames.Length; i++)
            {
                Frames[i] = codes;
            }
        }

        /// <summary>슬롯 바이트를 직접 쓴다(검사 입력용).</summary>
        /// <param name="sid">대상 슬롯 번호.</param>
        /// <returns>쓰기 가능한 슬롯 바이트.</returns>
        private Span<byte> Slot(int sid) => Pool.RawSlot(sid);

        /// <summary>opcode 와 같은 입력 raw·버킷을 구성한다.</summary>
        /// <param name="values">고정 입력 18개.</param>
        public void Prepare(uint[] values)
        {
            Assert.Equal(18, values.Length);
            Input = values;
            Hash.Reset();
            Events.Clear();
            // 타입 표를 원본 기본 상태로 되돌린다
            for (int i = 0; i < Types.Length; i++)
            {
                Types[i] = new OriginalTypeRecord();
            }
            // 각 원본 후보의 좌표·상태·발자국·해시 단계를 입력 순서로 설정한다
            foreach (int sid in PostIds)
            {
                uint type = sid == 50 ? values[4] : sid < 62 ? 90u : sid == 62 ? 92u : sid < 65 ? 82u : 94u;
                Span<byte> raw = Slot(sid);
                raw.Clear();
                Put(raw, 0, PostVtable);
                raw[RawSidPool.TypeOffset] = (byte)type;
                raw[RawSidPool.StateOffset] = (byte)(sid == 50 ? 2 : 0);
                raw[40] = (byte)(sid == 50 ? values[3] : sid == 61 ? 1u : 0u);
                raw[34] = 2;
                Put(raw, 12, 0x7355, 2);
                float x = sid == 50 ? BitConverter.UInt32BitsToSingle(values[16])
                    : sid == 60 ? 49.25f : sid == 61 ? 49.5f : sid == 63 ? 49.0f : sid == 65 ? 50.5f : 50.0f;
                float y = sid == 50 ? BitConverter.UInt32BitsToSingle(values[17])
                    : sid == 60 ? 49.25f : sid == 61 ? 49.5f : sid == 63 || sid == 64 ? 49.0f : sid == 65 ? 50.5f : 50.0f;
                Put(raw, 14, BitConverter.SingleToUInt32Bits(x));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(y));
                Put(raw, 36, (uint)(sid == 63 ? values[11] % 4 : sid == 64 ? 1u : 0u), 4);
                OriginalTypeRecord record = Types[type];
                record.Flags1 = type == 90 ? 0x10u : 0u;
                record.Flags2 = type == 82 ? 4u : type == 94 ? 0x1000000u : 0u;
                record.MaxHitPoints = 100;
                record.Group = 0;
                record.FootX = sid == 50 ? (int)values[9] : sid == 65 ? 3 : 1;
                record.FootY = sid == 50 ? (int)values[10] : sid == 65 ? 3 : 1;
                if (sid != 50)
                {
                    int level = sid == 63 || sid == 64 ? 0 : 1;
                    ref ushort bucket = ref Hash.Bucket(level, x, y);
                    Put(raw, 4, bucket, 2);
                    bucket = (ushort)sid;
                }
            }
            OriginalTypeRecord source = Types[values[4]];
            source.Flags1 = values[5];
            source.Flags2 = values[6];
            source.Group = (int)values[7];
            source.MaxHitPoints = (int)values[8];
            State.Battle = values[1] != 0;
            State.Boss = values[2] != 0;
            State.DudeCount = (int)values[12];
            State.DamageEvent = values[13];
            State.BridgeCrackLife = values[14];
        }

        /// <summary>실제 하위 함수 대체가 지정하는 경계만 기록한다.</summary>
        /// <returns>삭제 후 처리 훅.</returns>
        public DamageablePostDestroyHooks Hooks()
        {
            return new DamageablePostDestroyHooks
            {
                DropDude = (sid, index, randomPoint) =>
                {
                    Events.Add($"D:{sid}:{index}:{(randomPoint ? 1 : 0)}");
                    if (Input[15] == 1)
                    {
                        Slot(50)[RawSidPool.TypeOffset] = 91;
                    }
                    if (Input[15] == 6)
                    {
                        Slot(50)[34] = 3;
                    }
                },
                DropIsland = (x, y, flags, owner) =>
                {
                    Events.Add($"I:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}:{flags}:{owner}");
                    if (Input[15] == 2)
                    {
                        Slot(50)[40] = 1;
                    }
                },
                Regular = (sid, @event, payload, count) =>
                {
                    Events.Add($"R:{sid}:{@event}:{BitConverter.SingleToUInt32Bits(payload)}:{count}");
                    if (Input[15] == 4)
                    {
                        Slot(61)[RawSidPool.StateOffset] = 2;
                    }
                },
                HasBridgeFall = sid =>
                {
                    Events.Add($"F:{sid}");
                    return Input[11] == 3;
                },
                NotifySurface = sid =>
                {
                    Events.Add($"N:{sid}");
                    if (Input[15] == 3)
                    {
                        Put(Slot(sid), 36, 2, 4);
                    }
                },
                DisplayUpdate = sid =>
                {
                    Events.Add($"U:{sid}:{Get(Slot(sid), 36, 4)}");
                    if (Input[15] == 5)
                    {
                        Put(Slot(sid), 12, 0xfedc, 2);
                    }
                },
                Destroy = (sid, flags) => Events.Add($"X:{sid}:{flags}"),
                Base = (sid, flags) => Events.Add($"B:{sid}:{flags}"),
            };
        }

        /// <summary>관찰한 모든 경계와 현재 raw 후보를 원본 행 형식 그대로 직렬화한다.</summary>
        /// <returns>이벤트 문자열.</returns>
        public string EventText() => Events.Count == 0 ? "-" : string.Join(';', Events);

        /// <summary>관찰한 전체 후보의 raw 바이트와 번호를 같이 저장한다.</summary>
        /// <returns>슬롯 hex 문자열.</returns>
        public string Raw() => string.Join(';', PostIds.Select(id => $"{id}:{Convert.ToHexStringLower(Pool.Slot(id).ToArray())}"));
    }

    /// <summary>
    /// 피해가능객체 삭제 후 처리 fixture 의 10.78 몫을 재생한다.
    /// "Post" 절은 공통 post 를, "Drops" 절은 드롭 수량 계산만 부른다.
    /// </summary>
    [Fact]
    public void PostDestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("damageablepostdestroy");
        int count = 0;
        X86Fixture.CheckRows(rows, row =>
        {
            if (row[0] != PatchEdition)
            {
                return true;
            }
            uint[] input = row[2].Split(',').Select(X86Fixture.UInt).ToArray();
            var scene = new Scene();
            scene.Prepare(input);
            var hp = new RawHitPoints(scene.Pool, scene.Types, scene.HitPointState);
            var footprint = new RawSquidNeighbors(scene.Pool, scene.Hash, scene.Spots, scene.Types, scene.Frames);
            var post = new RawDamageablePostDestroy(scene.Pool, scene.Hash, scene.Types, scene.Frames, hp, footprint,
                scene.State, scene.Hooks());
            if (row[1] == "Post")
            {
                post.PostDestroy(50, input[0]);
            }
            else
            {
                post.Drops(50, input[6], input[0] != 0);
            }
            // 구현에서 기대 분기를 만들지 않고 각 원본 관찰과 비교한다
            Assert.Equal(row[3], scene.EventText());
            Assert.Equal(row[4], scene.Raw());
            Assert.Equal("0", row[5]);
            count++;
            return true;
        });
        Assert.Equal(RowsPerEdition, count);
    }
}
