// 실제 두 PE의 초기 배열·문자 대응·청크/칸 bounds와 문자 변환 결과를 그대로 대조한다.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;
using Netstorm.Core.Original;
using Xunit;

// 원본 지형 컨텍스트 x86 관찰 대조 테스트를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Tests;

// RawTerrainContext 생성자 및 문자 변환을 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class TerrainContextTests
{
    // 실제 생성자 모든 고유 입력과 앞뒤 문자 변환·정상/assert 계속 반환을 양 판본에서 전수 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainContext_ReplaysNativeConstructorAndCharacters()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "terraincontext-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int checkedRows = 0;
        // 헤더 이후의 모든 고유 행을 순회하며 값형 입력만 공급하고 기대 문자열과 비교하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int lineIndex = 1; lineIndex < lines.Length; ++lineIndex)
        {
            if (string.IsNullOrWhiteSpace(lines[lineIndex]))
            {
                continue;
            }

            string[] row = lines[lineIndex].Split('\t');
            Assert.Equal(13, row.Length);
            string[] fields = row[2].Split(',');
            Assert.True(fields.Length is 10 or 11);
            List<uint> input = new(fields.Length);
            // 원본 입력의 실제 DWORD 비트를 순서대로 파싱하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            foreach (string field in fields)
            {
                input.Add(ParseDword(field));
            }

            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            TerrainContextGlobals globals = new()
            {
                Battle = input[1] != 0u,
                NoRegion = unchecked((int)input[2]),
                IslandType = input[8],
                EdgeType = input[9],
            };
            uint assertions = 0u;
            RawTerrainContext context = new(
                edition,
                input[0],
                unchecked((int)input[7]),
                new ChunkRect(unchecked((int)input[3]), unchecked((int)input[4]), unchecked((int)input[5]), unchecked((int)input[6])),
                globals,
                _ => ++assertions);
            if (row[1] != "Ctor")
            {
                uint profile = input[10];
                // 같은 5×5의 문자/지역 입력만 넣고 실제 모듈에 변환을 맡기는 외부 X 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int x = 49; x < 54; ++x)
                {
                    // 각 X열은 Y순서로 원본 입력과 같은 바이트를 쓰는 내부 Y 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                    for (int y = 51; y < 56; ++y)
                    {
                        byte value = (byte)'x';
                        if (profile == 1u && x == 49)
                        {
                            value = (byte)'X';
                        }

                        if (profile == 2u && y == 51)
                        {
                            value = (byte)'o';
                        }

                        if (profile == 3u && x == 52)
                        {
                            value = (byte)'=';
                        }

                        if (profile == 4u && x == 51 && y == 53)
                        {
                            value = (byte)'.';
                        }

                        context.Codes[(x * 256) + y] = value;
                        context.Regions[(x * 256) + y] = (byte)(profile == 3u && y == 54 ? 6 : 5);
                    }
                }

                if (row[1] == "Edge")
                {
                    context.MarkEdges((byte)'x', (byte)'=');
                }
                else
                {
                    context.NormalizeAfterStamp();
                }
            }

            TerrainStampMap map = context.Map;
            ChunkRect chunks = context.Chunks;
            byte[] codes = context.Codes;
            byte[] regions = context.Regions;
            uint[] uniform =
            {
                codes.All(v => v == codes[0]) ? 1u : 0u,
                codes[0],
                regions.All(v => v == regions[0]) ? 1u : 0u,
                regions[0],
            };
            Assert.Equal(ParseDword(row[3]), context.Owner);
            Assert.Equal(row[4], ContextValues(new uint[] { map.MakePredictable, unchecked((uint)map.LastSid), map.ExcludedFlags, unchecked((uint)context.Region) }));
            Assert.Equal(row[5], ContextValues(new int[] { chunks.Left, chunks.Top, chunks.Right, chunks.Bottom }));
            Assert.Equal(row[6], ContextValues(map.Bounds));
            Assert.Equal(row[7], ContextValues(map.TypeMapping));
            Assert.Equal(row[8], ContextValues(map.Aliases));
            Assert.Equal(row[9], ContextValues(uniform));
            Assert.Equal(row[10], ContextWindow(codes));
            Assert.Equal(row[11], ContextWindow(regions));
            Assert.Equal(ParseDword(row[12]), assertions);
            ++checkedRows;
        }

        Assert.Equal(224, checkedRows);
    }

    // 초기 도장이 같은 소유 배열을 참조하며 잘못된 배열 밖 범위를 묵시 clamp하지 않는지 확인하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainContext_SharesArraysAndRejectsMissingDiagnostics()
    {
        TerrainContextGlobals globals = new();
        RawTerrainContext context = new(OriginalEdition.Patch1078, 1u, 17, new ChunkRect(1, 1, 15, 15), globals, _ => { });
        Assert.Same(context.Codes, context.Map.Codes);
        Assert.Same(context.Regions, context.Map.Regions);
        Assert.Same(context.Aliases, context.Map.Aliases);
        context.Aliases['h'] = 'x';
        Assert.Equal('x', context.Map.Aliases['h']);
        Assert.Throws<ArgumentException>(() => new RawTerrainContext(OriginalEdition.Patch1078, 1u, 17, new ChunkRect(1, 1, 15, 15), globals, null!));
        context.SetBounds(new ChunkRect(-1, 0, 1, 1));
        Assert.Equal(-16, context.Map.Bounds[0]);
        Assert.Throws<ArgumentOutOfRangeException>(() => context.MarkEdges((byte)'x', (byte)'='));
    }

    // 부호 있는/없는 10진수 문자열을 32비트 DWORD로 파싱하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static uint ParseDword(string value)
    {
        return value.StartsWith('-')
            ? unchecked((uint)int.Parse(value, CultureInfo.InvariantCulture))
            : uint.Parse(value, CultureInfo.InvariantCulture);
    }

    // 부호 없는 DWORD 배열을 쉼표 구분 문자열로 직렬화하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static string ContextValues(IReadOnlyList<uint> values)
    {
        StringBuilder builder = new();
        // 원본 순서를 유지하며 각 값을 쉼표로 이어 붙이는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int index = 0; index < values.Count; ++index)
        {
            if (index > 0)
            {
                builder.Append(',');
            }

            builder.Append(values[index].ToString(CultureInfo.InvariantCulture));
        }

        return builder.ToString();
    }

    // 부호 있는 정수 배열을 실제 DWORD 비트로 쉼표 구분 직렬화하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static string ContextValues(IReadOnlyList<int> values)
    {
        StringBuilder builder = new();
        // 원본 순서를 유지하며 부호 있는 값을 uint 비트로 변환해 이어 붙이는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int index = 0; index < values.Count; ++index)
        {
            if (index > 0)
            {
                builder.Append(',');
            }

            builder.Append(unchecked((uint)values[index]).ToString(CultureInfo.InvariantCulture));
        }

        return builder.ToString();
    }

    // 검증 입력의 X열 우선 16×16 창을 16진수 문자열로 읽는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static string ContextWindow(ReadOnlySpan<byte> bytes)
    {
        StringBuilder builder = new(512);
        // 네이티브가 관찰한 X열 순서로 같은 창을 이어 붙이는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int x = 48; x < 64; ++x)
        {
            ReadOnlySpan<byte> slice = bytes.Slice((x * 256) + 48, 16);
            // 슬라이스 내 각 바이트를 소문자 16진수로 변환하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < slice.Length; ++i)
            {
                builder.Append(slice[i].ToString("x2", CultureInfo.InvariantCulture));
            }
        }

        return builder.ToString();
    }
}
