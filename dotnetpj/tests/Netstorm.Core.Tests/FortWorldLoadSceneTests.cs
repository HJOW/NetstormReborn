// Fort 적재 바인딩의 전체 월드 장면(Builder·일반지형·저장후처리·Graph0) 재현 대조다.
using Netstorm.Assets;
using Netstorm.Core.Original;
using Netstorm.Core.Simulation;
using Netstorm.Tests;

namespace Netstorm.Core.Tests;

// R4-7c F3 보류 검사 중 Fort 적재 바인딩 전체 월드 장면이다. cpppj tests/FortWorldLoadBindingTests.cpp의
// ActualSelectedWorldAndTwoRandomStores와 같은 합성 188/171 타입·프레임·셰이프·실제 월드로 같은 자식 적재·지형 생성·
// Money·배지·기술·Graph 재구성을 순서대로 실행하고 fortworldload-builder-consumer-x86.tsv 2행과 대조한다.
// DN에 없던 fixture 연결과 장면 재현만 더하며 제품 코드는 바꾸지 않는다. 1.000에서 추가됐다.
public sealed class FortWorldLoadSceneTests
{
    /// <summary>같은 원본 10.78 진단 경계다. 미복원 하위 효과는 명시 실패로 둔다.</summary>
    /// <param name="name">실패한 효과 이름이다.</param>
    private static void Unrestored(string name) => throw new InvalidOperationException("선택 Fort 월드 장면의 미복원 효과: " + name);

    /// <summary>같은 원본 바이트를 소문자로 직렬화한다.</summary>
    /// <param name="bytes">직렬화할 바이트다.</param>
    /// <returns>소문자 16진 문자열이다.</returns>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexStringLower(bytes);

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

    /// <summary>현재 청크의 전체 3072바이트를 원본 X열 우선 배치와 동일하게 직렬화한다.</summary>
    /// <param name="chunks">직렬화할 청크 지도다.</param>
    /// <returns>소문자 16진 문자열이다.</returns>
    private static string ChunkHex(ChunkMap chunks)
    {
        byte[] bytes = new byte[256 * 12];
        // 각 X열의 Y항목을 순서대로 쓰는 반복문이다.
        for (int x = 0; x < 16; x++)
        {
            // Y항목의 섬·방향·기록·순서를 12바이트에 쓰는 반복문이다.
            for (int y = 0; y < 16; y++)
            {
                ChunkEntry entry = chunks.At(x, y);
                int at = (x * 16 + y) * 12;
                bytes[at] = entry.Island;
                bytes[at + 1] = unchecked((byte)entry.Side);
                entry.Record.CopyTo(bytes, at + 2);
                byte[] ordinal = BitConverter.GetBytes(entry.Ordinal);
                ordinal.CopyTo(bytes, at + 8);
            }
        }

        return Hex(bytes);
    }

    /// <summary>물리 프레임 코드 4바이트를 원본 순서대로 직렬화한다.</summary>
    /// <param name="codes">직렬화할 코드 목록이다.</param>
    /// <returns>소문자 16진 문자열이다.</returns>
    private static string PuzzleHex(IReadOnlyList<RiftFrameCode> codes)
    {
        byte[] bytes = new byte[codes.Count * 4];
        // 네 바이트 물리코드를 순서대로 보존하는 반복문이다.
        for (int i = 0; i < codes.Count; i++)
        {
            bytes[4 * i] = codes[i].Side;
            bytes[4 * i + 1] = codes[i].Variant;
            bytes[4 * i + 2] = codes[i].Number;
            bytes[4 * i + 3] = codes[i].Flags;
        }

        return Hex(bytes);
    }

    // 같은 실제 장기 장치를 빌리며 어떤 상태도 초기화하지 않는다.
    private sealed class Scene
    {
        /// <summary>같은 원본 판본이다.</summary>
        public readonly OriginalEdition Edition;
        /// <summary>같은 실제 합성 타입 표다.</summary>
        public readonly List<OriginalTypeRecord> Types;
        /// <summary>같은 실제 합성 프레임 표다.</summary>
        public readonly List<RiftTypeFrames> Frames;
        /// <summary>같은 실제 합성 표시 셰이프다.</summary>
        public readonly List<SquidDisplayShape> Shapes;
        /// <summary>같은 실제 표면 월드다.</summary>
        public readonly RawSurfaceWorld World;
        /// <summary>같은 실제 월드 핵심 장치다.</summary>
        public readonly RawWorldCore Core;
        /// <summary>현재 빈 청크 지도다.</summary>
        public readonly ChunkMap Chunks = new();
        /// <summary>같은 실제 섬 기록 바이트다.</summary>
        public readonly byte[] IslandBytes = new byte[RawIslandStorage.Bytes];
        /// <summary>같은 실제 종료 owner 모드다.</summary>
        public readonly FortFinishOwnerState Modes = new();
        /// <summary>같은 실제 거주 구역 목록이다.</summary>
        public readonly RawPostPopList Residences = new();
        /// <summary>같은 실제 게임 난수 저장소다.</summary>
        public readonly GameRandom Random = new(0x12345678);
        /// <summary>같은 실제 패치 SP 저장소다.</summary>
        public readonly RawSpStore Sp;
        /// <summary>같은 실제 보상 상태다.</summary>
        public readonly SquidRewardState HpState = new();
        /// <summary>같은 실제 HP 조횟다.</summary>
        public readonly RawSquidReward Hp;
        /// <summary>수명이 긴 초기 캐시 상태다.</summary>
        public readonly TerrainStampState Stamp = new();
        /// <summary>도장이 소비하는 공유 CRT 상자다.</summary>
        public readonly UIntBox Crt = new() { Value = 7 };
        /// <summary>같은 실제 섬 목록이다.</summary>
        public readonly RawIslandRuntime Islands;
        /// <summary>같은 실제 지형 바인딩이다.</summary>
        public readonly RawNormalTerrainWorldBinding Binding;
        /// <summary>같은 실제 배치 파이프라인이다.</summary>
        public readonly RawCanonPlacementPipeline Pipeline;
        /// <summary>같은 실제 운반체 후처리다.</summary>
        public readonly RawCarrierPostPop Carrier;
        /// <summary>같은 실제 무작위 배치다.</summary>
        public readonly RawFortRandomPlacement Single;
        /// <summary>같은 실제 군집 배치다.</summary>
        public readonly RawFarmCluster Cluster;
        /// <summary>같은 실제 도장 월드 효과다.</summary>
        public readonly TerrainStampHooks StampHooks;
        /// <summary>현재 깊이다. 모든 child·장치에 같은 값을 전달한다.</summary>
        public uint Depth = 1;
        /// <summary>효과 호출 순서를 기록한다.</summary>
        public readonly List<string> Effects = [];
        /// <summary>같은 실제 종료 owner 몸체다.</summary>
        public readonly RawFortFinishOwner Finish;
        /// <summary>현재 Fort 입력이다.</summary>
        public readonly FortTemplate Fort = new();
        /// <summary>현재 Money 상태다.</summary>
        public readonly FortMoneyState MoneyState = new();
        /// <summary>현재 Money cursor다.</summary>
        public readonly FortMoneyCursor MoneyCursor = new() { Position = 0, End = 4 };
        /// <summary>현재 배지 stream이다.</summary>
        public readonly FortMetaStream BadgeStream;
        /// <summary>현재 기술 stream이다.</summary>
        public readonly FortMetaStream TechStream = new() { Bytes = Array.Empty<byte>(), Position = 0, End = 0 };
        /// <summary>현재 메타 상태다.</summary>
        public readonly FortMetaLoadState MetaState = new();
        /// <summary>실제 Money 적재 몸체다.</summary>
        public readonly RawFortMoneyLoad Money;
        /// <summary>실제 배지·기술 적재 몸체다.</summary>
        public readonly RawFortMetaLoad Meta;

        /// <summary>같은 합성 자산·실제 월드로 장면을 조립한다.</summary>
        /// <param name="edition">원본 판본이다.</param>
        public Scene(OriginalEdition edition)
        {
            Edition = edition;
            int count = edition == OriginalEdition.Patch1078 ? 188 : 171;
            Types = Enumerable.Range(0, count).Select(_ => new OriginalTypeRecord()).ToList();
            Frames = Enumerable.Range(0, count).Select(_ => new RiftTypeFrames([])).ToList();
            Shapes = Enumerable.Range(0, count).Select(_ => new SquidDisplayShape()).ToList();
            var terrainCodes = new List<RiftFrameCode>();
            // 각 방향의 A변형 16물리프레임을 넣는 반복문이다.
            for (byte side = (byte)'A'; side <= (byte)'P'; side++)
            {
                // 같은 방향의 선택 16개를 채우는 반복문이다.
                for (int number = 0; number < 16; number++)
                {
                    terrainCodes.Add(new RiftFrameCode(side, (byte)'A', 1, 0));
                }
            }

            var pieceCodes = new List<RiftFrameCode>();
            // 실제 P변형 번호를 모두 공급하는 반복문이다.
            for (byte side = (byte)'A'; side <= (byte)'P'; side++)
            {
                // 영역 패턴 칸의 회전된 방향·P변형 번호 검색을 지원하는 반복문이다.
                for (byte number = 0; number < 11; number++)
                {
                    pieceCodes.Add(new RiftFrameCode(side, (byte)'P', number, 0));
                }
            }

            Frames[107] = new RiftTypeFrames(pieceCodes);
            Frames[97] = new RiftTypeFrames(terrainCodes);
            Frames[121] = new RiftTypeFrames(terrainCodes);
            // 원본 97의 null·base 계약과 121의 실제 Damageable 생성자를 그대로 쓰는 반복문이다.
            foreach (uint type in new uint[] { 97u, 121u })
            {
                OriginalTypeRecord record = Types[(int)type];
                record.ConstructorAddress = OriginalConstructors.Address(edition, (int)type);
                record.Flags1 = type == 97u ? 0x28000803u : 0x00200012u;
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

                Shapes[(int)type] = shape;
            }

            var storage = new RawSurfaceStorage(edition, Types, Frames, Shapes);
            World = new RawSurfaceWorld(edition, Types, 97u, storage, Frames);
            Core = World.Core();
            Islands = new RawIslandRuntime(
                Core.Pool, Core.Hash, IslandBytes, Modes, Residences,
                () => 0u, _ => { }, () => 0u, _ => { },
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (sid, player) => Core.Owner.Set(sid, player),
                    Assertion = _ => Unrestored("island"),
                });
            Islands.Reset();
            Sp = new RawSpStore(Random, () => 0x12345678u);
            Hp = new RawSquidReward(Core.Pool, Types, Core.Bookkeeping, HpState, edition == OriginalEdition.Patch1078 ? Sp : null);
            var plain = new PriestPlainCanonType[count];
            // 기존 파이프라인의 래퍼형 프레임으로 같은 불변 입력을 보관하는 반복문이다.
            for (int type = 0; type < plain.Length; type++)
            {
                plain[type] = new PriestPlainCanonType { Frames = Frames[type] };
            }

            var hotspots = new PriestTypeHotspot[count];
            // 모든 타입 번호마다 빈 기준점을 넣는 반복문이다.
            for (int i = 0; i < hotspots.Length; i++)
            {
                hotspots[i] = new PriestTypeHotspot(0, 0);
            }

            var pipelineState = new CanonPlacementPipelineState();
            Array.Copy(new uint[] { 107u, 82u, 94u, 157u, 131u, 129u, 140u, 142u }, pipelineState.Geometry.PatternTypes, 8);
            pipelineState.Permission.GraphReady = 0;
            Array.Fill(pipelineState.Permission.Alliances, 1u);
            pipelineState.Placement.LocalPlayer = 1;
            var data = new CanonPlacementPipelineData
            {
                Types = Types,
                Frames = plain,
                Shapes = Shapes,
                Hotspots = hotspots,
                Spots = Core.Spots,
                Islands = Core.Hash.BorrowEntries(0),
                Regions = new CanonPlacementIslandRegion[128],
            };
            Pipeline = new RawCanonPlacementPipeline(Core.Pool, Core.Hash, data, Core.Bookkeeping, new RawPostPopList(), new ContainedFinderState(), pipelineState);
            var carrierState = new CarrierPostPopState { LoadingDepth = Depth };
            Carrier = new RawCarrierPostPop(Core.Pool, Types, carrierState,
                RawCarrierPostPop.MakeCarrierPostPopHooks(Core.Pool, Core.PostPop,
                    _ => Unrestored("carrier"),
                    (x, y, owner) => World.SetSupportOwner(x, y, owner)));
            Core.PostPop.SetCarrierPostPop(Carrier);
            var globals = new TerrainContextGlobals();
            var placement = new FortRandomPlacementState();
            var farm = new FarmClusterState();
            var diagnostics = new NormalTerrainDiagnostics
            {
                Context = _ => Unrestored("terrain-context"),
                Stamp = _ => Unrestored("terrain-stamp"),
                Region = _ => Unrestored("terrain-region"),
                Farm = _ => Unrestored("farm"),
                Postpass = _ => Unrestored("postpass"),
            };
            var pieceBox = new UIntBox { Value = 107u };
            Binding = new RawNormalTerrainWorldBinding(Chunks, Islands, globals, placement, stamp: Stamp, types: Types, pool: Core.Pool, puzzlePieceType: pieceBox, frames: Frames, diagnostics: diagnostics);
            var terrainAux = new RawGameAuxState();
            terrainAux.TerrainEnabled = 0u;
            Binding.BindGenerationInputs(farm, new TerrainGenerationInputs
            {
                Enabled = new RawCheckingFlag(terrainAux.TerrainEnabled),
                Battle = new RawCheckingFlag(Core.OwnerMode.Battle),
                Mission = new RawCheckingFlag(Core.BridgeState.Mission),
                Span = new IntBox(),
                Minimum = new IntBox(),
                Maximum = new IntBox(),
                Percent = new UIntBox(),
                Archipelago = new UIntBox(),
            });
            FortRandomPlacementHooks farmHooks = RawFortRandomPlacement.MakeFortRandomPlacementHooks(Core.Pool, Pipeline, Core.Factory, Hp, new FortRandomPlacementHooks());
            farmHooks.ChunkRegion = (x, y) => unchecked((sbyte)Chunks.At(x, y).Island);
            farmHooks.RegionOwner = region => Islands.Owner(region);
            farmHooks.SetOwner = (sid, owner) => Core.Owner.Set(sid, owner);
            farmHooks.Unpop = (sid, flags) => World.Unpop(sid, flags);
            farmHooks.Repop = (sid, flags) => World.Repop(sid, flags);
            farmHooks.Pop = (sid, x, y, flags) => World.Pop(sid, x, y, flags);
            Single = new RawFortRandomPlacement(Core.Pool, Core.Hash, Core.Spots, Types, Frames, Random, placement, farmHooks);
            var clusterHooks = new FarmClusterHooks { Assertion = _ => Unrestored("cluster") };
            RawFarmCluster.MakeFarmClusterPlacementHook(Core.Pool, Single, clusterHooks);
            Cluster = new RawFarmCluster(Core.Pool, Chunks, Islands, placement, farm, clusterHooks);
            StampHooks = RawTerrainWorldBinding.MakeTerrainStampWorldHooks(World, Stamp, _ => Unrestored("stamp-region"), new TerrainStampHooks
            {
                ChunkRegion = (x, y) => unchecked((sbyte)Chunks.At(x, y).Island),
                Assertion = _ => Unrestored("stamp"),
            });
            Fort.Territory = new byte[120];
            byte[] territoryRecord = [3, 1, 0x11, 7, 0xaa, 0xbb];
            territoryRecord.CopyTo(Fort.Territory, 0);
            byte[] badges = new byte[6160];
            Array.Fill(badges, (byte)0xa5);
            BadgeStream = new FortMetaStream { Bytes = new byte[1], Position = 0, End = 1, Present = true };
            var finishHooks = new FortFinishOwnerHooks
            {
                RegionBounds = _ => { Unrestored("finish-region"); return new FortRandomRectangle(0f, 0f, 0f, 0f); },
                WallSeconds = () => { Unrestored("finish-time"); return 0; },
                Place = (_, _) => { Unrestored("finish-place"); return 0; },
                InvalidType = () => Unrestored("finish-type"),
            };
            var finish = new RawFortFinishOwner(Core.Pool, Types, Residences, Modes, finishHooks);
            Finish = finish;
            var playerRecords = new byte[9 * (edition == OriginalEdition.Patch1078 ? 0xb4 : 0xac)];
            Money = new RawFortMoneyLoad(edition, HpState, Core.Bookkeeping, playerRecords, () => Depth, value => Depth = value,
                MoneyState, edition == OriginalEdition.Patch1078 ? Sp : null,
                new FortMoneyHooks { Assertion = _ => Unrestored("money"), LoadingDepthChanged = Sync, RefreshSp = () => Unrestored("money-sp") });
            Meta = new RawFortMetaLoad(Core.Pool, badges, () => Depth, value => Depth = value, MetaState,
                new FortMetaLoadHooks
                {
                    Assertion = _ => Unrestored("meta"),
                    LoadingDepthChanged = Sync,
                    TechnologyParent = () => { Effects.Add("parent-boundary"); return new Sid(0); },
                    PrepareTypeNames = () => Unrestored("meta-names"),
                    Contents = (_, _, _, _) => Unrestored("meta-contents"),
                    ReleaseTypeConversion = () => Unrestored("meta-release"),
                    RefreshTechnologyUi = () => Effects.Add("ui-boundary"),
                });
        }

        /// <summary>현재 깊이를 같은 child·표면·저장에 전달한다.</summary>
        /// <param name="value">새 깊이다.</param>
        private void Sync(uint value)
        {
            Depth = value;
        }

        /// <summary>빈 저장 목록에서도 모든 객체 필수 경계를 공급한다.</summary>
        /// <returns>선택되지 않은 가상 요청을 명시 거부하는 효과 묶음이다.</returns>
        private FortObjectLoadHooks EmptyFortObjects() => new()
        {
            Create = (type, flags) => World.Create(type, flags),
            NotifySurface = _ => Unrestored("objects-surface"),
            SetState = (_, _) => Unrestored("objects-state"),
            SetOwner = (_, _) => Unrestored("objects-owner"),
            HasHitPoints = sid => Hp.HasHitPoints(sid),
            MaxHitPoints = sid => Hp.MaxHitPoints(sid),
            VirtualCount = _ => { Unrestored("objects-virtual"); return 0; },
            Pop = (sid, x, y, flags) => World.Pop(sid, x, y, flags),
            Destroy = (sid, flags) => { if (!World.Destroy(sid, flags)) Unrestored("objects-destroy"); },
            CreateContent = (_, _, _, _, _, _) => { Unrestored("objects-content"); return 0; },
            ChunkRegion = (_, _) => { Unrestored("objects-region"); return 0; },
            TerritoryAllowed = (_, _, _) => { Unrestored("objects-allowed"); return false; },
            LoadingDepthChanged = _ => Unrestored("objects-depth"),
            FinishOwner = _ => Unrestored("objects-finish"),
        };

        /// <summary>깊이 동기화를 묶은 객체 적재 몸체를 만든다.</summary>
        /// <returns>같은 풀·깊이의 객체 적재 몸체다.</returns>
        private RawFortObjectLoad NewObjects()
        {
            FortObjectLoadHooks objectHooks = RawFortFinishOwner.MakeFortFinishOwnerHooks(Core.Pool, Finish, EmptyFortObjects());
            objectHooks.LoadingDepthChanged = Sync;
            return new RawFortObjectLoad(Core.Pool, Core.Hash, Types, Frames, Random, new FortObjectLoadState { LoadingDepth = Depth }, objectHooks);
        }

        /// <summary>같은 그래프 프레임 표를 원본 코드 순서대로 변환한다.</summary>
        /// <param name="frames">변환할 타입별 프레임 코드 표다.</param>
        /// <returns>그래프용 프레임 코드 표다.</returns>
        private static List<IReadOnlyList<FrameCode>> GraphFrames(IReadOnlyList<RiftTypeFrames> frames)
        {
            var result = new List<IReadOnlyList<FrameCode>>(frames.Count);
            // 타입마다 코드 4바이트를 그래프 코드로 옮기는 반복문이다.
            foreach (RiftTypeFrames table in frames)
            {
                var codes = new List<FrameCode>(table.Codes.Count);
                // 코드를 순서대로 변환하는 반복문이다.
                foreach (RiftFrameCode code in table.Codes)
                {
                    codes.Add(new FrameCode((char)code.Side, (char)code.Variant, code.Number, (FrameCodeFlags)code.Flags));
                }

                result.Add(codes);
            }

            return result;
        }

        /// <summary>같은 부모·자식·생성·Graph를 한 실제 월드에 조립하고 골든 11열과 대조한다.</summary>
        /// <param name="golden">같은 판본의 골든 11열이다.</param>
        public void Run(string[] golden)
        {
            bool patch = Edition == OriginalEdition.Patch1078;
            Assert.Equal(Hex(Fort.Territory), golden[1]);
            Assert.Equal(PuzzleHex(Frames[107].Codes), golden[2]);
            Assert.Equal(ChunkHex(Chunks), golden[3]);
            Assert.Equal(Hex(IslandBytes), golden[4]);
            World.BeginLoad();
            Sync(1);
            var graph = new RawGraph(Core.Pool, Core.Hash, Core.Spots, Types, null, null, GraphFrames(Frames));
            FortWorldLoadHooks hooks = new()
            {
                TemplatePresent = _ => true,
                Territory = owner => owner == 1u ? new FortTerritoryStream { Bytes = Fort.Territory, Position = 0, End = 120 } : throw new InvalidOperationException("선택 Fort 월드 장면의 지역 owner 불일치"),
                CurrentTemplate = owner => owner == 1u ? Fort : throw new InvalidOperationException("선택 Fort 월드 장면의 현재 Template 불일치"),
                Assertion = _ => Unrestored("world"),
                LoadingDepthChanged = value => { Sync(value); },
                BuilderAssertion = _ => Unrestored("world-builder"),
                PopulateIsland = (_, _) => Unrestored("world-populate"),
            };
            hooks = RawFortWorldLoadBinding.MakeFortWorldLoadBinding(
                Core.Pool, Core.Pool, graph, NewObjects(),
                Money, Meta,
                (fort, owner) =>
                {
                    Assert.Same(Fort, fort);
                    Assert.Equal(1u, owner);
                    Assert.Equal(2u, Depth);
                    Effects.Add("money");
                    MoneyState.TemplatePresent = true;
                    MoneyCursor.Position = 0;
                    return MoneyCursor;
                },
                (fort, owner, technology) =>
                {
                    Assert.Same(Fort, fort);
                    MetaState.TemplateSlot = unchecked((int)owner);
                    MetaState.LocalOwner = 1;
                    MetaState.TemplatePresent = true;
                    Effects.Add(technology ? "technology" : "badges");
                },
                (fort, technology) =>
                {
                    Assert.Same(Fort, fort);
                    Assert.Equal(Depth, technology ? 2u : 1u);
                    if (technology)
                    {
                        Assert.Equal("parent-boundary", Effects[^1]);
                    }

                    Effects.Add(technology ? "technology-select" : "badges-select");
                    FortMetaStream stream = technology ? TechStream : BadgeStream;
                    stream.Position = 0;
                    return stream;
                },
                (state, region) =>
                {
                    Assert.Equal(ChunkHex(Chunks), golden[5]);
                    Assert.Equal(Hex(IslandBytes), golden[6]);
                    Assert.Equal(1u, Islands.Count());
                    Assert.Equal("1", golden[7]);
                    Assert.Equal("1", golden[8]);
                    Assert.Equal("1", golden[9]);
                    Assert.Equal("0", golden[10]);
                    Effects.Add("generate");
                    Binding.Generate(new NormalTerrainRequest(1u, state.Bounds, region, NormalTerrainBranch.Normal), Random, Crt, StampHooks, (area, farmRegion, quantity) => Cluster.Place(area, farmRegion, quantity));
                },
                hooks);
            var generate = hooks.Generate;
            hooks.Generate = (state, region) =>
            {
                Assert.Equal(ChunkHex(Chunks), golden[5]);
                Assert.Equal(Hex(IslandBytes), golden[6]);
                generate!(state, region);
            };
            var loadObjects = hooks.Objects;
            hooks.Objects = (fort, owner) => { Effects.Add("objects"); loadObjects!(fort, owner); };
            var rebuild = hooks.RebuildGraph;
            hooks.RebuildGraph = reset => { Effects.Add("graph0"); Assert.False(reset); rebuild!(reset); };
            var parent = new RawFortWorldLoad(Core.Pool, Chunks, Islands, Frames, () => 107u, () => 1u, () => 127, () => Depth, Sync, hooks);
            parent.Load(0);
            Assert.Equal(ChunkHex(Chunks), golden[5]);
            Assert.Equal(Hex(IslandBytes), golden[6]);
            Assert.Equal(1u, Islands.Count());
            Assert.Equal(1u, Islands.Exists(0));
            Assert.Equal(1u, Islands.Owner(0));
            Assert.True(Chunks.At(2, 2).Island == 0);
            Assert.Equal(new string[] { "generate", "objects", "money", "badges", "badges-select", "technology", "parent-boundary", "technology-select", "ui-boundary", "graph0" }, Effects);
            Assert.Equal(1u, Depth);
            Assert.Equal(4u, MoneyCursor.Position);
            Assert.Equal(1u, BadgeStream.Position);
            Assert.Equal(0u, TechStream.Position);
            Assert.NotEqual(0x12345678u, Random.State());
            Assert.NotEqual(7u, Crt.Value);
            Assert.NotNull(Stamp.Variants);
            Assert.False(Sp.Initialized);
            List<SurfaceSnapshot> objectsNow = World.Objects();
            Assert.NotEmpty(objectsNow);
            int farms = 0;
            // 모든 생성 SID가 같은 Factory·풀의 현재 TYPE·지역인지 확인하는 반복문이다.
            foreach (SurfaceSnapshot snapshot in objectsNow)
            {
                ReadOnlySpan<byte> raw = Core.Pool.Slot(snapshot.Sid);
                Assert.Equal(0u, ReadSlot(raw, 8, 2));
                if (raw[10] == 121)
                {
                    farms++;
                    Assert.Equal(patch ? 0x50a8e8u : 0x500b10u, ReadSlot(raw, 0, 4));
                    Assert.Equal(73u, ReadSlot(raw, 26, patch ? 4 : 2));
                }
                else
                {
                    Assert.True(raw[10] == 97);
                    Assert.Equal(patch ? 0x501dd0u : 0x5058d8u, ReadSlot(raw, 0, 4));
                }

                Assert.Equal(snapshot.Sid, World.Find(snapshot.Object.X, snapshot.Object.Y, snapshot.Object.Type));
            }

            Assert.True(farms > 0 && farms <= 60);
            Assert.Equal(0, Core.Kernel.Size());
            Assert.Same(Core.Pool, parent.Pool());
        }
    }

    /// <summary>원본 Fort 적재 바인딩 전체 월드 장면을 패치판에서 같은 순서로 실행한다.</summary>
    /// <remarks>CD판은 표면 월드의 CD 조립이 후속 단위에 묶여 있어 같은 장면을 실행할 수 없다. CD 조립이 연결되면 CD 행을 더한다.</remarks>
    [Fact]
    public void ActualSelectedWorldAndTwoRandomStores()
    {
        string[][] rows = X86Fixture.Read("fortworldload-builder-consumer");
        string[] row = Assert.Single(rows, candidate => candidate.Length == 11 && candidate[0] == "originals");
        var scene = new Scene(OriginalEdition.Patch1078);
        scene.Run(row);
    }
}
