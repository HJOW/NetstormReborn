using System.Globalization;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// 원본 기계어의 Factory/아웃포스트 가상 후처리를 실제 풀·해시·프레임과 대조한다. 2026-11-19 0.624에서 R4-2 공장 정기 트리거용으로 추가했다.
// 장면은 cpppj/tests/FactoryPostPopTests.cpp 의 FactoryScene 과 같다. 일반 Factory 372행 전수 재생과 경계·전역·실제 프레임·레벨 검사를 수행한다.
// 근거: cpppj o/RawFactoryPostPop.h/.cpp 와 factorypostpop-x86.tsv(원본·CD 각 498행)다.
public sealed class X86RawFactoryPostPopTests
{
    /// <summary>실제 풀의 슬롯 수(원본 관찰과 같다). 2026-11-19 0.624에서 추가했다.</summary>
    private const int PoolCapacity = 24000;

    /// <summary>후처리 대상 신전·공장 번호다. 2026-11-19 0.624에서 추가했다.</summary>
    private const int Source = 50;

    /// <summary>슬롯 전체를 소문자 hex 로 만든다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <param name="span">슬롯 바이트다.</param>
    private static string Hex(ReadOnlySpan<byte> span) => Convert.ToHexStringLower(span.ToArray());

    /// <summary>값을 리틀 엔디언으로 슬롯에 쓴다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <param name="bytes">대상 슬롯 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> bytes, int offset, uint value, int width = 4)
    {
        // 낮은 바이트부터 폭만큼 쓴다.
        for (int i = 0; i < width; ++i)
        {
            bytes[offset + i] = (byte)(value >> (i * 8));
        }
    }

    /// <summary>값을 리틀 엔디언으로 슬롯에서 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <param name="bytes">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <param name="width">읽는 바이트 수다.</param>
    private static uint Get(ReadOnlySpan<byte> bytes, int offset, int width = 4)
    {
        uint value = 0;
        // 낮은 바이트부터 합친다.
        for (int i = 0; i < width; ++i)
        {
            value |= (uint)bytes[offset + i] << (i * 8);
        }

        return value;
    }

    /// <summary>입력 글자 구간만 공급하며 프레임 기대값은 native가 계산한다. 2026-11-19 0.624에서 추가했다.</summary>
    private static readonly RiftFrameCode[] FactoryCodes =
    [
        new RiftFrameCode((byte)'A', (byte)'A', (byte)0, (byte)0), new RiftFrameCode((byte)'A', (byte)'A', (byte)1, (byte)0),
        new RiftFrameCode((byte)'B', (byte)'A', (byte)0, (byte)0), new RiftFrameCode((byte)'B', (byte)'A', (byte)1, (byte)0),
        new RiftFrameCode((byte)'B', (byte)'A', (byte)2, (byte)0), new RiftFrameCode((byte)'H', (byte)'A', (byte)0, (byte)0),
        new RiftFrameCode((byte)'H', (byte)'A', (byte)1, (byte)0),
    ];

    /// <summary>한 행의 실제 풀·해시·프레임과 callback 입력을 소유하는 장면. 2026-11-19 0.624에서 추가했다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 판본의 필드 폭과 vtable을 선택한다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly bool Patch;

        /// <summary>관찰 대상 SID 슬롯을 확보·수정한다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly RawSidPool Pool;

        /// <summary>일반 finder가 읽는 실제 네 공간 해시다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly RawSquidHash Hash = new();

        /// <summary>현재 타입의 발자국·장부 입력이다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly OriginalTypeRecord[] Types;

        /// <summary>공통 후처리 몸체가 읽는 판본별 타입 표다. CD판은 뒤에 빈 기록을 채운 복사본을 쓴다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly IReadOnlyList<OriginalTypeRecord> BaseTypes;

        /// <summary>현재 상태별 첫 프레임/개수 구간이다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly RiftTypeFrames[] Frames;

        /// <summary>공통 깊이를 정확히 한 번 감소시키는 장부다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly RawPostPopState BaseState = new();

        /// <summary>직접 Damageable의 권한·로딩 상태다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly CarrierPostPopState CarrierState = new();

        /// <summary>기존 거주 구역과 공유하는 목록·권한이다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly AssetPostPopState State = new();

        /// <summary>원본 명시 효과의 호출 순서다. 2026-11-19 0.624에서 추가했다.</summary>
        public readonly List<string> Events = [];

        /// <summary>확보 성공 비트·현재 확보 위치·callback 변형 입력이다. 2026-11-19 0.624에서 추가했다.</summary>
        public uint ReserveMask;

        /// <summary>현재 확보 위치다. 2026-11-19 0.624에서 추가했다.</summary>
        public uint Alloc;

        /// <summary>callback 뒤 효과를 바꾸는 변형 번호다. 2026-11-19 0.624에서 추가했다.</summary>
        public uint Mutation;

        /// <summary>실제 풀·해시·프레임과 타입 표를 같은 판본으로 만든다. 2026-11-19 0.624에서 추가했다.</summary>
        /// <param name="patch">참이면 패치판, 거짓이면 CD판이다.</param>
        public Scene(bool patch)
        {
            Patch = patch;
            Pool = new RawSidPool(PoolCapacity, false, null, patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072);
            Types = new OriginalTypeRecord[patch ? 188 : 171];
            // 모든 타입 기록을 빈 값으로 채우는 반복문이다.
            for (int i = 0; i < Types.Length; ++i)
            {
                Types[i] = new OriginalTypeRecord();
            }

            Frames = new RiftTypeFrames[Types.Length];
            // 모든 타입 번호마다 같은 입력 글자 구간을 넣는 반복문이다.
            for (int i = 0; i < Frames.Length; ++i)
            {
                Frames[i] = new RiftTypeFrames(FactoryCodes);
            }

            // 관찰 번호를 빠짐없이 확보한다.
            for (ushort sid = 5; sid <= 61; ++sid)
            {
                int got = Pool.Allocate(SidAllocation.Client);
                if (got != sid)
                {
                    throw new InvalidOperationException("공장 후처리 실제 SID 준비 오류");
                }
            }

            // finder의 현재 발자국을 입력하며 그림 크기와 구별한다.
            foreach (OriginalTypeRecord type in Types)
            {
                type.FootX = 1;
                type.FootY = 1;
            }

            // 공통 후처리 몸체는 10.78 규격 188개를 받는다. CD판은 뒤에 빈 기록을 채운 복사본을 쓴다.
            if (Patch)
            {
                BaseTypes = Types;
            }
            else
            {
                var padded = new OriginalTypeRecord[188];
                Array.Copy(Types, padded, Types.Length);
                // 빈 칸을 빈 기록으로 채우는 반복문이다.
                for (int i = Types.Length; i < padded.Length; ++i)
                {
                    padded[i] = new OriginalTypeRecord();
                }

                BaseTypes = padded;
            }
        }

        /// <summary>패치 40/CD 35의 추가 바이트 위치다. 2026-11-19 0.624에서 추가했다.</summary>
        public int Extra => Patch ? 40 : 35;

        /// <summary>패치 36 DWORD/CD 34 바이트의 프레임 위치다. 2026-11-19 0.624에서 추가했다.</summary>
        public int FrameOffset => Patch ? 36 : 34;

        /// <summary>생성자가 기록하는 vtable의 전체 시작 주소다. 2026-11-19 0.624에서 추가했다.</summary>
        /// <param name="kind">0이면 일반 공장, 0이 아니면 아웃포스트 wrapper다.</param>
        public uint Table(uint kind) => Patch ? (kind != 0 ? 0x00509210u : 0x00509148u) : (kind != 0 ? 0x00503358u : 0x00503298u);

        /// <summary>입력 슬롯/체인/목록을 초기화하며 기대 결과는 만들지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
        /// <param name="input">11개 입력이다.</param>
        public void Prepare(uint[] input)
        {
            if (input.Length != 11)
            {
                throw new ArgumentException("공장 후처리 입력 11개 오류", nameof(input));
            }

            Events.Clear();
            Alloc = 0;
            ReserveMask = input[7];
            Mutation = input[10];
            State.Boss = input[4] != 0;
            BaseState.Suppressed = true;
            BaseState.Depth = 0;
            CarrierState.Boss = true;
            Hash.Cell(1, 25, 26) = 50;
            // 실제 finder의 next 저장 순서가 source→60→61이 되게 한다.
            foreach (int sid in new[] { Source, 60, 61 })
            {
                Span<byte> raw = Pool.AllocatedBytes(sid);
                raw.Fill(0);
                Put(raw, 0, Table(input[0]));
                Put(raw, 4, unchecked((uint)(sid == Source ? 60 : sid == 60 ? 61 : 0)), 2);
                raw[10] = (byte)(sid == 61 ? (input[1] != 86u ? 86u : 87u) : input[1]);
                raw[11] = (byte)(sid == Source ? input[5] << 5 : 0);
                raw[Extra] = (byte)(sid == Source ? input[3] : (sid == 61 ? 8u : 0u));
                Put(raw, 14, BitConverter.SingleToUInt32Bits(sid == Source ? 50.75f : sid == 60 ? 50.25f : 50.5f));
                Put(raw, 18, BitConverter.SingleToUInt32Bits(sid == Source ? 52.5f : sid == 60 ? 52.75f : 52.25f));
            }

            Put(Pool.AllocatedBytes(Source), FrameOffset, input[6], Patch ? 4 : 1);
            // 중복 번호나 꽉 찬 용량을 그대로 입력한다.
            foreach (RawPostPopList list in new[] { State.Outposts, State.Residences })
            {
                list.Entries = [input[9] != 0 ? 50u : 7u, 8u, 9u, 10u];
                list.Count = input[8];
            }
        }

        /// <summary>직접 Damageable가 실제 공통 장부를 호출하고 native 경계와 같은 사건을 남긴다. 2026-11-19 0.624에서 추가했다.</summary>
        /// <param name="base">공통 후처리 몸체다.</param>
        public CarrierPostPopHooks DamageableHooks(RawPostPop @base)
        {
            return new CarrierPostPopHooks
            {
                CarrierCheck = _ => throw new InvalidOperationException("Factory는 Carrier 가상을 호출하지 않는다"),
                ClaimGround = (_, _, _) => throw new InvalidOperationException("이 입력은 로딩 소유권을 호출하지 않는다"),
                ValidateBase = (sid, flags) => @base.ValidateBase(sid, flags),
                Base = (sid, flags) =>
                {
                    Events.Add("D:" + flags.ToString(CultureInfo.InvariantCulture));
                    if (Mutation == 1)
                    {
                        Pool.AllocatedBytes(sid)[11] = 7 << 5;
                    }

                    @base.PostPopBase(sid, flags);
                },
            };
        }

        /// <summary>native와 같은 프로세스·프레임/소유권/로그/삭제 관찰 경계를 제공한다. 2026-11-19 0.624에서 추가했다.</summary>
        public FactoryPostPopHooks Hooks()
        {
            return new FactoryPostPopHooks
            {
                ReserveRegular = () =>
                {
                    Events.Add("A:40");
                    bool allowed = (ReserveMask & (1u << (int)Alloc)) != 0;
                    ++Alloc;
                    return allowed;
                },
                AddRegular = (sid, @event, delay) =>
                {
                    Events.Add($"R:{sid}:{@event}:{BitConverter.SingleToUInt32Bits(delay)}");
                    if (Mutation == 2 && @event == 2)
                    {
                        Pool.AllocatedBytes(sid)[10] = 87;
                    }
                },
                SetFrame = (sid, frame, flags) =>
                {
                    Events.Add($"F:{unchecked((uint)frame)}:{flags}");
                    Put(Pool.AllocatedBytes(sid), FrameOffset, unchecked((uint)frame), Patch ? 4 : 1);
                },
                RegionOwnership = (x, y) =>
                {
                    Events.Add($"C:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}");
                    if (Mutation == 3)
                    {
                        Pool.AllocatedBytes(Source)[10] = 87;
                    }
                },
                WarnDuplicate = (sid, x, y) => Events.Add($"W:{unchecked((uint)x)}:{unchecked((uint)y)}"),
                Destroy = (sid, flags) =>
                {
                    Events.Add($"X:{sid}:{flags}");
                    Pool.AllocatedBytes(sid)[11] |= 2;
                    if (Mutation == 4)
                    {
                        Span<byte> next = Pool.AllocatedBytes(61);
                        next[Extra] = 0;
                        next[10] = Pool.Slot(Source)[10];
                    }
                },
            };
        }

        /// <summary>이전 목록 메모리와 callback 뒤 후보 상태를 원본 형식으로 기록한다. 2026-11-19 0.624에서 추가했다.</summary>
        public string[] Output()
        {
            string actions = Events.Count == 0 ? "-" : string.Join(";", Events);
            string lists = string.Join("|", new[] { State.Outposts, State.Residences }.Select(list => $"{list.Count.ToString(CultureInfo.InvariantCulture)}:{string.Join(",", list.Entries.Select(entry => entry.ToString(CultureInfo.InvariantCulture)))}"));
            string other = string.Join("|", new[] { 60, 61 }.Select(sid =>
            {
                ReadOnlySpan<byte> raw = Pool.Slot(sid);
                return $"{raw[10]},{raw[11]},{raw[Extra]}";
            }));
            return [actions, Hex(Pool.Slot(Source)[..(Patch ? 50 : 36)].ToArray()), lists, other];
        }
    }

    /// <summary>원본 내부 호출 표식은 구현 계산으로 기대값을 만들지 않고 같은 입력의 실제 두 PE 결과를 전수 재생한다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <param name="edition">판본 이름이다. originals·originalCD 중 하나다.</param>
    /// <returns>재생한 행 수다. 판본마다 186행이다.</returns>
    private static int Replay(string edition)
    {
        string[][] rows = X86Fixture.Read("factorypostpop");
        int count = 0;
        // 같은 실제 명령에서 나온 각 반환을 전체 raw/목록으로 확인한다.
        foreach (string[] row in rows)
        {
            if (row[0] != edition || row.Length != 6)
            {
                continue;
            }

            uint[] input = row[1].Split(',').Select(part => uint.Parse(part, CultureInfo.InvariantCulture)).ToArray();
            if (input[0] != 0 || input[1] == 126)
            {
                continue;
            }

            var scene = new Scene(edition == "originals");
            scene.Prepare(input);
            var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
            bookkeeping.Suppressed = true;
            var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
            var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, scene.Hooks());
            pop.SetFactoryPostPop(factory);
            if (scene.Patch)
            {
                bookkeeping.Depth++;
                pop.PostPopVirtual(Source, input[2]);
            }
            else
            {
                pop.Activate(Source, input[2]);
            }

            string[] expected = row.Skip(2).ToArray();
            string[] output = scene.Output();
            for (int k = 0; k < expected.Length; ++k)
            {
                Assert.True(output[k] == expected[k], $"공장 후처리 {edition} 입력 {row[1]} 칸 {k} 불일치: 출력 {output[k]} / 기대 {expected[k]}");
            }

            Assert.Equal(0u, bookkeeping.Depth);
            ++count;
        }

        return count;
    }

    /// <summary>원본 일반 Factory 186행을 같은 실제 풀에서 재생한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void ReplaysOriginals()
    {
        Assert.Equal(186, Replay("originals"));
    }

    /// <summary>CD 일반 Factory 186행을 같은 실제 풀에서 재생한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void ReplaysCd()
    {
        Assert.Equal(186, Replay("originalCD"));
    }

    /// <summary>수행 중 예외를 성공·실패로만 구분한다. 앞선 부분 효과를 보존했는지는 호출자가 따로 대조한다. 2026-11-19 0.624에서 추가했다.</summary>
    /// <param name="action">실행할 몸체다.</param>
    private static bool FactoryFailed(Action action)
    {
        try
        {
            action();
        }
        catch (Exception)
        {
            return true;
        }

        return false;
    }

    /// <summary>최초가 아니고 현재 프레임도 유효하면 예약·프레임·중복·지역 장치를 요구하지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void UnselectedDevicesAreOptional()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 0, 0, 0, 0, 0, 7, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, new FactoryPostPopHooks());
        pop.SetFactoryPostPop(factory);
        bookkeeping.Depth++;
        pop.PostPopVirtual(Source, 0);
        Assert.Equal(new[] { "D:0" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>두 new 실패는 미연결 생성자를 호출하지 않고 공통 몸체까지 진행한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void FailedReservationSkipsMissingConstructor()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 1, 0, 0, 0, 0, 0, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.AddRegular = null;
        hooks.SetFrame = null;
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        factory.PostPop(Source, 1);
        Assert.Equal(new[] { "A:40", "A:40", "D:1" }, scene.Events);
        Assert.Equal(2u, scene.Alloc);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>첫 확보 성공 효과는 뒤 생성자 장치가 없을 때도 남는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void MissingConstructorFollowsSuccessfulReservation()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.AddRegular = null;
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        Assert.True(FactoryFailed(() => factory.PostPop(Source, 1)));
        Assert.Equal(new[] { "A:40" }, scene.Events);
        Assert.Equal(1u, scene.Alloc);
        Assert.Equal(1u, bookkeeping.Depth);
    }

    /// <summary>프레임 장치 누락 전에 실행한 두 예약과 공통 깊이 감소를 되돌리지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void LateFrameFailureRetainsReservationsAndCommon()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 1, 0, 0, 0, 7, 7, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.SetFrame = null;
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        pop.SetFactoryPostPop(factory);
        bookkeeping.Depth++;
        Assert.True(FactoryFailed(() => pop.PostPopVirtual(Source, 1)));
        Assert.Equal(new[] { "A:40", "R:50:2:1090519040", "A:40", "R:50:5:1056964608", "D:1" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
        Assert.Equal(7u, Get(scene.Pool.Slot(Source), 36));
    }

    /// <summary>공통 부모가 바꾼 TYPE·레벨·boss를 뒤 프레임/중복 분기가 다시 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void CurrentTypeAndBossFollowCommonCallback()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 86, 0, 0, 0, 0, 7, 0, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var carrierHooks = scene.DamageableHooks(pop);
        var common = carrierHooks.Base;
        carrierHooks.Base = (sid, flags) =>
        {
            scene.Events.Add("D");
            scene.Pool.AllocatedBytes(sid)[10] = 87;
            scene.Pool.AllocatedBytes(sid)[11] = 1 << 5;
            scene.Pool.AllocatedBytes(60)[10] = 87;
            scene.State.Boss = true;
            pop.PostPopBase(sid, flags);
        };
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, carrierHooks);
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, scene.Hooks());
        bookkeeping.Depth++;
        factory.PostPop(Source, 0);
        Assert.Equal(new[] { "D", "F:2:0", "W:50:52", "X:60:0" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>중복 경고가 바꾼 후보는 뒤 삭제 장치 누락에도 보존한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void WarningPrecedesMissingDestroy()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 0, 0, 1, 0, 0, 0, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.Destroy = null;
        hooks.WarnDuplicate = (sid, _, _) =>
        {
            scene.Events.Add("W");
            scene.Pool.AllocatedBytes(sid)[11] = 0x10;
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        Assert.True(FactoryFailed(() => factory.PostPop(Source, 0)));
        Assert.Equal(new[] { "D:0", "W" }, scene.Events);
        Assert.Equal(0x10, scene.Pool.Slot(60)[11]);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>boss여도 실제 일치 후보가 없으면 경고·삭제 장치를 선택하지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void NoDuplicateSkipsMissingDiagnostics()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 0, 0, 1, 0, 0, 0, 0, 0, 0]);
        scene.Pool.AllocatedBytes(60)[10] = 86;
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.WarnDuplicate = null;
        hooks.Destroy = null;
        hooks.SetFrame = null;
        hooks.RegionOwnership = null;
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        factory.PostPop(Source, 0);
        Assert.Equal(new[] { "D:0" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>두 가상80이 TYPE을 각각 읽고 최초 프레임·첫 구간은 저장값을 유지한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void TwoCountsReadCurrentTypeAndRetainSavedFrame()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 0, 0, 0, 0, 6, 0, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        uint calls = 0;
        hooks.VirtualCount = sid =>
        {
            ++calls;
            scene.Events.Add("C" + calls.ToString(CultureInfo.InvariantCulture));
            Span<byte> raw = scene.Pool.AllocatedBytes(sid);
            raw[10] = (byte)(calls == 1 ? 86 : 87);
            Put(raw, 36, 100);
            return calls == 1 ? 1u : 7u;
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        factory.PostPop(Source, 0);
        Assert.Equal(new[] { "D:0", "C1", "C2", "F:2:0" }, scene.Events);
        Assert.Equal(2u, calls);
        Assert.Equal(2u, Get(scene.Pool.Slot(Source), 36));
    }

    /// <summary>두 번째 TYPE의 개수와 첫 TYPE의 시작을 더하며 콜백이 쓴 현재 프레임은 비교에 섞지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void SecondRunUsesCurrentFramesWithSavedFirst()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 0, 0, 0, 0, 6, 0, 0, 0, 0]);
        var codes = new List<RiftFrameCode>();
        // 두 번째 표에 다른 시작·개수를 명시 입력한다.
        for (int index = 0; index < 3; ++index)
        {
            codes.Add(new RiftFrameCode((byte)'A', (byte)'A', (byte)index, (byte)0));
        }

        for (byte index = 0; index < 10; ++index)
        {
            codes.Add(new RiftFrameCode((byte)'H', (byte)'A', index, (byte)0));
        }

        scene.Frames[87] = new RiftTypeFrames(codes);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        uint calls = 0;
        hooks.SetFrame = null;
        hooks.VirtualCount = sid =>
        {
            ++calls;
            scene.Pool.AllocatedBytes(sid)[10] = (byte)(calls == 1 ? 86 : 87);
            Put(scene.Pool.AllocatedBytes(sid), 36, 100);
            return calls == 1 ? 1u : 7u;
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        factory.PostPop(Source, 0);
        Assert.Equal(2u, calls);
        Assert.Equal(100u, Get(scene.Pool.Slot(Source), 36));
        Assert.Equal(new[] { "D:0" }, scene.Events);
    }

    /// <summary>선택된 가상80의 실패는 이미 끝난 공통 효과를 지우지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void CountFailureFollowsCommon()
    {
        var scene = new Scene(true);
        scene.Prepare([0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0]);
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.VirtualCount = _ =>
        {
            scene.Events.Add("C");
            throw new InvalidOperationException("명시 가상80 중단");
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        bookkeeping.Depth++;
        Assert.True(FactoryFailed(() => factory.PostPop(Source, 0)));
        Assert.Equal(new[] { "D:0", "C" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
        Assert.Equal(7u, Get(scene.Pool.Slot(Source), 36));
    }

    /// <summary>flags0·로딩0이면 물리 TYPE 오류는 공통 억제 뒤 실제 Count에서만 나타난다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void PhysicalTypeReadIsReachedAfterCommon()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 255, 0, 0, 0, 0, 7, 0, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, scene.Hooks());
        bookkeeping.Depth++;
        Assert.True(FactoryFailed(() => factory.PostPop(Source, 0)));
        Assert.Equal(new[] { "D:0" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
        var numbers = new FactoryPostPopTypes { FirstTimerFactory = 85, SecondTimer = 87, FixedFrame = 255 };
        var fixedFactory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, new FactoryPostPopHooks(), numbers);
        bookkeeping.Depth++;
        fixedFactory.PostPop(Source, 0);
        Assert.Equal(0u, bookkeeping.Depth);
        Assert.Equal(7u, Get(scene.Pool.Slot(Source), 36));
    }

    /// <summary>BYTE TYPE와 전체 DWORD 비교를 유지하며 생성 후 바뀐 현재 값을 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void GlobalsFirstTimerKeepsFullDwordAndCurrentValue()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        uint first = 341, second = 342, fixedValue = 127;
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, scene.Hooks(), new FactoryPostPopTypes { FirstTimerFactory = 85, SecondTimer = 87, FixedFrame = 126, CurrentFirstTimer = () => first, CurrentSecondTimer = () => second, CurrentFixedFrame = () => fixedValue });
        bookkeeping.Depth++;
        factory.PostPop(Source, 1);
        Assert.Equal(new[] { "A:40", "R:50:5:1056964608", "D:1" }, scene.Events);
        first = 85;
        second = 87;
        scene.Events.Clear();
        scene.Alloc = 0;
        bookkeeping.Depth++;
        factory.PostPop(Source, 1);
        Assert.Equal(new[] { "A:40", "R:50:2:1090519040", "A:40", "R:50:5:1056964608", "D:1" }, scene.Events);
    }

    /// <summary>첫 Regular 생성 콜백이 바꾼 두 번째 TYPE DWORD를 다음 비교에서 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void GlobalsConstructorChangesNextTimerGlobal()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        uint first = 85, second = 86, fixedValue = 127;
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        var add = hooks.AddRegular;
        hooks.AddRegular = (sid, @event, delay) =>
        {
            add!(sid, @event, delay);
            if (@event == 2)
            {
                second = 85;
            }
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks, new FactoryPostPopTypes { FirstTimerFactory = 85, SecondTimer = 87, FixedFrame = 126, CurrentFirstTimer = () => first, CurrentSecondTimer = () => second, CurrentFixedFrame = () => fixedValue });
        bookkeeping.Depth++;
        factory.PostPop(Source, 1);
        Assert.Equal(85u, second);
        Assert.Equal(7, scene.Events.Count);
        Assert.Equal("R:50:2:1090519040", scene.Events[1]);
        Assert.Equal("R:50:1:1056964608", scene.Events[3]);
        Assert.Equal("D:1", scene.Events[^1]);
    }

    /// <summary>첫 할당 실패의 콜백 변경도 다음 TYPE 비교에 보존한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void GlobalsFailedReservationChangesNextTimerGlobal()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        uint first = 85, second = 86, fixedValue = 127, calls = 0;
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.ReserveRegular = () =>
        {
            scene.Events.Add("A");
            ++calls;
            if (calls == 1)
            {
                second = 85;
                return false;
            }

            return true;
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks, new FactoryPostPopTypes { FirstTimerFactory = 85, SecondTimer = 87, FixedFrame = 126, CurrentFirstTimer = () => first, CurrentSecondTimer = () => second, CurrentFixedFrame = () => fixedValue });
        bookkeeping.Depth++;
        factory.PostPop(Source, 1);
        Assert.Equal(new[] { "A", "A", "R:50:1:1056964608", "A", "R:50:5:1056964608", "D:1" }, scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>일반 Factory의 공통 콜백 뒤 현재 제외 TYPE가 같으면 가상80/프레임 장치를 선택하지 않는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void GlobalsCommonChangesFrameExclusionBeforeCount()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        uint first = 85, second = 87, fixedValue = 127;
        var carrierHooks = scene.DamageableHooks(pop);
        var common = carrierHooks.Base;
        carrierHooks.Base = (sid, flags) =>
        {
            common(sid, flags);
            fixedValue = 85;
        };
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, carrierHooks);
        var hooks = scene.Hooks();
        hooks.SetFrame = null;
        hooks.VirtualCount = _ => throw new InvalidOperationException("현재 제외된 일반 Factory는 Count하지 않는다");
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks, new FactoryPostPopTypes { FirstTimerFactory = 85, SecondTimer = 87, FixedFrame = 126, CurrentFirstTimer = () => first, CurrentSecondTimer = () => second, CurrentFixedFrame = () => fixedValue });
        bookkeeping.Depth++;
        factory.PostPop(Source, 0);
        Assert.Equal(new[] { "D:0" }, scene.Events);
        Assert.Equal(7u, Get(scene.Pool.Slot(Source), 36));
    }

    /// <summary>장기 참조를 지정하지 않은 기존 독립 호출자는 생성 시 값 사본을 유지한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void GlobalsOwnedNumbersRetainIndependentCopy()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 85, 1, 0, 0, 0, 0, 7, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var numbers = new FactoryPostPopTypes();
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, scene.Hooks(), numbers);
        numbers.FirstTimerFactory = 86;
        numbers.SecondTimer = 85;
        bookkeeping.Depth++;
        factory.PostPop(Source, 1);
        Assert.Equal(2u, scene.Alloc);
        Assert.Equal("R:50:2:1090519040", scene.Events[1]);
        Assert.Equal("R:50:5:1056964608", scene.Events[3]);
        Assert.Equal(0u, bookkeeping.Depth);
    }

    /// <summary>제외 번호도 BYTE로 좁히지 않으며 다음 helper 호출에서 현재 DWORD를 다시 읽는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void GlobalsFrameExclusionKeepsFullDword()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 0, 85, 0, 0, 0, 0, 7, 0, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        uint first = 85, second = 87, fixedValue = 341, reads = 0;
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.VirtualCount = _ =>
        {
            ++reads;
            return 0u;
        };
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks, new FactoryPostPopTypes { FirstTimerFactory = 85, SecondTimer = 87, FixedFrame = 126, CurrentFirstTimer = () => first, CurrentSecondTimer = () => second, CurrentFixedFrame = () => fixedValue });
        factory.AdjustFrame(Source);
        Assert.Equal(2u, reads);
        Assert.Equal(0u, Get(scene.Pool.Slot(Source), 36));
        fixedValue = 85;
        Put(scene.Pool.AllocatedBytes(Source), 36, 7);
        factory.AdjustFrame(Source);
        Assert.Equal(2u, reads);
        Assert.Equal(7u, Get(scene.Pool.Slot(Source), 36));
    }

    /// <summary>실제 기계어가 만든 독립 행을 같은 C# 풀/해시의 전체 출력과 비교한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void NativeBodyAndDynamicFinder()
    {
        string[][] rows = X86Fixture.Read("factorypostpop");
        int count = 0;
        // 두 판본 모두 현재 입력을 재생한다.
        foreach (bool patch in new[] { true, false })
        {
            var scene = new Scene(patch);
            // 기대값을 구현으로 재계산하지 않고 fixture를 읽는다.
            foreach (string[] row in rows)
            {
                if (row[0] != (patch ? "originals" : "originalCD") || row.Length != 6)
                {
                    continue;
                }

                uint[] input = row[1].Split(',').Select(part => uint.Parse(part, CultureInfo.InvariantCulture)).ToArray();
                scene.Prepare(input);
                var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
                var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
                var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
                var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, scene.Hooks());
                Assert.False(pop.HandlesFactory(Source));
                pop.SetFactoryPostPop(factory);
                Assert.True(pop.HandlesFactory(Source));
                if (patch)
                {
                    bookkeeping.Depth++;
                    pop.PostPopVirtual(Source, input[2]);
                }
                else
                {
                    pop.Activate(Source, input[2]);
                }

                Assert.Equal(0u, bookkeeping.Depth);
                string[] expected = row.Skip(2).ToArray();
                string[] output = scene.Output();
                for (int k = 0; k < expected.Length; ++k)
                {
                    Assert.True(output[k] == expected[k], $"공장 후처리 {row[0]} 입력 {row[1]} 칸 {k} 불일치: 출력 {output[k]} / 기대 {expected[k]}");
                }
                ++count;
            }
        }

        Assert.Equal(rows.Length, count);
        Assert.True(count > 500);
    }

    /// <summary>다른 풀과 필요한 가상 효과 누락은 쓰기 전에 거부한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void RequiresEffectsAndSamePool()
    {
        var scene = new Scene(true);
        scene.Prepare(new uint[] { 1, 85, 1, 0, 1, 0, 7, 7, 0, 0, 0 });
        var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
        var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
        var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
        var hooks = scene.Hooks();
        hooks.RegionOwnership = null;
        var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
        pop.SetFactoryPostPop(factory);
        string before = Convert.ToHexStringLower(scene.Pool.Slot(Source).ToArray());
        bool rejected = false;
        try
        {
            pop.Activate(Source, 1);
        }
        catch (ArgumentException)
        {
            rejected = true;
        }

        Assert.True(rejected);
        Assert.Equal(before, Convert.ToHexStringLower(scene.Pool.Slot(Source).ToArray()));
        Assert.Empty(scene.Events);
        Assert.Equal(0u, bookkeeping.Depth);
        var other = new Scene(true);
        var foreign = new RawPostPop(other.Pool, other.BaseTypes, new RawPostPopState());
        rejected = false;
        try
        {
            foreign.SetFactoryPostPop(factory);
        }
        catch (ArgumentException)
        {
            rejected = true;
        }

        Assert.True(rejected);
        pop.SetFactoryPostPop(null);
        Assert.False(pop.HandlesFactory(Source));
    }

    /// <summary>Factory의 구간 판단에서 실제 SquidFrame의 dirty/공간 갱신과 raw 프레임 쓰기를 연결한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void ActualFrameEffectAndPoolContract()
    {
        // 양 판본의 프레임 폭과 단계 바이트를 동시에 확인한다.
        foreach (bool patch in new[] { true, false })
        {
            var scene = new Scene(patch);
            scene.Prepare(new uint[] { 0, 86, 0, 0, 0, 1, 7, 7, 0, 0, 0 });
            scene.Pool.AllocatedBytes(Source)[patch ? 33 : 31] = 1;
            var observed = new List<uint>();
            var frameHooks = new SquidFrameHooks
            {
                FrameSize = (_, _) => new float[] { 1, 1 },
                Update = (sid, flags) =>
                {
                    Assert.Equal(Source, sid);
                    Assert.Equal(0u, flags);
                    observed.Add(Get(scene.Pool.Slot(sid), scene.FrameOffset, patch ? 4 : 1));
                },
                Unpop = (_, _) => throw new InvalidOperationException("같은 해시 단계는 Unpop하지 않는다"),
                Pop = (_, _, _, _) => throw new InvalidOperationException("같은 해시 단계는 Pop하지 않는다"),
            };
            var frame = new SquidFrame(scene.Pool, scene.Types, frameHooks);
            var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
            var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
            var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, RawFactoryPostPop.MakeFactoryPostPopHooks(scene.Pool, frame, scene.Hooks()));
            pop.SetFactoryPostPop(factory);
            pop.Activate(Source, 0);
            Assert.Equal(2u, Get(scene.Pool.Slot(Source), scene.FrameOffset, patch ? 4 : 1));
            Assert.Equal(new uint[] { 7, 2 }, observed);
            Assert.Equal(0u, bookkeeping.Depth);
            var other = new Scene(patch);
            bool rejected = false;
            try
            {
                _ = RawFactoryPostPop.MakeFactoryPostPopHooks(other.Pool, frame, new FactoryPostPopHooks());
            }
            catch (ArgumentException)
            {
                rejected = true;
            }

            Assert.True(rejected);
        }
    }

    /// <summary>실제 Factory 가상84는 진단 복귀 뒤 현재 상태를 저장하고 가상88을 한 번 호출한다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void LevelDiagnosticReturnAndCurrentService()
    {
        // 양 판본에서 현재 진단 장치 교체와 진단 중 슬롯 변경의 순서를 확인한다.
        foreach (bool patch in new[] { true, false })
        {
            var scene = new Scene(patch);
            scene.Prepare(new uint[] { 0, 86, 0, 0, 0, 1, 7, 7, 0, 0, 0 });
            var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
            var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
            var hooks = scene.Hooks();
            hooks.UpdateState = sid =>
            {
                Assert.Equal(Source, sid);
                scene.Events.Add("U:" + scene.Pool.Slot(sid)[11].ToString(CultureInfo.InvariantCulture));
            };
            var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
            Func<string, bool>? current = null;
            var level = new RawSquidLevel(scene.Pool, scene.Types, SquidLevelDiagnostics.MakeSquidLevelAssertion(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, (expression, _, _) =>
            {
                if (current is null)
                {
                    throw new InvalidOperationException("검사의 현재 미설치");
                }

                current(expression);
            }));
            factory.SetState(level, Source, 3);
            Assert.Equal(new[] { "U:96" }, scene.Events);
            current = expression =>
            {
                Assert.Equal("0 <= n && n < 8", expression);
                scene.Events.Add("A:1");
                scene.Pool.AllocatedBytes(Source)[11] = 0x14;
                return true;
            };
            factory.SetState(level, Source, 8);
            Assert.Equal(new[] { "U:96", "A:1", "U:20" }, scene.Events);
            current = _ =>
            {
                scene.Events.Add("A:2");
                scene.Pool.AllocatedBytes(Source)[11] = 0x0a;
                return true;
            };
            factory.SetState(level, Source, 0xffffffff);
            Assert.Equal((uint)7, level.Get(Source));
            Assert.Equal("U:234", scene.Events[^1]);
            Assert.Equal("A:2", scene.Events[^2]);
            current = null;
            string before = Convert.ToHexStringLower(scene.Pool.Slot(Source).ToArray());
            int count = scene.Events.Count;
            bool rejected = false;
            try
            {
                factory.SetState(level, Source, 8);
            }
            catch (InvalidOperationException)
            {
                rejected = true;
            }

            Assert.True(rejected);
            Assert.Equal(before, Convert.ToHexStringLower(scene.Pool.Slot(Source).ToArray()));
            Assert.Equal(count, scene.Events.Count);
            var missing = new RawSquidLevel(scene.Pool, scene.Types, SquidLevelDiagnostics.MakeSquidLevelAssertion(patch ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072, null));
            factory.SetState(missing, Source, 2);
            Assert.Equal(2u, level.Get(Source));
            Assert.Equal("U:74", scene.Events[^1]);
            rejected = false;
            try
            {
                factory.SetState(missing, Source, 8);
            }
            catch (InvalidOperationException)
            {
                rejected = true;
            }

            Assert.True(rejected);
        }
    }

    /// <summary>양판본 Factory+84는 base 레벨 저장 뒤 표시 호출 실패 전에 먼저 쓴 상태 바이트가 남는다. 2026-11-19 0.624에서 추가했다.</summary>
    [Fact]
    public void SetterMissingDisplayFailsAfterLevelWrittenPrefix()
    {
        // 양 판본의 같은 실제 풀/부모에 연결한다.
        foreach (bool patch in new[] { true, false })
        {
            var scene = new Scene(patch);
            scene.Prepare(new uint[] { 0, 86, 0, 0, 0, 1, 7, 7, 0, 0, 0 });
            var bookkeeping = new RawPostPopState(); bookkeeping.Suppressed = true;
            var pop = new RawPostPop(scene.Pool, scene.BaseTypes, bookkeeping);
            var damageable = new RawCarrierPostPop(scene.Pool, scene.Types, new CarrierPostPopState { Boss = true }, scene.DamageableHooks(pop));
            var hooks = scene.Hooks();
            hooks.UpdateState = null;
            var factory = new RawFactoryPostPop(scene.Pool, scene.Hash, scene.Types, scene.Frames, pop, damageable, scene.State, hooks);
            int diagnostics = 0;
            var level = new RawSquidLevel(scene.Pool, scene.Types, _ =>
            {
                ++diagnostics;
                scene.Pool.AllocatedBytes(Source)[11] = 0x14;
            });
            // 정상 레벨과 진단 복귀 뒤 저장 모두 실제 표시 호출보다 먼저 실패한다.
            foreach (uint value in new uint[] { 3, 8 })
            {
                scene.Pool.AllocatedBytes(Source)[11] = 0;
                bool failed = false;
                try
                {
                    factory.SetState(level, Source, value);
                }
                catch (InvalidOperationException)
                {
                    failed = true;
                }

                Assert.True(failed);
                Assert.Equal(value == 3 ? 0x60 : 0x14, scene.Pool.Slot(Source)[11]);
            }

            Assert.Equal(1, diagnostics);
        }
    }
}
