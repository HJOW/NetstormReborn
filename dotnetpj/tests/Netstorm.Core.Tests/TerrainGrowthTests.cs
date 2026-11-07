// 양 판본 실제 일반 성장의 전체 code/region 배열을 고유 원본 입력 그대로 대조한다.
using System;
using System.Globalization;
using System.IO;
using Netstorm.Core.Original;
using Xunit;

// 원본 지형 성장 x86 관찰 대조 테스트를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Tests;

// RawTerrainGrowth 일반 성장을 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class TerrainGrowthTests
{
    // 정상 108개 원본 입력을 양 판본에서 재생하여 전체 배열과 비교하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainGrowth_ReplaysBothNativeFullArrays()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "terraingrowth-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int checkedRows = 0;
        // 모든 기대 배열을 픽스처에서 읽고 C# 성장 모듈에 같은 입력만 공급하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int lineIndex = 1; lineIndex < lines.Length; ++lineIndex)
        {
            if (string.IsNullOrWhiteSpace(lines[lineIndex]))
            {
                continue;
            }

            string[] row = lines[lineIndex].Split('\t');
            Assert.Equal(6, row.Length);
            string[] input = row[1].Split(',');
            Assert.Equal(3, input.Length);
            uint profile = uint.Parse(input[0], CultureInfo.InvariantCulture);
            uint seed = uint.Parse(input[1], CultureInfo.InvariantCulture);
            uint selected = uint.Parse(input[2], CultureInfo.InvariantCulture);
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            TerrainContextGlobals globals = new();
            uint assertions = 0u;
            RawTerrainContext context = new(
                edition,
                1u,
                unchecked((int)selected),
                new ChunkRect(3, 3, profile == 4u ? 6 : 5, 5),
                globals,
                _ => ++assertions);
            ChunkMap chunks = new();
            GrowthChunks(chunks, profile, seed);
            if (profile == 5u)
            {
                context.Codes[(56 * 256) + 56] = (byte)'x';
                context.Regions[(56 * 256) + 56] = 3;
                context.Codes[(57 * 256) + 56] = (byte)'=';
                context.Regions[(57 * 256) + 56] = 3;
            }

            RawTerrainGrowth growth = new(context, chunks, globals);
            growth.RunNormal();
            byte[] codes = GrowthBytes(row[3]);
            byte[] regions = GrowthBytes(row[4]);
            Assert.Equal(codes, context.Codes);
            Assert.Equal(regions, context.Regions);
            Assert.Equal(seed, uint.Parse(row[2], CultureInfo.InvariantCulture));
            Assert.Equal(uint.Parse(row[5], CultureInfo.InvariantCulture), assertions);
            ++checkedRows;
        }

        Assert.Equal(108, checkedRows);
    }

    // 성장이 같은 context를 갱신하고 mapping/alias/도장 마지막 SID는 유지하는지 검증하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainGrowth_UsesSameContextAndPreservesStampFields()
    {
        TerrainContextGlobals globals = new();
        RawTerrainContext context = new(OriginalEdition.Patch1078, 1u, 127, new ChunkRect(3, 3, 5, 5), globals, _ => { });
        ChunkMap chunks = new();
        GrowthChunks(chunks, 2u, 7u);
        byte[] codes = context.Codes;
        byte[] regions = context.Regions;
        context.Map.LastSid = 123;
        int[] mapping = (int[])context.Map.TypeMapping.Clone();
        RawTerrainGrowth growth = new(context, chunks, globals);
        growth.RunNormal();
        Assert.Same(codes, context.Codes);
        Assert.Same(codes, context.Map.Codes);
        Assert.Same(regions, context.Regions);
        Assert.Same(regions, context.Map.Regions);
        Assert.Equal(123, context.Map.LastSid);
        Assert.Equal(1u, context.Map.MakePredictable);
        Assert.Equal(mapping, context.Map.TypeMapping);
    }

    // 전체 배열의 원본 RLE를 풀어 65536바이트 배열로 복원하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    internal static byte[] GrowthBytes(string value)
    {
        byte[] result = new byte[65536];
        int offset = 0;
        // 네이티브의 연속값/길이를 원본 저장 순서로 그대로 복원하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        foreach (string run in value.Split(','))
        {
            int separator = run.IndexOf(':');
            if (separator < 0)
            {
                throw new InvalidOperationException("성장 RLE 필드 누락");
            }

            byte b = byte.Parse(run[..separator], NumberStyles.HexNumber, CultureInfo.InvariantCulture);
            int count = int.Parse(run[(separator + 1)..], CultureInfo.InvariantCulture);
            if (count > 65536 - offset)
            {
                throw new InvalidOperationException("성장 RLE 배열 범위 초과");
            }

            result.AsSpan(offset, count).Fill(b);
            offset += count;
        }

        if (offset != 65536)
        {
            throw new InvalidOperationException("성장 RLE 전체 배열 길이 오류");
        }

        return result;
    }

    // 기대 지형 없이 입력 청크의 원본 섬 BYTE/방향/seed만 만드는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static void GrowthChunks(ChunkMap chunks, uint profile, uint seed)
    {
        (int X, int Y)[][] profiles =
        {
            Array.Empty<(int X, int Y)>(),
            new[] { (3, 3) },
            new[] { (3, 3), (4, 3), (3, 4), (4, 4) },
            new[] { (3, 3), (4, 3), (3, 4), (4, 4) },
            new[] { (3, 3), (4, 3), (5, 3), (3, 4), (4, 4), (5, 4) },
            new[] { (3, 3), (4, 4) },
        };
        int index = 0;
        // 원본 입력의 현재 청크 필드만 채우는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        foreach ((int x, int y) in profiles[profile])
        {
            ref ChunkEntry chunk = ref chunks.At(x, y);
            chunk.Island = (byte)((profile == 3u || profile == 5u) ? (x - 3) & 1 : 0);
            chunk.Side = (sbyte)('A' + (((profile * 3u) + (uint)index) % 16u));
            chunk.Record ??= new byte[6];
            chunk.Record[3] = unchecked((byte)(seed + (uint)((x - 3) * 13) + (uint)((y - 3) * 19)));
            ++index;
        }
    }
}
