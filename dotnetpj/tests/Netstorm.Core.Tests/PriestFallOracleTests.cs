using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Core.Display;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

// 원본 사제 상태/낙하 자료를 실제 HP·현재 프레임·외부 효과 순서에 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>원본 PATCH6752상태/456낙하 입력과 실제 현재 대여 계약이다.</summary>
public sealed class PriestFallOracleTests
{
    /// <summary>원본 실행기가 사용하는 동일 client SID다.</summary>
    private const int Target = 50;
    /// <summary>같은188칸 TYPE와 공통 풀·HP 제공자를 소유하는 원본 입력 장면이다.</summary>
    private sealed class Scene
    {
        /// <summary>모든 실제 모듈이 빌리는 같은 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, false);
        /// <summary>현재 원본 입력의 실제 TYPE 표다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>HP/상태가 읽는 같은 지면 spot 지도다.</summary>
        public readonly byte[] Spots = new byte[65536];
        /// <summary>조회가 SP 초기화/난수를 소비하지 않는지도 확인하는 실제 저장소다.</summary>
        public readonly RawSpStore Store = new(new GameRandom(), () => 1u);
        /// <summary>현재 약화 전역이다.</summary>
        public readonly SquidRewardState HpMode = new();
        /// <summary>상태/낙하가 공유하는 현재 실제 HP 제공자다.</summary>
        public RawSquidReward Hp;
        /// <summary>동일 표의 원본 상태 조회기다.</summary>
        public RawPriestState Priest;
        /// <summary>원본과 같이5~50을 실제 client 정책으로 확보한다.</summary>
        public Scene()
        {
            // 실제 할당 순서를 고정하며 지정 raw 슬롯으로 대신하지 않는다.
            for (int sid = 5; sid <= Target; sid++) Assert.Equal(sid, Pool.Allocate(SidAllocation.Client));
            Hp = new RawSquidReward(Pool, Types, new RawPostPopState(), HpMode, Store);
            Priest = new RawPriestState(Pool, Hp, Types, Spots);
        }
        /// <summary>현재 원문 TYPE를 같은 실제 HP와 상태의 생성 시 사본에 연결한다.</summary>
        public void Rebuild()
        {
            Hp = new RawSquidReward(Pool, Types, new RawPostPopState(), HpMode, Store);
            Priest = new RawPriestState(Pool, Hp, Types, Spots);
        }
    }
    /// <summary>프레임 연결 검사에서 실제 표시 계산이 보낸 영역만 센다.</summary>
    private sealed class FrameSink : SquidDisplaySink
    {
        /// <summary>표시가 무효화한 영역 수다.</summary>
        public int Changes;
        /// <summary>표시 계산을 억제하지 않는다.</summary>
        public override bool Suppressed() => false;
        /// <summary>원문 표시 경로가 도달한 영역을 관측한다.</summary>
        public override void Invalidate(DisplayRect rect, uint flags) => Changes++;
    }
    /// <summary>정렬되지 않은 원본 DWORD 필드만 쓴다.</summary>
    private static void Put(Span<byte> raw, int offset, uint value) => BinaryPrimitives.WriteUInt32LittleEndian(raw[offset..], value);
    /// <summary>float 좌표의 정확한 DWORD 비트를 같은 필드에 쓴다.</summary>
    private static void Position(Span<byte> raw, float x, float y) { Put(raw, 14, BitConverter.SingleToUInt32Bits(x)); Put(raw, 18, BitConverter.SingleToUInt32Bits(y)); }

    /// <summary>미연결이었던 원본 사제상태6752행의 반환·HP전환·전체 raw·외부 효과를 직접 대조한다.</summary>
    [Fact]
    public void OriginalState6752MatchesWholeRawAndReachedEffects()
    {
        string[][] all = X86Fixture.Read("prieststate"); Assert.Equal(20256, all.Length);
        string[][] rows = all.Where(row => row[1] == "originals").ToArray(); Assert.Equal(6752, rows.Length);
        var scene = new Scene();
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(18, row.Length); OriginalTypeRecord type = scene.Types[158]; type.MaxHitPoints = X86Fixture.Int(row[2]); type.Flags1 = 0x69012; type.Flags2 = X86Fixture.UInt(row[4]);
            scene.HpMode.PriestQuarterHp = row[3] != "0";
            byte spot = (byte)X86Fixture.UInt(row[7]);
            // 체크무늬는 원본 입력이며 두 축의 거의 올림/절삭 차이는 제품 코드가 판단한다.
            for (int y = 0; y < 256; y++)
                // 원본 y/x 순서의 현재 spot만 채운다.
                for (int x = 0; x < 256; x++) scene.Spots[y * 256 + x] = (byte)(spot ^ (((x + y) & 1) != 0 ? 6 : 0));
            Span<byte> raw = scene.Pool.AllocatedBytes(Target); raw.Fill(0xab); Put(raw, 0, 0x0050f210); raw[10] = 158; raw[11] = 0;
            raw[40] = (byte)X86Fixture.UInt(row[6]); raw[34] = (byte)X86Fixture.UInt(row[13]);
            Put(raw, 14, X86Fixture.UInt(row[8])); Put(raw, 18, X86Fixture.UInt(row[9])); Put(raw, 26, X86Fixture.UInt(row[5])); scene.Rebuild();
            var mode = new PriestHitPointMode { Authority = row[11] != "0" }; mode.RecoveryAllowed[X86Fixture.UInt(row[13])] = row[12] != "0";
            var events = new List<string>();
            // 원본의 명시 Unpop/Repop 경계는 실제 당시 HP/좌표를 읽고 입력으로 지정한 변이만 적용한다.
            void Record(char marker, int sid, uint flags)
            {
                ReadOnlySpan<byte> current = scene.Pool.Slot(sid);
                events.Add($"{marker}:{sid}:{flags}:{scene.Priest.CurrentHitPoints(sid)}:{BinaryPrimitives.ReadUInt32LittleEndian(current[14..])}:{BinaryPrimitives.ReadUInt32LittleEndian(current[18..])}");
                if (marker == 'U' && row[14] != "-")
                {
                    Span<byte> target = scene.Pool.AllocatedBytes(sid); Put(target, 26, unchecked((uint)X86Fixture.Int(row[14]))); Position(target, 30.75f, 31.9f);
                }
            }
            if (row[0] == "Set") scene.Priest.SetHitPoints(Target, X86Fixture.Int(row[10]), mode, new PriestHitPointHooks { Unpop = (sid, flags) => Record('U', sid, flags), Repop = (sid, flags) => Record('R', sid, flags) });
            else Assert.Equal(row[15] == "1", row[0] == "Ground" ? scene.Priest.GroundImmobile(Target) : scene.Priest.Immobile(Target));
            Assert.Equal(row[16], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(X86Fixture.Bytes(row[17]), scene.Pool.Slot(Target).ToArray()); return true;
        });
        Assert.False(scene.Store.Initialized);
    }

    /// <summary>낙하456입력의 현재 권한 재조회·확보 실패·좌표/프레임 부작용 뒤 재조회와 전체 raw를 대조한다.</summary>
    [Fact]
    public void OriginalFall456MatchesEventsCurrentAuthorityAndRaw()
    {
        string[][] all = X86Fixture.Read("priestfall"); Assert.Equal(1368, all.Length);
        string[][] rows = all.Where(row => row[0] == "originals").ToArray(); Assert.Equal(456, rows.Length);
        var scene = new Scene(); RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        var choices = new PriestFallFrames[188]; var surfaces = new ushort[65536];
        X86Fixture.CheckRows(rows, row =>
        {
            Assert.Equal(16, row.Length); bool begin = row[1] == "Begin", allocated = row[3] != "0"; uint change = X86Fixture.UInt(row[9]);
            var mode = new PriestFallState { Authority = row[2] != "0" }; var events = new List<string>();
            scene.Types[158].MaxHitPoints = 200; scene.Types[158].Flags1 = 0x69012; scene.Types[158].Flags2 = 0x210000;
            frames[158] = new RiftTypeFrames([new RiftFrameCode((byte)X86Fixture.UInt(row[7]), 80, 1, 0), new RiftFrameCode(74, 80, 1, 0)]); choices[158] = new PriestFallFrames(3, 4);
            Array.Fill(scene.Spots, (byte)X86Fixture.UInt(row[6])); Array.Fill(surfaces, (ushort)X86Fixture.UInt(row[8]));
            Span<byte> raw = scene.Pool.AllocatedBytes(Target); raw.Clear(); Put(raw, 0, 0x0050f210); raw[10] = 158; raw[34] = 1; raw[40] = (byte)X86Fixture.UInt(row[4]);
            Put(raw, 26, X86Fixture.UInt(row[5])); Put(raw, 14, X86Fixture.UInt(row[10])); Put(raw, 18, X86Fixture.UInt(row[11])); scene.Rebuild();
            // 원본 관측기에 공급한 명시 효과 입력만 바꾸며 반환/분기 기대값은 재계산하지 않는다.
            void Change(string stage)
            {
                Span<byte> current = scene.Pool.AllocatedBytes(Target);
                if (begin)
                {
                    if ((stage == "shield" && change == 1) || (stage == "new" && change == 2)) mode.Authority = !mode.Authority;
                    if (stage == "repop" && change == 3) Position(current, 30.75f, 31.9f);
                    return;
                }
                if (stage == "shield" && change == 1) Put(current, 36, 1);
                if ((stage is "set" or "advance") && (change is 2 or 4)) Position(current, 30.75f, 31.9f);
                if (stage == "land" && (change is 3 or 4)) { current[40] = 0; Put(current, 26, 150); }
            }
            var hooks = new PriestFallHooks
            {
                EnsureShield = sid => { events.Add($"S:{sid}"); Change("shield"); },
                ReserveRegular = () => { events.Add("A:40"); Change("new"); return allocated; },
                AddSharedRegular = (sid, ev, payload) => events.Add($"R:{sid}:{ev}:{BitConverter.SingleToUInt32Bits(payload)}"),
                Unpop = (sid, flags) => events.Add($"U:{sid}:{flags}"),
                Repop = (sid, flags) => { events.Add($"P:{sid}:{flags}"); Change("repop"); },
                Sound = (sid, x, y) => events.Add($"W:{sid}:{BitConverter.SingleToUInt32Bits(x)}:{BitConverter.SingleToUInt32Bits(y)}"),
                SetFrame = (sid, frame, flags) => { events.Add($"F:{sid}:{frame}:{flags}"); Change(frame == 4 ? "land" : "set"); },
                AdvanceFrame = (sid, steps, flags) => { events.Add($"N:{sid}:{steps}:{flags}"); Change("advance"); },
                ClearShield = sid => events.Add($"X:{sid}"),
            };
            var body = new RawPriestFall(scene.Pool, scene.Priest, frames, choices, surfaces, mode, hooks); string result = "-";
            if (begin) body.Begin(Target); else result = BitConverter.SingleToUInt32Bits(body.Handle(Target, RawPriestFall.FallEvent, 0xabcdef01, BitConverter.UInt32BitsToSingle(0x7fc12345))).ToString();
            Assert.Equal(row[12], events.Count == 0 ? "-" : string.Join(';', events)); Assert.Equal(X86Fixture.Bytes(row[13]), scene.Pool.Slot(Target).ToArray());
            Assert.Equal(row[14], result); Assert.Equal(row[15] != "0", mode.Authority); return true;
        });
        Assert.False(scene.Store.Initialized);
    }

    /// <summary>생성 시 사본과 실제 현재 대여를 구별하고 잘못된 재연결은 기존 보기를 보존한다.</summary>
    [Fact]
    public void StateBorrowsCurrentTypesOnlyWhenExplicitlyBound()
    {
        var scene = new Scene(); scene.Types[158].MaxHitPoints = 200; scene.Types[158].Flags1 = 0x69012; scene.Types[158].Flags2 = 0;
        Span<byte> raw = scene.Pool.AllocatedBytes(Target); raw.Clear(); raw[10] = 158; Put(raw, 0, 0x0050f210); scene.Rebuild();
        scene.Types[158].Flags2 = 0x200000; Assert.False(scene.Priest.GroundImmobile(Target));
        scene.Priest.BindCurrentTypes(scene.Types); Assert.True(scene.Priest.GroundImmobile(Target));
        scene.Types[158] = new OriginalTypeRecord(); Assert.False(scene.Priest.GroundImmobile(Target));
        Assert.Throws<ArgumentException>(() => scene.Priest.BindCurrentTypes(new OriginalTypeRecord[187]));
        Assert.False(scene.Priest.GroundImmobile(Target)); scene.Priest.BindCurrentTypes(null); Assert.False(scene.Priest.GroundImmobile(Target));
    }

    /// <summary>실제9기록의 중립/첫/끝 owner와 상위WORD 신전도 허용하며 인접 바이트는 쓰지 않는다.</summary>
    [Theory]
    [InlineData(OriginalEdition.Patch1078, 0xb4, 0x7c)]
    [InlineData(OriginalEdition.Cd1072, 0xac, 0x74)]
    public void RecoveryUsesNineCurrentRecordsAndFullDword(OriginalEdition edition, int stride, int field)
    {
        byte[] records = Enumerable.Repeat((byte)0xa5, 9 * stride).ToArray(); var mode = new PriestHitPointMode { PlayerRecords = records };
        // 실제 원문의 허용 owner0/1/8만 같은 현재 장기 기록에서 조회한다.
        foreach (uint owner in new uint[] { 0, 1, 8 })
        {
            Span<byte> target = records.AsSpan((int)owner * stride + field); Put(target, 0, 0); Assert.False(mode.GetRecoveryAllowed(owner, edition));
            Put(target, 0, 0x10000); byte[] before = records.ToArray(); Assert.True(mode.GetRecoveryAllowed(owner, edition)); Assert.Equal(before, records);
        }
        Assert.Throws<ArgumentOutOfRangeException>(() => mode.GetRecoveryAllowed(9, edition));
        mode.PlayerRecords = new byte[10 * stride]; Assert.Throws<ArgumentException>(() => mode.GetRecoveryAllowed(1, edition));
        mode.PlayerRecords = null; mode.RecoveryAllowed[8] = true; Assert.True(mode.GetRecoveryAllowed(8, edition));
    }
    /// <summary>같은 실제 공유Form/Kernel·공장/owner·보호막 조회를 합성해 낙하 재예약과 최종 자기 삭제까지 확인한다.</summary>
    [Fact]
    public void ActualSharedProcessAndShieldLifetimeCompose()
    {
        var scene = new Scene(); var types = scene.Types;
        // 현재 원본 내장 생성자만 입력으로 명시하며 다른 자산으로 대신하지 않는다.
        foreach (int type in new[] { 2, 158, 167 }) types[type].ConstructorAddress = OriginalConstructors.Address(type);
        types[158].Flags1 = 0x69012; types[158].Flags2 = 0x210000; types[158].MaxHitPoints = 200;
        types[158].FootX = types[158].FootY = types[167].FootX = types[167].FootY = 1; Array.Fill(scene.Spots, (byte)6);
        Span<byte> raw = scene.Pool.AllocatedBytes(Target); raw.Clear(); Put(raw, 0, 0x0050f210); raw[10] = 158; raw[34] = 1; raw[40] = 32;
        Put(raw, 26, 20); Position(raw, 20.75f, 21.9f); scene.Rebuild();
        var hash = new RawSquidHash(); var unpop = new RawUnpop(scene.Pool, hash, scene.Spots); var destroy = new RawSquidDestroy(scene.Pool, unpop, types);
        var kernel = new Kernel(); var processMode = new SquidProcessState { Now = 12.5 }; RawPriestFall? fall = null;
        var host = new SquidProcessHost(scene.Pool, types, kernel, destroy, processMode,
            (sid, ev, count, payload) => RawPriestFall.MakePriestFallHandler(scene.Pool, fall!)(sid, ev, count, payload));
        var factory = new RawSquidFactory(scene.Pool, types); host.BindFormFactory(factory);
        var lookup = new RawPriestForcefield(scene.Pool, hash, types, new PriestForcefieldState());
        int shieldSid = 0, creates = 0, clears = 0, sounds = 0;
        PriestShieldHooks effects = RawPriestShield.MakePriestShieldCreationHooks(scene.Pool, factory,
            new SquidOwner(scene.Pool, types, new RawPostPopState(), new SquidOwnerMode()), new PriestShieldHooks
            {
                Pop = (sid, x, y, flags) =>
                {
                    Assert.Equal(0u, flags); creates++; shieldSid = sid;
                    // cpp 합성 검사와 같은 명시 공간 경계다. 판단은 실제 현재 finder가 수행한다.
                    Span<byte> born = scene.Pool.AllocatedBytes(sid); Position(born, x, y); born[11] = 0;
                    ref ushort head = ref hash.Bucket(0, x, y); BinaryPrimitives.WriteUInt16LittleEndian(born[4..], head); head = (ushort)sid;
                },
                ReserveSound = () => false, LoadSound = _ => 0, AttachSound = (_, _, _, _) => Assert.Fail("미선택 보호막 소리"),
                NoticeSound = _ => Assert.Fail("로딩 중 미선택 안내"), Translate = _ => "", Tell = _ => Assert.Fail("로딩 중 미선택 안내문"),
            });
        var shield = new RawPriestShield(scene.Pool, lookup, new PriestShieldState { LocalPlayer = 1, LoadingDepth = 1 }, effects);
        RiftTypeFrames[] frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[158] = new RiftTypeFrames([new RiftFrameCode(65, 80, 1, 0), new RiftFrameCode(74, 80, 1, 0)]);
        var choices = new PriestFallFrames[188]; choices[158] = new PriestFallFrames(3, 4); var surfaces = new ushort[65536];
        PriestFallHooks hooks = RawPriestFall.MakePriestFallHooks(host, shield, lookup, (sid, flags) =>
        {
            Assert.Equal(shieldSid, sid); Assert.Equal(0u, flags); clears++;
            hash.Bucket(0, 20.75f, 21.9f) = 0; scene.Pool.AllocatedBytes(sid)[11] = 4; scene.Pool.Release(sid);
        }, new PriestFallHooks
        {
            Unpop = (_, _) => Assert.Fail("미선택 권한 공간 해제"), Repop = (_, _) => Assert.Fail("미선택 권한 재등록"),
            Sound = (sid, x, y) => { Assert.Equal(Target, sid); Assert.Equal(20.75f, x); Assert.Equal(21.9f, y); sounds++; },
            // cpp 합성 검사와 같은 명시 프레임 경계다. 공통 Set/Advance 자체는 별도 실제 SHP 검사로 대조한다.
            SetFrame = (sid, frame, flags) =>
            {
                Assert.Equal(0u, flags); Span<byte> current = scene.Pool.AllocatedBytes(sid); Put(current, 36, frame == 3 ? 1u : 0u);
                if (frame == 4) { current[40] = 0; Put(current, 26, 150); }
            },
            AdvanceFrame = (sid, steps, flags) => { Assert.Equal(Target, sid); Assert.Equal(1u, steps); Assert.Equal(0u, flags); },
        });
        bool boss = true; Action<int, uint, float> actualAdd = hooks.AddSharedRegular!;
        hooks.AddSharedRegular = (sid, ev, payload) => { actualAdd(sid, ev, payload); boss = false; };
        fall = new RawPriestFall(scene.Pool, scene.Priest, frames, choices, surfaces, new PriestFallState { Authority = true, SharedAuthority = () => boss }, hooks);
        fall.Begin(Target); Assert.False(boss); RegularProcess process = Assert.IsType<RegularProcess>(host.FindEvent(Target, RawPriestFall.FallEvent));
        Assert.Equal(1, kernel.Size()); Assert.Equal(61, scene.Pool.Slot(process.Form())[10]); Assert.Equal(1, creates); Assert.Equal(shieldSid, lookup.Find(Target));
        Assert.Equal(12.5, process.Time()); Assert.Equal(0u, process.Count()); Assert.Equal(0.01f, process.Payload());
        processMode.Now = 13; boss = true; fall.Begin(Target); Assert.False(boss); RegularProcess newest = Assert.IsType<RegularProcess>(host.FindEvent(Target, RawPriestFall.FallEvent));
        Assert.NotSame(process, newest); Assert.Equal(13d, newest.Time()); Assert.Equal(2, kernel.Size()); Assert.Equal(1, creates); Assert.Equal(2, sounds);
        host.Kill(newest, 0x10); Assert.Equal(1, kernel.Size()); Assert.Same(process, host.FindEvent(Target, RawPriestFall.FallEvent));
        processMode.Now = 12.5; kernel.RunFrame(); process = Assert.IsType<RegularProcess>(host.FindEventMasked(Target, 0xffff, RawPriestFall.FallEvent));
        Assert.Equal(1u, process.Count()); Assert.Equal(0.1f, process.Payload()); Assert.Equal(12.5 + (double)0.1f, process.Time());
        processMode.Now = process.Time() - 0.000001; kernel.RunFrame(); Assert.Equal(1u, process.Count()); Assert.Equal(1, creates);
        processMode.Now = process.Time(); Array.Fill(surfaces, ushort.MaxValue); kernel.RunFrame();
        Assert.Equal(0, kernel.Size()); Assert.Null(host.FindEvent(Target, RawPriestFall.FallEvent)); Assert.Equal(1, clears); Assert.Equal(0, lookup.Find(Target)); Assert.False(scene.Store.Initialized);
    }

    /// <summary>현재 방향/VT의 분배와 실제 동일 프레임 지정/진행기를 연결하고 다른 실체는 거부한다.</summary>
    [Fact]
    public void CurrentDirectionDispatchAndActualFrameAdapterCompose()
    {
        var scene = new Scene(); Span<byte> raw = scene.Pool.AllocatedBytes(Target); raw.Clear(); raw[10] = 158; raw[11] = 4; Put(raw, 0, 0x0050f210);
        var frames = Enumerable.Range(0, 188).Select(_ => new RiftTypeFrames([])).ToArray();
        frames[158] = new RiftTypeFrames(Enumerable.Range(0, 4).Select(index => new RiftFrameCode(74, 80, (byte)index, 0)));
        var choices = new PriestFallFrames[188]; var hooks = new PriestFallHooks
        {
            EnsureShield = _ => Assert.Fail("J방향 미선택 보호막"), ReserveRegular = () => false, AddSharedRegular = (_, _, _) => Assert.Fail("J방향 미선택 예약"),
            Unpop = (_, _) => { }, Repop = (_, _) => { }, Sound = (_, _, _) => { }, SetFrame = (_, _, _) => { }, AdvanceFrame = (_, _, _) => { }, ClearShield = _ => { },
        };
        var shapes = Enumerable.Range(0, 188).Select(_ => new SquidDisplayShape()).ToArray(); shapes[158].Loaded = true; shapes[158].FrameCount = 4;
        // 실제 SHP 헤더 네 칸을 같은 표시기에서 조회하고 저장된 공간 단계도 같은1로 맞춘다.
        for (int index = 0; index < 4; index++) shapes[158].Frames.Add(new SquidDisplayFrame { Width = 10, Height = 10, CellWidth = 1, CellHeight = 1 });
        var display = new SquidDisplay(scene.Pool.Edition, scene.Types, shapes, new FrameSink(), new SquidDisplayView()); raw[33] = 1;
        var frameHooks = new SquidFrameHooks
        {
            FrameSize = (sid, frame) => display.FrameSize(scene.Pool.Edition, scene.Pool.Slot(sid), frame),
            Update = (sid, flags) => display.Update(scene.Pool.Slot(sid), flags),
            Unpop = (_, _) => Assert.Fail("동일 단계 미선택 Unpop"), Pop = (_, _, _, _) => Assert.Fail("동일 단계 미선택 Pop"),
        };
        var setter = new SquidFrame(scene.Pool, scene.Types, frameHooks); var advance = new RawFrameAdvance(scene.Pool, frames, setter);
        PriestFallHooks actual = RawPriestFall.MakePriestFallFrameHooks(setter, advance, hooks);
        actual.SetFrame!(Target, 2, 0); actual.AdvanceFrame!(Target, uint.MaxValue, 0); Assert.Equal(1, BinaryPrimitives.ReadInt32LittleEndian(raw[36..]));
        var fall = new RawPriestFall(scene.Pool, scene.Priest, frames, choices, new ushort[65536], new PriestFallState(), actual);
        Assert.True(fall.TryFall(Target)); Assert.True(RawPriestFall.MakePriestFallWalker(scene.Pool, fall)(Target));
        RegularHandler handler = RawPriestFall.MakePriestFallHandler(scene.Pool, fall);
        Assert.Throws<ArgumentException>(() => handler(Target, 0x25a, 0, 3));
        Assert.Equal(-7f, RawPriestFall.MakePriestFallHandler(scene.Pool, fall, (_, _, _, _) => -7)(Target, 0x25a, 0, 3));
        Put(raw, 0, 0x501dd0); Assert.Equal(3f, handler(Target, 0x25a, 0, 3)); Assert.Throws<ArgumentException>(() => RawPriestFall.MakePriestFallWalker(scene.Pool, fall)(Target));
        Assert.True(RawPriestFall.MakePriestFallWalker(scene.Pool, fall, _ => true)(Target));
        Assert.Throws<ArgumentException>(() => RawPriestFall.MakePriestFallHandler(new RawSidPool(24000, false), fall));
        Assert.Throws<ArgumentException>(() => RawPriestFall.MakePriestFallFrameHooks(new SquidFrame(scene.Pool, scene.Types, frameHooks), advance, hooks));
        Assert.Throws<ArgumentException>(() => new RawPriestFall(scene.Pool, scene.Priest, frames, choices, new ushort[65536], new PriestFallState(), new PriestFallHooks()));
    }

}
