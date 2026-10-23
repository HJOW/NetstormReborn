using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

/// <summary>
/// 원본 월드 코어의 신전 삭제 전처리(RawVortexPreDestroy)를 원본 x86 기대값에 대조한다.
/// 장면은 cpppj/tests/VortexPreDestroyTests.cpp 의 VortexScene 과 같다:
/// 실제 풀에 부모 신전 50과 outpost 60·61을 두고, 목록 압축→섬 조회·진단→섬 지우기·소유자 전파→
/// 지역 갱신→outpost 지역 갱신→공통 하위→생산 갱신→Player 정리를 순서대로 기록한다.
/// 호출마다 Player 전체·세 슬롯 전체·목록 48바이트·로딩·효과 문자열을 비교한다.
/// 기준 판본은 10.78 이다 (W03 신전 삭제 조각, 2026-10-23).
/// </summary>
public sealed class X86RawVortexPreDestroyTests
{
    /// <summary>원본 관찰이 쓰는 삭제 플래그.</summary>
    private const uint TestFlags = 0x400000;

    /// <summary>원본 관찰의 부모와 두 outpost SID 입력.</summary>
    private static readonly int[] VortexIds = [50, 60, 61];

    /// <summary>가림 비트가 없을 때 쓰는 extra 값 표(원본 관찰과 같다).</summary>
    private static readonly byte[] ExtraByVariant = [0, 1, 8, 9];

    /// <summary>한 Player 기록의 바이트 수.</summary>
    private const int PlayerStride = 0xb4;

    /// <summary>슬롯 전체를 소문자 hex 로 만든다.</summary>
    /// <param name="span">슬롯 바이트.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다.</summary>
    /// <param name="bytes">대상 슬롯 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>값을 리틀 엔디언으로 Player 기록에 쓴다.</summary>
    /// <param name="bytes">대상 Player 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>한 행의 풀·목록·전역을 소유하는 장면.</summary>
    private sealed class Scene
    {
        /// <summary>신전과 outpost 가 공유하는 실제 풀(24,000칸, 클라이언트).</summary>
        public readonly RawSidPool Pool;

        /// <summary>원본 9×0xb4 의 같은 Player 기록.</summary>
        public readonly byte[] Players = new byte[9 * PlayerStride];

        /// <summary>현재 신전 등록 목록.</summary>
        public readonly RawPostPopList Temples = new();

        /// <summary>현재 거주 구역 등록 목록.</summary>
        public readonly RawPostPopList Residences = new();

        /// <summary>효과 뒤 현재 값으로 읽는 outpost 목록.</summary>
        public readonly RawPostPopList Outposts = new();

        /// <summary>현재 로딩 깊이. 콜백이 바꾸면 다음 분기가 다시 읽는다.</summary>
        public readonly VortexPreDestroyState State = new();

        /// <summary>실제로 선택된 외부 하위만 순서대로 기록한 문자열.</summary>
        public readonly List<string> TraceParts = [];

        /// <summary>검증 대상 신전 삭제 몸체.</summary>
        public readonly RawVortexPreDestroy Body;

        /// <summary>원본 관찰이 지정한 입력 갈래.</summary>
        private readonly uint _variant;

        /// <summary>원본 관찰이 지정한 콜백 변형 번호.</summary>
        private readonly uint _mutation;

        /// <summary>한 행의 입력으로 장면을 만든다.</summary>
        /// <param name="variant">소유자·일치·로딩·가림을 정하는 입력 갈래.</param>
        /// <param name="mutation">콜백 뒤 효과를 바꾸는 변형 번호.</param>
        public Scene(uint variant, uint mutation)
        {
            _variant = variant;
            _mutation = mutation;
            Pool = new RawSidPool(24000, false);
            // 마지막 검사 대상 61까지 실제 클라이언트 슬롯을 차례로 확보한다
            for (;;)
            {
                int got = Pool.Allocate(SidAllocation.Client);
                // 61을 넘기면 장면 준비가 잘못된 것이다
                if (got > 61)
                {
                    throw new InvalidOperationException("신전 검사 실제 SID 준비 오류");
                }
                if (got == 61)
                {
                    break;
                }
            }
            uint owner = (variant >> 2) & 15u;
            uint match = (variant >> 6) & 3u;
            State.Loading = (variant & 256u) != 0 ? 1u : 0u;
            Array.Fill(Players, (byte)0xa5);
            // 네 신전 참조를 owner 0까지 독립 입력으로 준비한다
            for (uint player = 0; player < 9; player++)
            {
                Put(Players, (int)(player * PlayerStride + 0x7c), (match & 1u) != 0 ? 50u : 99u);
                Put(Players, (int)(player * PlayerStride + 0x80), 99u);
                Put(Players, (int)(player * PlayerStride + 0x84), (match & 2u) != 0 ? 50u : 99u);
                Put(Players, (int)(player * PlayerStride + 0x88), 99u);
            }
            // 각 실제 할당 슬롯에 원본과 같은 전체 50바이트를 복사한다
            foreach (int sid in VortexIds)
            {
                var bytes = new byte[50];
                bytes[10] = sid == 50 ? (byte)124 : (byte)126;
                bytes[11] = sid == 50 ? (byte)2 : (byte)0;
                bytes[34] = (byte)(sid == 61 ? (owner + 1) % 9 : owner);
                bytes[40] = sid == 50 ? ExtraByVariant[variant & 3u] : (byte)0;
                Put(bytes.AsSpan(), 14, BitConverter.SingleToUInt32Bits((float)sid + 0.25f));
                Put(bytes.AsSpan(), 18, BitConverter.SingleToUInt32Bits((float)sid + 0.5f));
                bytes.CopyTo(Pool.AllocatedBytes(sid));
            }
            Temples.Entries = [50, 60, 50, 61];
            Temples.Count = 4;
            Residences.Entries = [62, 50, 63, 50];
            Residences.Count = 4;
            Outposts.Entries = [60, 61];
            Outposts.Count = 2;
            Body = new RawVortexPreDestroy(Pool, Players, Temples, Residences, Outposts, State, Hooks());
        }

        /// <summary>각 호출 시점의 인자 비트를 문자열로 기록한다.</summary>
        /// <param name="value">추가할 효과 조각.</param>
        private void Event(string value)
        {
            // 사건 사이에 세미콜론을 둔다
            if (TraceParts.Count != 0)
            {
                TraceParts.Add(";");
            }
            TraceParts.Add(value);
        }

        /// <summary>원본이 외부로 선택한 작은 경계의 현재 반환·변경 입력만 공급한다.</summary>
        private VortexPreDestroyHooks Hooks()
        {
            return new VortexPreDestroyHooks
            {
                IslandNumber = (x, y) =>
                {
                    Event($"Island,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)}");
                    return 2;
                },
                IslandExists = island =>
                {
                    Event($"Exists,{island}");
                    return (_variant & 512u) == 0;
                },
                IslandTemple = island =>
                {
                    Event($"Temple,{island}");
                    return (_variant & 1024u) != 0 ? 99u : 50u;
                },
                ClearIslandTemple = island => Event($"Clear,{island}"),
                IslandOwner = (island, owner) => Event($"Owner,{island},{owner}"),
                RegionOwnership = (x, y) => Event($"Region,{BitConverter.SingleToUInt32Bits(x)},{BitConverter.SingleToUInt32Bits(y)}"),
                DamageablePre = (sid, flags) =>
                {
                    Event($"Base,{flags}");
                    // 변형 번호마다 원본 관찰과 같은 현재 슬롯·로딩 변경을 적용한다
                    if (_mutation == 1)
                    {
                        Pool.RawSlot(sid)[34] = 2;
                    }
                    else if (_mutation == 2)
                    {
                        Pool.RawSlot(sid)[40] = 9;
                    }
                    else if (_mutation == 3)
                    {
                        State.Loading = 1;
                    }
                },
                RefreshProduction = () =>
                {
                    Event("Refresh");
                    // 변형 4는 생산 갱신 뒤 현재 소유자를 바꿔 Player 정리를 옮긴다
                    if (_mutation == 4)
                    {
                        Pool.RawSlot(50)[34] = 8;
                    }
                },
                Assertion = line => Event($"Assert,{line}"),
            };
        }

        /// <summary>외부 효과 문자열. 없으면 빈 문자열이다.</summary>
        public string TraceText() => string.Concat(TraceParts);

        /// <summary>출력 비교 대상 세 슬롯의 모든 150바이트를 hex 로 잇는다.</summary>
        public string ObjectsText()
        {
            string result = string.Empty;
            // 부모 50과 두 outpost 60·61 순서로 잇는다
            foreach (int sid in VortexIds)
            {
                result += Hex(Pool.Slot(sid));
            }
            return result;
        }

        /// <summary>논리 count 와 사용하지 않는 물리 꼬리까지 원문 순서로 직렬화한다.</summary>
        public string ListsText()
        {
            var bytes = new byte[48];
            Put(bytes, 0, Temples.Count);
            Put(bytes, 4, Residences.Count);
            int offset = 8;
            // 각 목록은 같은 초기 물리 크기를 그대로 유지한다
            foreach (RawPostPopList list in new[] { Temples, Residences, Outposts })
            {
                // 개수 밖 stale 값까지 순서대로 쓴다
                foreach (uint value in list.Entries)
                {
                    Put(bytes, offset, value);
                    offset += 4;
                }
            }
            return Hex(bytes);
        }
    }

    /// <summary>신전 삭제 fixture 의 10.78 몫 102회를 재생한다.</summary>
    [Fact]
    public void VortexPreDestroyMatchOriginalX86()
    {
        string[][] rows = X86Fixture.Read("vortex-pre");
        Assert.Equal(103, rows.Length);
        Assert.Equal("input", rows[0][0]);
        int count = 0;
        X86Fixture.CheckRows(rows.Skip(1).ToArray(), row =>
        {
            Assert.Equal(6, row.Length);
            string[] input = row[0].Split(',');
            Assert.Equal(3, input.Length);
            var scene = new Scene(X86Fixture.UInt(input[0]), X86Fixture.UInt(input[1]));
            scene.Body.PreDestroy(50, X86Fixture.UInt(input[2]));
            Assert.Equal(TestFlags, X86Fixture.UInt(input[2]));
            Assert.Equal(row[1], Hex(scene.Players));
            Assert.Equal(row[2], scene.ObjectsText());
            Assert.Equal(row[3], scene.ListsText());
            Assert.Equal(row[4], scene.State.Loading.ToString(System.Globalization.CultureInfo.InvariantCulture));
            Assert.Equal(row[5], scene.TraceText());
            count++;
            return true;
        });
        Assert.Equal(102, count);
    }
}
