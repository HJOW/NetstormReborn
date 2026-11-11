// 독립 원본 118회의 현재 지도/난수/가상 순서/직접 frame·HP 쓰기를 같은 기존 배치 몸체로 비교한다.
using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using Netstorm.Core.Original;
using Xunit;

// 원본 섬 인구 및 편집기 섬 추가 x86 관찰 대조 테스트를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Tests;

// RawIslandPopulation 및 RawIslandAdd를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class IslandPopulationTests
{
    // 독립 원본 118회의 지도/난수/외부 효과/직접 frame·HP 쓰기를 전수 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void IslandPopulation_ReplaysNative118ActualMapRandomAndEffects()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "island-population-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int rows = 0;
        // 모든 네이티브 행을 처음부터 실제 복원 몸체에 재생하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int lineIndex = 1; lineIndex < lines.Length; ++lineIndex)
        {
            if (string.IsNullOrWhiteSpace(lines[lineIndex]))
            {
                continue;
            }

            string[] row = lines[lineIndex].Split('\t');
            Assert.Equal(18, row.Length);
            PopulationScene scene = new(row);
            if (row[0] == "Residence")
            {
                scene.Population.Residence(3);
            }
            else if (row[0] == "Populate")
            {
                scene.Population.Populate(3);
            }
            else
            {
                FortRandomRectangle rect = PopulationRectangle(uint.Parse(row[1], CultureInfo.InvariantCulture));
                scene.Population.ReplaceInitial(rect, 3);
            }

            string trace = scene.Events.Count == 0 ? "-" : string.Join(';', scene.Events);
            Assert.Equal(row[12], trace);
            Assert.Equal(uint.Parse(row[16], CultureInfo.InvariantCulture), scene.Calls);
            Assert.Equal(row[15], PopulationHex(PopulationMap(scene.Map)));
            if (row[12].Contains("create:", StringComparison.Ordinal))
            {
                ReadOnlySpan<byte> raw = scene.Pool.Slot(15002);
                Assert.Equal(uint.Parse(row[13], CultureInfo.InvariantCulture), BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(36, 4)));
                Assert.Equal(uint.Parse(row[14], CultureInfo.InvariantCulture), BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(26, 4)));
            }

            ++rows;
        }

        Assert.Equal(118, rows);
    }

    // 실제 임시 지도 충돌 거부와 Place 직후 하위 미연결 실패 시 앞선 부분 효과 보존을 검증하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void IslandAdd_ActualPreviewCollisionAndPlaceBeforeMissingGeneration()
    {
        string[] row = { "Populate", "0", "7", "1", "1", "0", "71", "133", "1", "1", "0", "639" };
        PopulationScene scene = new(row);
        ChunkMap preview = new(14, 14);
        RawIslandBuilderState state = new(new ChunkRect(0, 0, 14, 14), 8u);
        uint piece = 107u;
        bool fort = true;
        RiftTypeFrames[] frames = new RiftTypeFrames[188];
        // 프레임 표 전체를 빈 목록으로 초기화하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int i = 0; i < frames.Length; ++i)
        {
            frames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }

        List<RiftFrameCode> codes = new();
        // 독립 기존 Builder 관측과 같은 완전 프레임 입력의 side를 순회하는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (byte side = (byte)'A'; side <= (byte)'P'; ++side)
        {
            // 각 side별 11개 번호를 추가하는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (byte number = 0; number < 11; ++number)
            {
                codes.Add(new RiftFrameCode(side, (byte)'P', number, 0));
            }
        }

        frames[107] = new RiftTypeFrames(codes);
        uint graph = 0u;
        uint saves = 0u;
        uint generation = 0u;
        IslandAddHooks effects = new()
        {
            Generate = (current, region) =>
            {
                ++generation;
                Assert.Equal(1u, current.Owner);
                Assert.Equal(new ChunkRect(1, 1, 15, 15), current.Bounds);
                Assert.Equal(4, region);
                throw new InvalidOperationException("actual-generation-stop");
            },
            Graph = _ =>
            {
                ++graph;
                throw new InvalidOperationException("미선택실제Graph");
            },
            Territory = _ =>
            {
                ++saves;
                throw new InvalidOperationException("미선택실제Territory저장");
            },
        };
        RawIslandAdd add = new(
            scene.Map,
            scene.Islands,
            frames,
            () => piece,
            state,
            () => fort,
            scene.Population,
            _ => throw new InvalidOperationException("미선택Builder진단"),
            effects);
        byte[] record = { 0, 3, 0, 7, 0xab, 0xcd };
        byte[] before = PopulationMap(scene.Map);
        Span<ChunkEntry> previewEntries = preview.Entries();
        // 명시 임시 지도 전체 충돌을 설정하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int i = 0; i < previewEntries.Length; ++i)
        {
            previewEntries[i].Island = 0;
        }

        Assert.False(add.TryDrop(preview, record));
        Assert.Equal((byte)3, record[1]);
        Assert.Equal(8u, state.Owner);
        Assert.Equal(4u, scene.Islands.Count());
        Assert.Equal(0u, generation);
        Assert.Equal(before, PopulationMap(scene.Map));
        preview.Reset();
        InvalidOperationException ex = Assert.Throws<InvalidOperationException>(() => add.TryDrop(preview, record));
        Assert.Equal("actual-generation-stop", ex.Message);
        Assert.Equal(1u, generation);
        Assert.Equal(0u, graph);
        Assert.Equal(0u, saves);
        Assert.Empty(scene.Events);
        Assert.Equal((byte)1, record[1]);
        Assert.Equal(1u, state.Owner);
        Assert.Equal(5u, scene.Islands.Count());
        Assert.Equal(1u, scene.Islands.Owner(4));
        bool anyIsland4 = false;
        Span<ChunkEntry> mapEntries = scene.Map.Entries();
        // 월드 지도에 실제로 섬 4 청크가 배치됐는지 확인하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int i = 0; i < mapEntries.Length; ++i)
        {
            if (mapEntries[i].Island == 4)
            {
                anyIsland4 = true;
                break;
            }
        }

        Assert.True(anyIsland4);
    }

    // 명시 입력 사각형을 반환하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static FortRandomRectangle PopulationRectangle(uint profile)
    {
        FortRandomRectangle[] rects =
        {
            new(16f, 32f, 64f, 96f),
            new(15.25f, 21.5f, 16.1f, 23.9f),
            new(-15.25f, -3.5f, 4.5f, 9.5f),
            new(20f, 21f, 20f, 21f),
            new(20f, 21f, 19f, 19f),
        };
        return rects[profile];
    }

    // 실제 256행/헤더를 원문 배치로 직렬화하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static byte[] PopulationMap(ChunkMap map)
    {
        byte[] bytes = new byte[3084];
        BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(0, 4), map.Mode);
        BinaryPrimitives.WriteInt32LittleEndian(bytes.AsSpan(4, 4), map.XLength());
        BinaryPrimitives.WriteInt32LittleEndian(bytes.AsSpan(8, 4), map.YLength());
        Span<ChunkEntry> entries = map.Entries();
        // 256개 청크 항목을 12바이트씩 직렬화하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int index = 0; index < 256; ++index)
        {
            ref ChunkEntry entry = ref entries[index];
            int at = 12 + (index * 12);
            bytes[at] = entry.Island;
            bytes[at + 1] = unchecked((byte)entry.Side);
            entry.Record.AsSpan(0, 6).CopyTo(bytes.AsSpan(at + 2, 6));
            BinaryPrimitives.WriteInt32LittleEndian(bytes.AsSpan(at + 8, 4), entry.Ordinal);
        }

        return bytes;
    }

    // 바이트 배열을 소문자 16진수 문자열로 변환하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static string PopulationHex(ReadOnlySpan<byte> bytes)
    {
        StringBuilder builder = new(bytes.Length * 2);
        // 각 바이트를 2자리 16진수로 추가하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int i = 0; i < bytes.Length; ++i)
        {
            builder.Append(bytes[i].ToString("x2", CultureInfo.InvariantCulture));
        }

        return builder.ToString();
    }

    // 섬 인구 테스트를 위한 단일 장면 조립 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private sealed class PopulationScene
    {
        // 실제 SID 풀 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawSidPool Pool { get; } = new(24000, true, null, OriginalEdition.Patch1078);

        // 공간 해시 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawSquidHash Hash { get; } = new();

        // 현재 청크 지도 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public ChunkMap Map { get; } = new();

        // 타입 레코드 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public OriginalTypeRecord[] Types { get; } = new OriginalTypeRecord[188];

        // 거주 구역 목록 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawPostPopList Residences { get; } = new();

        // 모드 상태 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public FortFinishOwnerState Modes { get; } = new();

        // 섬 런타임 바이트 배열 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public byte[] IslandBytes { get; } = new byte[RawIslandStorage.Bytes];

        // 무작위 배치 호출 횟수 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public uint Calls { get; private set; }

        // 이벤트 추적 리스트 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public List<string> Events { get; } = new();

        // 섬 런타임 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawIslandRuntime Islands { get; }

        // 무작위 배치 몸체 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawFortFinishOwner Placement { get; }

        // 섬 인구 생성기 프로퍼티다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public RawIslandPopulation Population { get; }

        // 행 입력으로부터 테스트 장면을 초기화하는 생성자다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        public PopulationScene(string[] row)
        {
            // 타입 레코드 배열을 초기화하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < Types.Length; ++i)
            {
                Types[i] = new OriginalTypeRecord();
            }

            uint at = uint.Parse(row[9], CultureInfo.InvariantCulture);
            uint flip = uint.Parse(row[5], CultureInfo.InvariantCulture);
            uint hp = uint.Parse(row[8], CultureInfo.InvariantCulture);
            uint mutate = uint.Parse(row[10], CultureInfo.InvariantCulture);
            uint seed = uint.Parse(row[2], CultureInfo.InvariantCulture);
            uint fort = uint.Parse(row[3], CultureInfo.InvariantCulture);
            IslandPopulationGlobals globals = new()
            {
                Boss = uint.Parse(row[4], CultureInfo.InvariantCulture) != 0u,
                Fort = fort != 0u,
                Residence = 89u,
                Primary = uint.Parse(row[6], CultureInfo.InvariantCulture),
                Fallback = uint.Parse(row[7], CultureInfo.InvariantCulture),
                Replacement = 158u,
            };
            Assert.Equal(15000, Pool.Allocate());
            Assert.Equal(15001, Pool.Allocate());
            Span<ChunkEntry> entries = Map.Entries();
            // 전체 실제 지도와 미사용 영역의 명시 입력을 설정하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < entries.Length; ++i)
            {
                entries[i].Island = 127;
                entries[i].Side = (sbyte)'.';
                entries[i].Record = new byte[] { 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5 };
                entries[i].Ordinal = 0x12345678;
            }

            (int X, int Y)[] points = { (1, 2), (2, 2), (1, 3), (2, 3) };
            // 네이티브 도구가 준비한 물리 오프셋에 섬 3 레코드를 기록하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            foreach ((int px, int py) in points)
            {
                ref ChunkEntry entry = ref entries[(py * 16) + px];
                entry.Island = 3;
                entry.Record = new byte[] { 3, 1, 0x21, unchecked((byte)seed), 0xab, 0xcd };
            }

            BinaryPrimitives.WriteUInt32LittleEndian(IslandBytes.AsSpan(0, 4), 4u);
            BinaryPrimitives.WriteUInt32LittleEndian(IslandBytes.AsSpan(4 + (3 * 32) + 4, 4), 1u);
            BinaryPrimitives.WriteUInt32LittleEndian(IslandBytes.AsSpan(4 + (3 * 32) + 8, 4), 3u);
            uint redraw = 0u;
            uint revision = 0u;
            Islands = new RawIslandRuntime(
                Pool,
                Hash,
                IslandBytes,
                Modes,
                Residences,
                () => redraw,
                v => redraw = v,
                () => revision,
                v => revision = v,
                new IslandRuntimeHooks
                {
                    SetResidenceOwner = (_, _) => throw new InvalidOperationException("미선택Runtime거주구역owner"),
                    Assertion = _ => throw new InvalidOperationException("미선택Runtime진단"),
                });
            FortFinishOwnerHooks randomHooks = new()
            {
                RegionBounds = _ => throw new InvalidOperationException("직접40ded0의미선택bounds"),
                WallSeconds = () => throw new InvalidOperationException("직접40ded0의미선택시각"),
                InvalidType = () => Events.Add("D:29"),
                Place = (local, request) =>
                {
                    ++Calls;
                    uint bx = BitConverter.SingleToUInt32Bits(request.X);
                    uint by = BitConverter.SingleToUInt32Bits(request.Y);
                    Events.Add($"P:{request.Type}:{bx}:{by}:{unchecked((uint)request.Region)}:{request.Mode}:{request.Force}:{local.State()}");
                    return Calls >= at ? 15001u : 0u;
                },
            };
            Placement = new RawFortFinishOwner(Pool, Types, Residences, Modes, randomHooks);
            IslandPopulationHooks hooks = new()
            {
                Assertion = assertion =>
                {
                    ReadOnlySpan<uint> assertionLines = stackalloc uint[] { 0xc2u, 0x139u, 0xc3u, 0xd1u, 0x118u };
                    uint line = assertionLines[(int)assertion];
                    Events.Add($"A:{line}");
                    if (flip != 0u && (line == 0xc3u || line == 0xd1u))
                    {
                        globals.Boss = true;
                    }
                },
                WallSeconds = () =>
                {
                    ulong bits = BitConverter.DoubleToUInt64Bits(2.5);
                    uint low = unchecked((uint)bits);
                    uint high = unchecked((uint)(bits >> 32));
                    Events.Add($"T:{low}:{high}");
                    return 2.5;
                },
                Position = sid =>
                {
                    Events.Add($"position:{sid}");
                    return (55.25f, 66.5f);
                },
                Destroy = (sid, flags) =>
                {
                    Events.Add($"destroy:{sid}:{flags}");
                    if (mutate != 0u)
                    {
                        Map.Entries()[(2 * 16) + 1].Record[0] = 0xff;
                    }
                },
                Create = (type, flags) =>
                {
                    Events.Add($"create:{type}:{flags}");
                    int sid = Pool.Allocate();
                    Assert.Equal(15002, sid);
                    Span<byte> raw = Pool.AllocatedBytes(sid);
                    raw.Fill(0xa5);
                    BinaryPrimitives.WriteUInt32LittleEndian(raw.Slice(0, 4), 0x10012000u);
                    raw[10] = unchecked((byte)type);
                    raw[11] = 4;
                    return sid;
                },
                DefaultFrame = type =>
                {
                    Events.Add($"type:{type}");
                    return -7;
                },
                HasHitPoints = _ =>
                {
                    Events.Add("hasHp");
                    return hp != 0u;
                },
                TypeHitPoints = type =>
                {
                    Events.Add($"type:{type}");
                    Events.Add("maxHp");
                    return 87;
                },
                SetOwner = (_, owner) =>
                {
                    Events.Add("owner:3");
                    Events.Add($"setOwner:{owner}");
                },
                Pop = (_, x, y, flags) =>
                {
                    uint bx = BitConverter.SingleToUInt32Bits(x);
                    uint by = BitConverter.SingleToUInt32Bits(y);
                    Events.Add($"pop:{bx}:{by}:{flags}");
                },
                RefreshProduction = () => Events.Add("refresh"),
            };
            Population = new RawIslandPopulation(Pool, Map, Map, Islands, Placement, globals, hooks);
        }
    }
}
