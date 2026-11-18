using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 원본 신전 가상 Owner(RawVortexOwner)를 원본 x86 기대값에 대조한다. 2026-11-19 0.620에서 R4-2 지역 소유자 확정용으로 추가했다.
// 장면은 cpppj/tests/VortexOwnerTests.cpp 의 VortexScene 과 같다:
// 실제 풀에 명시 SID 50/60/61/62를 두고 9 Player 신전 DWORD·별도 목록·현재 좌표·공통 Owner를 같은 입력으로 준비한다.
// 호출마다 외부 효과 순서·부모 슬롯 전체·Player 36 DWORD·소유자별 9 목록·revision을 비교한다.
// 근거: cpppj o/RawVortexOwner.h/.cpp 와 vortex-owner-x86.tsv(원본·CD 각 864행)다.
public sealed class X86RawVortexOwnerTests
{
    /// <summary>실제 풀의 슬롯 수(원본 관찰과 같다). 2026-11-19 0.620에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>소유자를 정할 신전 번호다. 2026-11-19 0.620에서 추가했다.</summary>
    private const int Source = 50;

    /// <summary>패치의 실제 신전 가상 표다. 2026-11-19 0.620에서 추가했다.</summary>
    private const uint VortexPatch = 0x00519860u;

    /// <summary>CD의 실제 신전 가상 표다. 2026-11-19 0.620에서 추가했다.</summary>
    private const uint VortexCd = 0x00501de0u;

    /// <summary>슬롯 전체를 소문자 hex 로 만든다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="span">슬롯 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>값을 리틀 엔디언으로 Player 기록에 쓴다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="bytes">대상 Player 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다.
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>Player 기록의 DWORD 를 읽는다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="bytes">Player 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    private static uint Get(byte[] bytes, int offset)
    {
        // 네 바이트를 낮은 바이트부터 합친다.
        return (uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    /// <summary>한 행의 풀·목록·전역을 소유하는 장면. 2026-11-19 0.620에서 추가했다.</summary>
    private sealed class Scene
    {
        /// <summary>신전과 outpost 가 공유하는 실제 풀이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly RawSidPool Pool;

        /// <summary>실제 9 Player 기록이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly byte[] Players;

        /// <summary>실제 별도 목록이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly RawPostPopList Outposts = new();

        /// <summary>같은 실제 Player별 작업장 목록이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly RawPostPopState Book = new();

        /// <summary>실제 전투 조건이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly SquidOwnerMode Mode = new();

        /// <summary>대체 함수 없이 쓰는 실제 공통 Owner다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly SquidOwner Owner;

        /// <summary>현재 DWORD와 첫 외부 도장 호출 수다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly UIntBox Revision = new() { Value = 37u };

        /// <summary>원본 외부 효과의 실제 인자 순서다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly List<string> Events = [];

        /// <summary>독립 원본 실행기에 공급한 6개 입력이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly uint[] Input;

        /// <summary>현재 판본의 필드/목록/revision 분배 조건이다. 2026-11-19 0.620에서 추가했다.</summary>
        public readonly bool Patch;

        /// <summary>외부 도장 호출 수다. 2026-11-19 0.620에서 추가했다.</summary>
        private int _paints;

        /// <summary>풀 번호와 공통 모듈을 실제 입력에서 구성한다. 2026-11-19 0.620에서 추가했다.</summary>
        /// <param name="edition">현재 판본이다.</param>
        /// <param name="input">6개 입력이다.</param>
        public Scene(OriginalEdition edition, uint[] input)
        {
            Patch = edition == OriginalEdition.Patch1078;
            Input = input;
            int stride = Patch ? 0xb4 : 0xac;
            Players = new byte[9 * stride];
            Pool = new RawSidPool(PoolCapacity, false, null, edition);
            // 이전 관찰과 같은 명시 번호를 확보한다.
            for (ushort sid = 5; sid < 128; ++sid)
            {
                int got = Pool.Allocate(SidAllocation.Client);
                if (got != sid)
                {
                    throw new InvalidOperationException("신전 가상 Owner 실제 SID 준비 오류");
                }
            }

            var types = new OriginalTypeRecord[edition == OriginalEdition.Patch1078 ? 188 : 171];
            // 모든 타입 번호에 빈 타입 기록을 채우는 반복문이다.
            for (int i = 0; i < types.Length; ++i)
            {
                types[i] = new OriginalTypeRecord();
            }

            types[82].Flags2 = 0x200u;
            Mode.Battle = true;
            // 기존 공통 Owner가 쓰는 9개의 실제 목록을 같은 입력으로 준비한다.
            foreach (RawPostPopList list in Book.OwnerFactories)
            {
                list.Entries = [1, 2, 3, 4, 5, 6];
                list.Count = 0;
            }

            Owner = new SquidOwner(Pool, types, Book, Mode);
            Span<byte> raw = Pool.AllocatedBytes(Source);
            raw.Fill(0);
            Put(raw, 0, Patch ? VortexPatch : VortexCd);
            raw[10] = 82;
            raw[11] = (byte)input[2];
            raw[Patch ? 34 : 32] = (byte)input[0];
            raw[Patch ? 40 : 35] = (byte)input[3];
            Put(raw, 14, BitConverter.SingleToUInt32Bits(40.5f));
            Put(raw, 18, BitConverter.SingleToUInt32Bits(50.25f));
            Array.Fill(Players, (byte)0xa5);
            // 신전 네 DWORD만 원본 입력으로 채우고 다른 Player 바이트는 유지한다.
            for (int player = 0; player < 9; ++player)
            {
                Put(Players, Temple(player), input[4]);
                Put(Players, Temple(player) + 4, 0x11223344u);
                Put(Players, Temple(player) + 8, 0x55667788u);
                Put(Players, Temple(player) + 12, 0x99aabbccu);
            }

            Outposts.Entries = [60, 61, 62];
            Outposts.Count = 2;
            // 원본 추가 목록의 현재 owner·좌표 입력이다. free/dead 여부로 후보를 거르지 않는다.
            for (int index = 0; index < 3; ++index)
            {
                Span<byte> other = Pool.AllocatedBytes(60 + index);
                other.Fill(0);
                uint[] owners = [input[0], input[1], 8u];
                other[Patch ? 34 : 32] = (byte)owners[index];
                Put(other, 14, BitConverter.SingleToUInt32Bits(70.5f + index));
                Put(other, 18, BitConverter.SingleToUInt32Bits(80.25f + index));
            }
        }

        /// <summary>실제 판본별 신전 필드 위치다. 2026-11-19 0.620에서 추가했다.</summary>
        /// <param name="player">조회할 소유자다.</param>
        public int Temple(int player) => player * (Patch ? 0xb4 : 0xac) + (Patch ? 0x7c : 0x74);

        /// <summary>원본과 같은 명시 외부 효과 변경을 공급하며 신전 몸체 결과는 계산하지 않는다. 2026-11-19 0.620에서 추가했다.</summary>
        public VortexOwnerHooks Hooks()
        {
            return new VortexOwnerHooks
            {
                IslandNumber = (_, _) => 3,
                IslandExists = _ => true,
                SetIslandOwner = (island, player) =>
                {
                    Events.Add($"I:{island}:{player}");
                    if (Input[5] == 1)
                    {
                        Span<byte> raw = Pool.AllocatedBytes(Source);
                        raw[Patch ? 34 : 32] = 8;
                        Put(raw, 14, BitConverter.SingleToUInt32Bits(42.75f));
                        Put(raw, 18, BitConverter.SingleToUInt32Bits(60.5f));
                    }
                },
                RegionOwnership = (x, y) =>
                {
                    Events.Add($"P:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}");
                    if (_paints == 0)
                    {
                        if (Input[5] == 1 || Input[5] == 3)
                        {
                            Outposts.Count = 3;
                        }

                        if (Input[5] == 2)
                        {
                            Outposts.Count = 0;
                            Pool.AllocatedBytes(Source)[Patch ? 40 : 35] = 1;
                        }

                        if (Input[5] == 3)
                        {
                            Pool.AllocatedBytes(Source)[Patch ? 34 : 32] = 1;
                        }
                    }

                    ++_paints;
                },
                RefreshProduction = () =>
                {
                    Events.Add("R");
                    if (Patch && Input[5] == 3)
                    {
                        Revision.Value = 0xffffffffu;
                    }
                },
                MissingIsland = (_, _, _) => throw new InvalidOperationException("이번 정상 PE 입력은 missingIsland를 요청하지 않는다"),
            };
        }

        /// <summary>실제 Player 필드·공통 목록·raw 슬롯·revision을 원본 출력 형식으로 수집한다. 2026-11-19 0.620에서 추가했다.</summary>
        public string[] Output()
        {
            string actions = string.Join(";", Events);
            // CD 원본 슬롯은 36바이트이며 DN 풀의 50바이트 중 앞 36바이트로 대조한다.
            string hex = Hex(Pool.Slot(Source)[..(Patch ? 50 : 36)].ToArray());
            string temples = string.Join(",", Enumerable.Range(0, 9).SelectMany(player => Enumerable.Range(0, 4).Select(field => Get(Players, Temple(player) + field * 4).ToString(CultureInfo.InvariantCulture))));
            string lists = string.Join("|", Book.OwnerFactories.Select(list => $"{list.Count}:{string.Join(",", list.Entries)}"));
            return [actions, hex, temples, lists, Revision.Value.ToString(CultureInfo.InvariantCulture)];
        }
    }

    /// <summary>원본 내부 호출 표식은 독립 공통 Owner 결과로 확인하고 외부 콜백 순서와 구별한다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="value">원본 관찰의 효과 문자열이다.</param>
    private static string ExternalEvents(string value)
    {
        var kept = new List<string>();
        // 내부 B 표식은 원본 함수 호출 계수이며 외부 콜백이 아니다.
        foreach (string part in value.Split(';'))
        {
            if (!part.StartsWith("B:", StringComparison.Ordinal))
            {
                kept.Add(part);
            }
        }

        return string.Join(";", kept);
    }

    /// <summary>구현 계산으로 기대값을 만들지 않고 같은 입력의 실제 두 PE 결과를 전수 재생한다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="edition">판본 이름이다. originals·originalCD 중 하나다.</param>
    private static void Replay(string edition)
    {
        string[][] rows = X86Fixture.Read("vortex-owner");
        int count = 0;
        // 같은 실제 명령에서 나온 각 반환을 전체 raw/Player/목록으로 확인한다.
        foreach (string[] row in rows)
        {
            if (row[0] != edition)
            {
                continue;
            }

            Assert.Equal(7, row.Length);
            uint[] input = row[1].Split(',').Select(part => uint.Parse(part, CultureInfo.InvariantCulture)).ToArray();
            Assert.Equal(6, input.Length);
            var scene = new Scene(edition == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, input);
            var vortex = new RawVortexOwner(scene.Pool, scene.Owner, scene.Players, scene.Outposts, scene.Revision, scene.Hooks());
            Assert.True(vortex.Handles(Source));
            vortex.Set(Source, input[1]);
            var expected = row.Skip(2).ToArray();
            expected[0] = ExternalEvents(expected[0]);
            string[] output = scene.Output();
            for (int k = 0; k < expected.Length; ++k)
            {
                Assert.True(output[k] == expected[k], $"신전 가상 {edition} 입력 {row[1]} 칸 {k} 불일치: 출력 {output[k]} / 기대 {expected[k]}");
            }

            ++count;
        }

        Assert.Equal(864, count);
    }

    /// <summary>패치의 현재 추가 목록·revision·효과 뒤 owner/좌표 변경을 실제 PE와 비교한다. 2026-11-19 0.620에서 추가했다.</summary>
    [Fact]
    public void ReplaysOriginals()
    {
        Replay("originals");
    }

    /// <summary>CD의 추가 목록/revision 생략과 초기 신전 DWORD 폭을 실제 PE와 비교한다. 2026-11-19 0.620에서 추가했다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        Replay("originalCD");
    }

    /// <summary>미복원 가상 표/잘못된 9 기록은 최초 raw/Player 변경 전에 거부한다. 2026-11-19 0.620에서 추가했다.</summary>
    [Fact]
    public void RejectsForeignTableAndPlayerSize()
    {
        string[][] rows = X86Fixture.Read("vortex-owner");
        uint[] input = rows[0][1].Split(',').Select(part => uint.Parse(part, CultureInfo.InvariantCulture)).ToArray();
        var scene = new Scene(OriginalEdition.Patch1078, input);
        byte[] before = (byte[])scene.Players.Clone();
        Assert.Throws<ArgumentException>(() => new RawVortexOwner(scene.Pool, scene.Owner, scene.Players.AsSpan(0, 10).ToArray(), scene.Outposts, scene.Revision, scene.Hooks()));
        var vortex = new RawVortexOwner(scene.Pool, scene.Owner, scene.Players, scene.Outposts, scene.Revision, scene.Hooks());
        Put(scene.Pool.AllocatedBytes(Source), 0, 0x11111111u);
        Assert.Throws<ArgumentException>(() => vortex.Set(Source, 1));
        Assert.Equal(before, scene.Players);
        Assert.Empty(scene.Events);
    }
}
