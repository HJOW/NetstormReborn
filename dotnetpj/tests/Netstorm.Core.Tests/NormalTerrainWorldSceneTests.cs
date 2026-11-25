// 실제 일반 지형 소비자의 생성자·도장·군집·공간·지역·장부와 서로 다른 난수 저장소 대조다.
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// R4-7c F3 보류 검사 중 일반 지형 바인딩 전체 월드 장면이다. cpppj tests/NormalTerrainWorldBindingTests.cpp의
// 세 장면(두 섬 실제 생성·도장·군집 후처리, 새 분기 실제 농지 생성·owner·HP·게임 난수, 누락 효과·미복원 분기 거부)과
// 같은 합성 타입·실제 월드로 두 판본을 실행하고 게임·CRT 난수 소비와 HP·지역·장부를 대조한다.
// DN에 없던 장면 재현과 표면 월드 CD 조립 연결만 더하고 제품 본체는 바꾸지 않는다.
// 1.000에서 패치판을, 1.001에서 CD판을 이었다.
public sealed class NormalTerrainWorldSceneTests
{
    /// <summary>같은 원본 10.78 진단 경계다. 미복원 하위 효과는 명시 실패로 둔다.</summary>
    /// <param name="name">실패한 효과 이름이다.</param>
    private static void Unrestored(string name) => throw new InvalidOperationException("일반 지형 장면의 미복원 효과: " + name);

    /// <summary>같은 원본 슬롯의 리틀 엔디언 DWORD·WORD를 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint ReadSlot(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint value = 0;
        // 폭만큼 아래 바이트부터 쌓는 반복문이다.
        for (int i = 0; i < width; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
    }

    // 같은 실제 장기 장치와 두 섬·수명 긴 전역을 빌린다.
    private sealed class Scene
    {
        /// <summary>같은 원본 판본이다.</summary>
        public readonly OriginalEdition Edition;
        /// <summary>패치판인가.</summary>
        public bool Patch => Edition == OriginalEdition.Patch1078;
        /// <summary>같은 실제 합성 타입 표다.</summary>
        public readonly List<OriginalTypeRecord> Types;
        /// <summary>같은 실제 합성 프레임 표다.</summary>
        public readonly List<RiftTypeFrames> Frames;
        /// <summary>같은 실제 표면 월드다.</summary>
        public readonly RawSurfaceWorld World;
        /// <summary>같은 실제 월드 핵심 장치다.</summary>
        public readonly RawWorldCore Core;
        /// <summary>현재 청크 지도다.</summary>
        public readonly ChunkMap Chunks = new();
        /// <summary>같은 실제 섬 기록 바이트다.</summary>
        public readonly byte[] IslandBytes = new byte[RawIslandStorage.Bytes];
        /// <summary>같은 실제 종료 owner 모드다.</summary>
        public readonly FortFinishOwnerState Modes = new();
        /// <summary>같은 실제 거주 구역 목록이다.</summary>
        public readonly RawPostPopList Residences = new();
        /// <summary>현재 지역 revision·변경 전역이다.</summary>
        public uint Revision;
        /// <summary>현재 지역 변경 전역이다.</summary>
        public uint Changed;
        /// <summary>같은 실제 섬 목록이다.</summary>
        public readonly RawIslandRuntime Islands;
        /// <summary>실제 지면·농지 배치 파이프라인 상태다.</summary>
        public readonly CanonPlacementPipelineState PipelineState = new();
        /// <summary>같은 실제 파이프라인 프레임 자료다.</summary>
        public readonly PriestPlainCanonType[] PlainFrames;
        /// <summary>같은 실제 파이프라인 기준점 자료다.</summary>
        public readonly PriestTypeHotspot[] Hotspots;
        /// <summary>같은 실제 파이프라인 셰이프 자료다.</summary>
        public readonly List<SquidDisplayShape> Shapes;
        /// <summary>같은 실제 배치 파이프라인이다.</summary>
        public readonly RawCanonPlacementPipeline Pipeline;
        /// <summary>같은 실제 게임 난수 저장소다.</summary>
        public readonly GameRandom Random = new(0x12345678);
        /// <summary>도장이 소비하는 공유 CRT 상자다.</summary>
        public readonly UIntBox Crt = new() { Value = 7 };
        /// <summary>실제 지면 문자 mapping 전역이다.</summary>
        public readonly TerrainContextGlobals Globals = new();
        /// <summary>수명이 긴 초기 캐시 상태다.</summary>
        public readonly TerrainStampState Stamp = new();
        /// <summary>실제 지면·edgeFarm 번호와 현재 owner다.</summary>
        public readonly FortRandomPlacementState Placement = new();
        /// <summary>같은 실제 농지 군집 상태다.</summary>
        public readonly FarmClusterState Farm = new();
        /// <summary>바인딩에 연결한 상위 입력 묶음이다. 군도 모드 전환에 쓴다.</summary>
        public readonly TerrainGenerationInputs GenerationInputs = new()
        {
            Enabled = new RawCheckingFlag(0u),
            Span = new IntBox(),
            Minimum = new IntBox(),
            Maximum = new IntBox(),
            Percent = new UIntBox(),
            Archipelago = new UIntBox(),
        };
        /// <summary>실제 펏 조각 타입 상자다.</summary>
        public readonly UIntBox PieceBox = new() { Value = 107u };
        /// <summary>같은 실제 지형 바인딩이다.</summary>
        public readonly RawNormalTerrainWorldBinding Binding;
        /// <summary>같은 실제 HP 조횟다.</summary>
        public readonly RawSquidReward Hp;
        /// <summary>같은 실제 운반체 후처리 상태다.</summary>
        public readonly CarrierPostPopState CarrierState = new() { LoadingDepth = 1 };
        /// <summary>같은 실제 운반체 후처리다.</summary>
        public readonly RawCarrierPostPop Carrier;
        /// <summary>같은 실제 무작위 배치다.</summary>
        public readonly RawFortRandomPlacement Single;
        /// <summary>같은 실제 군집 배치다.</summary>
        public readonly RawFarmCluster Cluster;
        /// <summary>같은 실제 도장 월드 효과다.</summary>
        public readonly TerrainStampHooks StampHooks;
        /// <summary>진단 관찰 수다.</summary>
        public int Diagnostics;
        /// <summary>같은 실제 보상 상태다.</summary>
        public readonly SquidRewardState HpState = new();
        /// <summary>같은 실제 패치 SP 저장소다.</summary>
        public readonly RawSpStore Sp;

        /// <summary>같은 합성 자산·실제 월드로 장면을 조립한다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        /// <param name="randomFarm">농지 생성자 flags1을 무작위 프레임용으로 바꾸는지 여부다.</param>
        public Scene(OriginalEdition edition, bool randomFarm = false)
        {
            Edition = edition;
            int count = edition == OriginalEdition.Patch1078 ? OriginalConstructors.TypeCount : 171;
            Types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToList();
            Frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToList();
            var shapes = Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToList();
            var terrainCodes = new List<RiftFrameCode>();
            // 각 16방향의 16후보는 실제 fallback 조건을 함께 만족한다.
            for (byte side = (byte)'A'; side <= (byte)'P'; side++)
            {
                // 같은 A변형의 16물리프레임을 넣는 반복문이다.
                for (byte variant = (byte)'A'; variant <= (byte)'P'; variant++)
                {
                    terrainCodes.Add(new RiftFrameCode(side, (byte)'A', 1, 0));
                }
            }

            var pieceCodes = new List<RiftFrameCode>();
            // 영역 패턴 칸의 실제 회전된 방향·P변형·번호 검색을 지원하는 반복문이다.
            for (byte side = (byte)'A'; side <= (byte)'P'; side++)
            {
                pieceCodes.Add(new RiftFrameCode(side, (byte)'P', 1, 0));
            }

            Frames[97] = new RiftTypeFrames(terrainCodes);
            Frames[121] = new RiftTypeFrames(terrainCodes);
            Frames[107] = new RiftTypeFrames(pieceCodes);
            // 지면과 농지는 실제 원본 생성자를 사용하며 농지의 Damageable 가상표를 base로 대신하지 않는다.
            foreach (uint type in new uint[] { 97u, 121u })
            {
                OriginalTypeRecord record = Types[(int)type];
                record.ConstructorAddress = OriginalConstructors.Address(edition, (int)type);
                record.Flags1 = type == 97u ? 0x28000803u : randomFarm ? 0x00100012u : 0x00200012u;
                record.Flags2 = type == 97u ? 2u : 0x80000u;
                record.FootX = 1;
                record.FootY = 1;
                record.MaxHitPoints = 73;
                var shape = new SquidDisplayShape { FrameCount = 256, Loaded = true };
                // 같은 합성 SHP 메타 256프레임을 채우는 반복문이다.
                for (int i = 0; i < 256; i++)
                {
                    shape.Frames.Add(new SquidDisplayFrame { Width = 20, Height = 16, HotspotX = 8, HotspotY = 12, CellWidth = 1, CellHeight = 1 });
                }

                shapes[(int)type] = shape;
            }

            var storage = new RawSurfaceStorage(edition, Types, Frames, shapes);
            World = new RawSurfaceWorld(edition, Types, 97u, storage, Frames);
            Core = World.Core();
            Islands = new RawIslandRuntime(
                Core.Pool, Core.Hash, IslandBytes, Modes, Residences,
                () => Revision, value => Revision = value, () => Changed, value => Changed = value,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (sid, player) => Core.Owner.Set(sid, player),
                    Assertion = _ => Diagnostics++,
                });
            Islands.Reset();
            Assert.Equal(0u, Islands.Add(2u, 3));
            Assert.Equal(1u, Islands.Add(3u, 4));
            // 두 실제 섬은 서로 다른 청크·원래 owner·현재 owner를 유지한다.
            for (int region = 0; region < 2; region++)
            {
                ref ChunkEntry chunk = ref Chunks.At(3 + region, 3);
                chunk.Island = (byte)region;
                chunk.Side = (sbyte)'A';
                chunk.Record = [(byte)3, (byte)1, (byte)0, (byte)(7 + region * 13), (byte)0, (byte)0];
            }

            var plain = new PriestPlainCanonType[Types.Count];
            // 기존 파이프라인의 래퍼형 프레임으로 같은 불변 입력을 보관하는 반복문이다.
            for (int type = 0; type < plain.Length; type++)
            {
                plain[type] = new PriestPlainCanonType { Frames = Frames[type] };
            }

            PlainFrames = plain;
            var hotspots = new PriestTypeHotspot[Types.Count];
            // 모든 타입 번호마다 빈 기준점을 넣는 반복문이다.
            for (int i = 0; i < hotspots.Length; i++)
            {
                hotspots[i] = new PriestTypeHotspot(0, 0);
            }

            Hotspots = hotspots;
            Shapes = shapes;

            Array.Copy(new uint[] { 107u, 82u, 94u, 157u, 131u, 129u, 140u, 142u }, PipelineState.Geometry.PatternTypes, 8);
            PipelineState.Permission.GraphReady = 0;
            Array.Fill(PipelineState.Permission.Alliances, 1u);
            PipelineState.Placement.LocalPlayer = 1;
            var data = new CanonPlacementPipelineData
            {
                Types = Types,
                Frames = plain,
                Shapes = shapes,
                Hotspots = hotspots,
                Spots = Core.Spots,
                Islands = Core.Hash.BorrowEntries(0),
                Regions = new CanonPlacementIslandRegion[128],
            };
            Pipeline = new RawCanonPlacementPipeline(Core.Pool, Core.Hash, data, Core.Bookkeeping, new RawPostPopList(), new ContainedFinderState(), PipelineState);
            Sp = new RawSpStore(Random, () => 0x12345678u);
            Hp = new RawSquidReward(Core.Pool, Types, Core.Bookkeeping, HpState, edition == OriginalEdition.Patch1078 ? Sp : null);
            Carrier = new RawCarrierPostPop(Core.Pool, Types, CarrierState,
                RawCarrierPostPop.MakeCarrierPostPopHooks(Core.Pool, Core.PostPop,
                    _ => Unrestored("carrier"),
                    (x, y, owner) => World.SetSupportOwner(x, y, owner)));
            Core.PostPop.SetCarrierPostPop(Carrier);
            var diagnostics = new NormalTerrainDiagnostics
            {
                Context = _ => Diagnostics++,
                Stamp = _ => Diagnostics++,
                Region = _ => Diagnostics++,
                Farm = _ => Diagnostics++,
                Postpass = _ => Diagnostics++,
            };
            Binding = new RawNormalTerrainWorldBinding(Chunks, Islands, Globals, Placement, Stamp, Types, Core.Pool, PieceBox, Frames, diagnostics);
            GenerationInputs.Battle = new RawCheckingFlag(Core.OwnerMode.Battle);
            GenerationInputs.Mission = new RawCheckingFlag(Core.BridgeState.Mission);
            Binding.BindGenerationInputs(Farm, GenerationInputs);
            FortRandomPlacementHooks farmHooks = RawFortRandomPlacement.MakeFortRandomPlacementHooks(Core.Pool, Pipeline, Core.Factory, Hp, new FortRandomPlacementHooks());
            farmHooks.ChunkRegion = (x, y) => unchecked((sbyte)Chunks.At(x, y).Island);
            farmHooks.RegionOwner = region => Islands.Owner(region);
            farmHooks.SetOwner = (sid, owner) => Core.Owner.Set(sid, owner);
            farmHooks.Unpop = (sid, flags) => World.Unpop(sid, flags);
            farmHooks.Repop = (sid, flags) => World.Repop(sid, flags);
            farmHooks.Pop = (sid, x, y, flags) => World.Pop(sid, x, y, flags);
            Single = new RawFortRandomPlacement(Core.Pool, Core.Hash, Core.Spots, Types, Frames, Random, Placement, farmHooks);
            var clusterHooks = new FarmClusterHooks { Assertion = _ => Unrestored("cluster") };
            RawFarmCluster.MakeFarmClusterPlacementHook(Core.Pool, Single, clusterHooks);
            Cluster = new RawFarmCluster(Core.Pool, Chunks, Islands, Placement, Farm, clusterHooks);
            StampHooks = RawTerrainWorldBinding.MakeTerrainStampWorldHooks(World, Stamp, _ => Diagnostics++, new TerrainStampHooks
            {
                ChunkRegion = (x, y) => unchecked((sbyte)Chunks.At(x, y).Island),
                Assertion = _ => Diagnostics++,
            });
        }

        /// <summary>예상 결과를 만드는 모형 없이 기존 FrameAt만 호출해 같은 게임·CRT 난수 소비를 사전 관찰한다.</summary>
        /// <param name="request">관찰할 생성 요청이다.</param>
        /// <param name="expected">사전 관찰용 게임 난수다.</param>
        /// <param name="expectedCrt">사전 관찰용 CRT 상자다.</param>
        /// <param name="expectedState">사전 관찰용 도장 상태다.</param>
        /// <returns>실제 초기 도장의 프레임 요청 수다.</returns>
        public uint ExpectedFrames(NormalTerrainRequest request, GameRandom expected, UIntBox expectedCrt, TerrainStampState expectedState)
        {
            var context = new RawTerrainContext(Edition, request.Owner, request.SelectedRegion, request.Bounds, Globals, _ => Diagnostics++);
            var growth = new RawTerrainGrowth(context, Chunks, Globals);
            growth.RunNormal();
            context.MarkEdges('x', (byte)'=');
            var chooser = new RawTerrainStamp(Core.Pool, Types, Frames, context.Map, expected,
                () => expectedCrt.Value, value => expectedCrt.Value = value, expectedState, StampHooks);
            uint count = 0;
            // 실제 초기 도장의 Y·X 순서에서 매 실제 지면 프레임 요청만 관찰하는 반복문이다.
            for (int y = context.Map.Bounds[1]; y < context.Map.Bounds[3]; y++)
            {
                // 생성·Pop 없이 실제 FrameAt이 게임·CRT 저장소만 갱신하는 반복문이다.
                for (int x = context.Map.Bounds[0]; x < context.Map.Bounds[2]; x++)
                {
                    byte code = context.Codes[x * 256 + y];
                    if (context.Map.Aliases[code] == 'x' && context.Map.TypeMapping[code] != 0)
                    {
                        chooser.FrameAt(x, y);
                        count++;
                    }
                }
            }

            return count;
        }

        /// <summary>도장 상태 사본을 만든다. 게임·CRT 사전 관찰과 실제 실행을 분리한다.</summary>
        /// <returns>같은 값의 도장 상태다.</returns>
        public static TerrainStampState CopyStamp(TerrainStampState state) => new()
        {
            Variants = state.Variants?.ToArray(),
            IslandType = state.IslandType,
            EdgeType = state.EdgeType,
            FringeType = state.FringeType,
            NoRegion = state.NoRegion,
            FringePrefixFlags = state.FringePrefixFlags,
        };
    }

    /// <summary>같은 Factory·풀·공간·지역·장부로 두 섬을 실제 성장·도장·군집 후처리하며 캐시는 첫 요청만 소비한다.</summary>
    [Fact]
    public void ActualNormalGenerationAndFarmRetype()
    {
        // 두 판본의 프레임 폭·선택 정책과 같은 현재 지역 바이트를 각각 실행하는 반복문이다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            RunNormalGenerationAndFarmRetype(edition);
        }
    }

    /// <summary>한 판본의 두 섬 실제 생성·도장·군집 후처리를 실행한다.</summary>
    /// <param name="edition">원본 판본이다.</param>
    private static void RunNormalGenerationAndFarmRetype(OriginalEdition edition)
    {
        bool patch = edition == OriginalEdition.Patch1078;
        var scene = new Scene(edition);
        RawWorldCore core = scene.Core;
        scene.World.BeginLoad();
        Assert.Same(core.Pool, scene.Islands.Pool());
        Assert.Same(core.Bookkeeping, scene.Hp.Bookkeeping);
        uint total = 0;
        uint farmCount = 0;
        // 첫·두 번째 요청의 CRT cache는 하나이며 선택 지역만 바뀌는 반복문이다.
        for (int region = 0; region < 2; region++)
        {
            var request = new NormalTerrainRequest(1u, new ChunkRect(3, 3, 5, 4), region, NormalTerrainBranch.Normal);
            var expected = new GameRandom(scene.Random.State());
            var expectedCrt = new UIntBox { Value = scene.Crt.Value };
            TerrainStampState expectedState = Scene.CopyStamp(scene.Stamp);
            uint count = scene.ExpectedFrames(request, expected, expectedCrt, expectedState);
            Assert.True(count > 0);
            total += count;
            uint previousCrt = scene.Crt.Value;
            int last = scene.Binding.Generate(request, scene.Random, scene.Crt, scene.StampHooks, (area, farmRegion, quantity) => scene.Cluster.Place(area, farmRegion, quantity));
            Assert.NotEqual(0, last);
            Assert.Equal(expected.State(), scene.Random.State());
            Assert.Equal(expectedCrt.Value, scene.Crt.Value);
            Assert.Equal(expectedState.Variants, scene.Stamp.Variants);
            Assert.True(region == 0 ? scene.Crt.Value != previousCrt : scene.Crt.Value == previousCrt);
        }

        List<SurfaceSnapshot> objects = scene.World.Objects();
        Assert.Equal((int)total, objects.Count);
        Assert.Equal(0, scene.Diagnostics);
        // 실제 생성한 모든 SID의 지면·농지 가상표와 현재 해시 단계·지역 WORD·HP를 확인하는 반복문이다.
        foreach (SurfaceSnapshot snapshot in objects)
        {
            ReadOnlySpan<byte> raw = core.Pool.Slot(snapshot.Sid);
            uint region = ReadSlot(raw, 8, 2);
            Assert.True(region < 2, $"장면 지역 범위: region={region}");
            Assert.True(snapshot.Object.Owner == 0 && snapshot.Object.X >= 48 && snapshot.Object.X < 80 && snapshot.Object.Y >= 48 && snapshot.Object.Y < 64);
            uint expectedVtable = snapshot.Object.Type == 121u ? (patch ? 0x50a8e8u : 0x500b10u) : (patch ? 0x501dd0u : 0x5058d8u);
            Assert.Equal(expectedVtable, ReadSlot(raw, 0, 4));
            Assert.Equal(snapshot.Sid, scene.World.Find(snapshot.Object.X, snapshot.Object.Y, snapshot.Object.Type));
            uint level = ReadSlot(raw, patch ? 33 : 31, 1);
            bool found = false;
            int inspected = 0;
            // 실제 같은 버킷의 SID 체인 포함 여부와 순회 한도다.
            for (int current = core.Hash.Bucket((int)level, snapshot.Object.X, snapshot.Object.Y);
                current != 0 && inspected++ < core.Pool.Capacity;
                current = (int)ReadSlot(core.Pool.Slot(current), 4, 2))
            {
                if (current == snapshot.Sid)
                {
                    found = true;
                    break;
                }
            }

            Assert.True(found);
            if (raw[10] == 121)
            {
                Assert.Equal(73u, ReadSlot(raw, 26, patch ? 4 : 2));
                farmCount++;
            }
            else
            {
                Assert.True(raw[10] == 97);
            }
        }

        Assert.True(farmCount > 0);
        Assert.Equal(total, core.Bookkeeping.GlobalCounts[97]);
        Assert.Equal(0u, core.Bookkeeping.Depth);
        Assert.Equal(0u, scene.PipelineState.Placement.ForcePlacement);
        scene.World.FinishLoad();
        Assert.Equal(2u, scene.Islands.Owner(0));
        Assert.Equal(3u, scene.Islands.Owner(1));
    }

    /// <summary>새 배치 분기는 실제 농지 생성·HP·게임 프레임 난수·현재 섬 owner를 같은 공간·장부에 쓴다.</summary>
    [Fact]
    public void ActualFarmCreateOwnerHpAndGameRandom()
    {
        // terrain-frame 교체와 구별한 synthetic random-frame 입력으로 양판본의 실제 생성 소비자를 확인하는 반복문이다.
        foreach (OriginalEdition edition in new[] { OriginalEdition.Patch1078, OriginalEdition.Cd1072 })
        {
            RunFarmCreateOwnerHpAndGameRandom(edition);
        }
    }

    /// <summary>한 판본의 새 배치 분기를 실행한다.</summary>
    /// <param name="edition">원본 판본이다.</param>
    private static void RunFarmCreateOwnerHpAndGameRandom(OriginalEdition edition)
    {
        bool patch = edition == OriginalEdition.Patch1078;
        var scene = new Scene(edition, randomFarm: true);
        RawWorldCore core = scene.Core;
        scene.World.BeginLoad();
        var context = new RawTerrainContext(edition, 1, 0, new ChunkRect(3, 3, 4, 4), scene.Globals, _ => scene.Diagnostics++);
        context.Codes[50 * 256 + 53] = (byte)'x';
        context.Regions[50 * 256 + 53] = 0;
        var stamp = new RawTerrainStamp(core.Pool, scene.Types, scene.Frames, context.Map, scene.Random,
            () => scene.Crt.Value, value => scene.Crt.Value = value, scene.Stamp, scene.StampHooks);
        stamp.Stamp((sbyte)'x');
        Assert.NotEqual(0, context.Map.LastSid);
        var single = new RawFortRandomPlacement(core.Pool, core.Hash, core.Spots, scene.Types, scene.Frames, scene.Random, scene.Placement,
            RawFortRandomPlacement.MakeFortRandomPlacementHooks(core.Pool, scene.Pipeline, core.Factory, scene.Hp, new FortRandomPlacementHooks
            {
                ChunkRegion = (x, y) => unchecked((sbyte)scene.Chunks.At(x, y).Island),
                RegionOwner = region => scene.Islands.Owner(region),
                SetOwner = (sid, owner) => core.Owner.Set(sid, owner),
                Unpop = (sid, flags) => scene.World.Unpop(sid, flags),
                Repop = (sid, flags) => scene.World.Repop(sid, flags),
                Pop = (sid, x, y, flags) => scene.World.Pop(sid, x, y, flags),
            }));
        var local = new GameRandom(77);
        var expected = new GameRandom(scene.Random.State());
        uint expectedFrame = 240 + expected.Next(16u);
        uint previousCrt = scene.Crt.Value;
        uint created = single.Place(local, new FortRandomPlaceRequest(121u, 50f, 53f, 0, 0u, 0u));
        Assert.NotEqual(0u, created);
        ReadOnlySpan<byte> raw = core.Pool.Slot((int)created);
        Assert.Equal((int)created, scene.World.Find(50f, 53f, 121u));
        Assert.Equal(expected.State(), scene.Random.State());
        Assert.Equal(77u, local.State());
        Assert.Equal(previousCrt, scene.Crt.Value);
        Assert.Equal(expectedFrame, ReadSlot(raw, patch ? 36 : 34, patch ? 4 : 1));
        Assert.Equal(2, raw[patch ? 34 : 32]);
        Assert.Equal(73u, ReadSlot(raw, 26, patch ? 4 : 2));
        Assert.Equal(1u, core.Bookkeeping.GlobalCounts[121]);
        Assert.Equal(0, core.Bookkeeping.TotalCost);
        Assert.Equal(0u, core.Bookkeeping.Depth);
        Assert.Equal(0, scene.Diagnostics);
        Assert.Same(core.Pool, scene.Pipeline.Pool);
        scene.World.FinishLoad();
    }

    /// <summary>필수 효과·다른 월드·미복원 분기를 생성 전 거부하고 원본을 기본 생성자·허용값으로 대신하지 않는다.</summary>
    [Fact]
    public void RejectsMissingEffectsAndUnrestoredBranches()
    {
        var scene = new Scene(OriginalEdition.Patch1078);
        RawWorldCore core = scene.Core;
        string before = scene.World.Report();
        uint random = scene.Random.State();
        uint crt = scene.Crt.Value;
        Assert.ThrowsAny<Exception>(() => scene.Binding.Generate(new NormalTerrainRequest(1u, new ChunkRect(3, 3, 4, 4), 0, NormalTerrainBranch.Archipelago), scene.Random, scene.Crt, scene.StampHooks, (area, region, quantity) => scene.Cluster.Place(area, region, quantity)));
        Assert.ThrowsAny<Exception>(() => scene.Binding.Generate(new NormalTerrainRequest(1u, new ChunkRect(3, 3, 4, 4), 0, NormalTerrainBranch.UnrestoredParent), scene.Random, scene.Crt, scene.StampHooks, (area, region, quantity) => scene.Cluster.Place(area, region, quantity)));
        scene.GenerationInputs.Archipelago.Value = 1u;
        Assert.ThrowsAny<Exception>(() => scene.Binding.PostprocessFarms(scene.Farm, 0, (area, region, quantity) => scene.Cluster.Place(area, region, quantity)));
        scene.GenerationInputs.Archipelago.Value = 0u;
        scene.Placement.EdgeFarmType = 122u;
        Assert.ThrowsAny<Exception>(() => scene.Binding.PostprocessFarms(scene.Farm, 0, (area, region, quantity) => scene.Cluster.Place(area, region, quantity)));
        scene.Placement.EdgeFarmType = 121u;
        Assert.Throws<ArgumentException>(() => RawTerrainWorldBinding.MakeTerrainStampWorldHooks(scene.World, scene.Stamp, null, new TerrainStampHooks
        {
            ChunkRegion = (x, y) => unchecked((sbyte)scene.Chunks.At(x, y).Island),
            Assertion = _ => scene.Diagnostics++,
        }));
        var other = new Scene(OriginalEdition.Patch1078);
        Assert.Throws<ArgumentException>(() => new RawFarmCluster(core.Pool, scene.Chunks, other.Islands, scene.Placement, scene.Farm, new FarmClusterHooks { Assertion = _ => scene.Diagnostics++ }));
        var foreignPool = new RawSidPool(32768, true, null, OriginalEdition.Patch1078);
        var foreignHash = new RawSquidHash();
        var foreignPipeline = new RawCanonPlacementPipeline(foreignPool, foreignHash,
            new CanonPlacementPipelineData
            {
                Types = scene.Types,
                Frames = scene.PlainFrames,
                Shapes = scene.Shapes,
                Hotspots = scene.Hotspots,
                Spots = new byte[65536],
                Islands = foreignHash.BorrowEntries(0),
                Regions = new CanonPlacementIslandRegion[128],
            },
            new RawPostPopState(), new RawPostPopList(), new ContainedFinderState(), new CanonPlacementPipelineState());
        Assert.Throws<ArgumentException>(() => RawFortRandomPlacement.MakeFortRandomPlacementHooks(core.Pool, foreignPipeline, core.Factory, scene.Hp, new FortRandomPlacementHooks()));
        Assert.Equal(before, scene.World.Report());
        Assert.Equal(random, scene.Random.State());
        Assert.Equal(crt, scene.Crt.Value);
        Assert.Null(scene.Stamp.Variants);
        byte[] record = [3, 1, 0, 7, 0, 0];
        Assert.Equal(3u, scene.Binding.PatternCount(record));
        record[0] |= 0xc0;
        Assert.Equal(3u, scene.Binding.PatternCount(record));
        Assert.Same(core.Pool, scene.Single.Pool());
    }
}
