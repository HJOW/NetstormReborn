// 실제 양 판본 raw+5 시드와 공유 지형 흩뿌리기의 전체 배열/진단만 대조한다.
using System;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using Netstorm.Core.Original;
using Xunit;

// 원본 지형 시드 및 흩뿌리기 x86 관찰 대조 테스트를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Tests;

// RawTerrainGrowth.FirstChunkSeed 및 Scatter를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class TerrainSeedTests
{
    // 두 판본 111개씩의 signed BYTE/전체 noRegion·순회·진단 복귀/고정 폴백을 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainSeed_ReplaysNativeFirstChunkAndDiagnostics()
    {
        ReplaySeed("First", 222);
    }

    // 중복/기존 x의 지역 덮어쓰기·다른 섬 거부·빈 지도 폴백·0회 호출을 원문 배열 전체로 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainSeed_ReplaysNativeScatterFullArrays()
    {
        ReplaySeed("Scatter", 156);
    }

    // 같은 장기 checking은 호출 때 다시 읽으며 진단 없는 정상 호출에 장치를 미리 강제하지 않는지 검증하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainSeed_LiveCheckingAndSelectedMissingDevice()
    {
        TerrainContextGlobals globals = new();
        RawTerrainContext context = new(OriginalEdition.Patch1078, 1u, 127, new ChunkRect(3, 3, 5, 5), globals, _ => { });
        ChunkMap chunks = new();
        int checking = 0;
        TerrainGrowthDiagnostics diagnostics = new();
        diagnostics.Checking.Bind(() => unchecked((uint)checking));
        RawTerrainGrowth growth = new(context, chunks, globals, diagnostics);
        Assert.Equal(0x2333u, growth.FirstChunkSeed());
        checking = unchecked((int)0x80000000u);
        Assert.Throws<InvalidOperationException>(() => growth.FirstChunkSeed());
        chunks.At(4, 3).Island = 0;
        chunks.At(4, 3).Record[3] = 255;
        Assert.Equal(255u, growth.FirstChunkSeed());
        chunks.Mode = 0u;
        checking = 0;
        Assert.Throws<InvalidOperationException>(() => growth.FirstChunkSeed());
    }

    // 일반 분기는 첫 시드 실패 진단을 만들지 않는지 검증하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainSeed_NormalBranchDoesNotQueryFirstSeed()
    {
        TerrainContextGlobals globals = new();
        RawTerrainContext context = new(OriginalEdition.Patch1078, 1u, 127, new ChunkRect(3, 3, 5, 5), globals, _ => { });
        ChunkMap chunks = new();
        uint checking = 0xffffffffu;
        RawTerrainGrowth growth = new(
            context,
            chunks,
            globals,
            new TerrainGrowthDiagnostics
            {
                Checking = new RawCheckingFlag(() => checking),
            });
        growth.RunNormal();
        Assert.True(context.Codes.All(v => v == (byte)'.'));
    }

    // 입력 프로파일의 청크 경계를 반환하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static ChunkRect SeedBounds(int profile)
    {
        if (profile == 4)
        {
            return new ChunkRect(-1, -1, 1, 1);
        }

        return new ChunkRect(3, 3, profile == 0 ? 3 : 5, 5);
    }

    // 네이티브 입력과 같은 청크 원시 바이트만 채우고 출력 지형은 생성하지 않는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static void SeedChunks(ChunkMap chunks, int profile, uint seed)
    {
        static void Add(ChunkMap map, int x, int y, byte island, uint value)
        {
            ref ChunkEntry chunk = ref map.At(x, y);
            chunk.Island = island;
            chunk.Record ??= new byte[6];
            chunk.Record[3] = unchecked((byte)value);
        }

        if (profile == 2)
        {
            Add(chunks, 4, 3, 0, seed);
            Add(chunks, 3, 4, 1, seed + 19u);
        }

        if (profile == 3)
        {
            Add(chunks, 3, 3, 255, seed);
            Add(chunks, 4, 3, 128, seed + 19u);
            Add(chunks, 3, 4, 0, seed + 37u);
        }

        if (profile == 4)
        {
            Add(chunks, 0, 0, 0, seed);
        }

        if (profile is >= 5 and <= 9)
        {
            Add(chunks, 3, 3, 0, seed);
            Add(chunks, 4, 3, 1, seed + 13u);
            Add(chunks, 3, 4, 127, seed + 19u);
            Add(chunks, 4, 4, 255, seed + 37u);
        }
    }

    // 선택한 원본 몸체만 실행하며 픽스처 기대값과 대조하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static void ReplaySeed(string kind, int expected)
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "terrain-seed-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int checkedRows = 0;
        // 첫 조회와 흩뿌리기 사례를 행별로 실행하고 결과를 대조하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int lineIndex = 1; lineIndex < lines.Length; ++lineIndex)
        {
            if (string.IsNullOrWhiteSpace(lines[lineIndex]))
            {
                continue;
            }

            string[] row = lines[lineIndex].Split('\t');
            Assert.Equal(10, row.Length);
            if (row[1] != kind)
            {
                continue;
            }

            string[] fields = row[2].Split(',');
            Assert.Equal(7, fields.Length);
            int[] input = new int[7];
            // 전체 signed DWORD 입력을 파싱하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < input.Length; ++i)
            {
                input[i] = unchecked((int)long.Parse(fields[i], CultureInfo.InvariantCulture));
            }

            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            TerrainContextGlobals globals = new() { NoRegion = input[2] };
            RawTerrainContext context = new(edition, 1u, 127, SeedBounds(input[0]), globals, _ => { });
            ChunkMap chunks = new();
            SeedChunks(chunks, input[0], unchecked((uint)input[1]));
            chunks.Mode = unchecked((uint)input[4]);
            if (input[0] == 8)
            {
                context.Map.Bounds[0] = 50;
                context.Map.Bounds[1] = 50;
                context.Map.Bounds[2] = 53;
                context.Map.Bounds[3] = 53;
            }

            if (input[0] == 9)
            {
                Array.Fill(context.Codes, (byte)'x');
                Array.Fill(context.Regions, (byte)9);
            }

            StringBuilder assertions = new();
            int checking = input[3];
            RawTerrainGrowth growth = new(
                context,
                chunks,
                globals,
                new TerrainGrowthDiagnostics
                {
                    Checking = new RawCheckingFlag(() => unchecked((uint)checking)),
                    Assertion = (expression, file, line) =>
                    {
                        if (assertions.Length > 0)
                        {
                            assertions.Append(';');
                        }

                        assertions.Append(expression).Append('|').Append(file).Append('|').Append(line.ToString(CultureInfo.InvariantCulture));
                        if (input[5] == 1 && line == 0xb7u)
                        {
                            chunks.Mode = 1u;
                            globals.NoRegion = 0;
                        }

                        if (input[5] == 2 && line == 0xb8u)
                        {
                            ChunkRect bounds = context.Chunks;
                            context.SetBounds(new ChunkRect(bounds.Left, bounds.Top, 0, bounds.Bottom));
                            chunks.At(0, 0).Island = 127;
                        }

                        if (input[5] == 3 && line == 0xb7u)
                        {
                            chunks.Mode = 1u;
                            checking = 0;
                            chunks.At(0, 0).Island = 127;
                        }
                    },
                });
            if (kind == "First")
            {
                Assert.Equal(uint.Parse(row[3], CultureInfo.InvariantCulture), growth.FirstChunkSeed());
            }
            else
            {
                growth.Scatter(input[6]);
            }

            Assert.Equal(row[4], assertions.Length == 0 ? "-" : assertions.ToString());
            byte[] codes = TerrainGrowthTests.GrowthBytes(row[8]);
            byte[] regions = TerrainGrowthTests.GrowthBytes(row[9]);
            Assert.Equal(codes, context.Codes);
            Assert.Equal(regions, context.Regions);
            Assert.Equal(
                kind == "First" ? 0u : unchecked((uint)input[6] * 2u),
                uint.Parse(row[7], CultureInfo.InvariantCulture));
            ++checkedRows;
        }

        Assert.Equal(expected, checkedRows);
    }
}
