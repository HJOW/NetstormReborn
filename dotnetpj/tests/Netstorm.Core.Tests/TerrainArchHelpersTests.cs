// 군도 공유 성장/제거 도우미의 전체 지도와 호출자 난수를 양 판본 PE 기록에 대조한다.
using System;
using System.Globalization;
using System.IO;
using Netstorm.Core.Original;
using Xunit;

// 원본 지형 군도 도우미 x86 관찰 대조 테스트를 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
namespace Netstorm.Core.Tests;

// RawTerrainGrowth의 GrowRectangle, RemoveSparseCell, RemoveSparse, ClearPlacement를 검증하는 테스트 클래스다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
public sealed class TerrainArchHelpersTests
{
    // 지역 BYTE 감김/군도 비영 DWORD·목표 비양수와 비정사각형 X폭/최종 시드를 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainArchHelpers_GrowRectangleNative()
    {
        ReplayArch("Grow", 144);
    }

    // 전체 256 이웃 마스크와 -1/4/12 임계값·비x/현재 noRegion BYTE를 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainArchHelpers_SparseCellNative()
    {
        ReplayArch("Cell", 1542);
    }

    // 같은 패스의 즉시 제거와 재순회·내부 경계 제외를 전체 배열에서 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainArchHelpers_RemoveSparseNative()
    {
        ReplayArch("Sparse", 24);
    }

    // 컨텍스트 bounds 밖의 배치·역전/빈 영역·DWORD 시프트 감김을 실제 양 판본 출력과 대조하는 테스트 메서드다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    [Fact]
    public void TerrainArchHelpers_ClearPlacementNative()
    {
        ReplayArch("Clear", 24);
    }

    // 실제 도우미가 읽을 명시 문자/지역만 놓고 결과 성장/가중치/제거는 계산하지 않는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static void ArchInput(RawTerrainContext context, uint profile)
    {
        Array.Fill(context.Regions, (byte)9);
        // 네이티브와 같은 넓은 입력 칸 영역의 X열을 순회하는 외부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int x = 48; x < 112; ++x)
        {
            // 입력 공식에 따라 각 Y칸의 초기 문자와 지역을 채우는 내부 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int y = 48; y < 112; ++y)
            {
                byte code = (byte)'.';
                if (profile == 0u)
                {
                    code = (x == 70 && y == 70) ? (byte)'x' : (byte)'.';
                }
                else if (profile == 1u)
                {
                    code = (((x * 3) + y) % 7) == 0 ? (byte)'x' : ((x + y) % 3) == 0 ? (byte)'=' : (byte)'.';
                }
                else if (profile == 2u)
                {
                    code = (byte)'x';
                }
                else if (profile == 3u)
                {
                    code = (x == 70 || y == 70) ? (byte)'x' : (byte)'.';
                }
                else if (profile == 4u)
                {
                    code = (((x / 3) + (y / 3)) % 2) == 0 ? (byte)'x' : (byte)'.';
                }
                else
                {
                    code = ((x + y) % 2) == 0 ? (byte)'x' : (byte)'.';
                }

                int index = (x * 256) + y;
                context.Codes[index] = code;
                context.Regions[index] = (byte)((x + y) % 256);
            }
        }
    }

    // 입력의 signed DWORD를 같은 하위 비트로 읽는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static int ArchSigned(string value)
    {
        return value.StartsWith('-')
            ? int.Parse(value, CultureInfo.InvariantCulture)
            : unchecked((int)uint.Parse(value, CultureInfo.InvariantCulture));
    }

    // 선택 도우미의 양 판본 모든 고유 행을 재생하며 두 배열과 호출자 시드를 원문 기대값과 비교하는 정적 보조 함수다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
    private static void ReplayArch(string kind, int expected)
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "terrain-arch-helpers-x86.tsv");
        string[] lines = File.ReadAllLines(path);
        int checkedRows = 0;
        // 선택된 도우미의 입력만 실행하고 두 배열 및 시드를 대조하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
        for (int lineIndex = 1; lineIndex < lines.Length; ++lineIndex)
        {
            if (string.IsNullOrWhiteSpace(lines[lineIndex]))
            {
                continue;
            }

            string[] row = lines[lineIndex].Split('\t');
            Assert.Equal(7, row.Length);
            if (row[1] != kind)
            {
                continue;
            }

            string[] input = row[2].Split(',');
            OriginalEdition edition = row[0] == "originals" ? OriginalEdition.Patch1078 : OriginalEdition.Cd1072;
            TerrainContextGlobals globals = new();
            RawTerrainContext context = new(edition, 1u, 127, new ChunkRect(4, 4, 6, 5), globals, _ => { });
            ChunkMap chunks = new();
            RawTerrainGrowth growth = new(context, chunks, globals);
            uint seed = 0u;
            bool result = false;
            chunks.Mode = 1u;
            Span<ChunkEntry> entries = chunks.Entries();
            // 네이티브의 명시 청크 0 입력을 기본 noRegion 127과 구별하도록 설정하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
            for (int i = 0; i < entries.Length; ++i)
            {
                entries[i].Island = 0;
            }

            ArchInput(context, kind == "Cell" ? 0u : uint.Parse(input[0], CultureInfo.InvariantCulture));
            if (kind == "Grow")
            {
                Assert.Equal(5, input.Length);
                uint profile = uint.Parse(input[0], CultureInfo.InvariantCulture);
                context.ArchipelagoMode = uint.Parse(input[1], CultureInfo.InvariantCulture);
                int region = ArchSigned(input[2]);
                seed = uint.Parse(input[3], CultureInfo.InvariantCulture);
                Array.Fill(context.Regions, unchecked((byte)region));
                growth.GrowRectangle(
                    region,
                    profile == 1u ? new ChunkRect(4, 4, 5, 6) : new ChunkRect(4, 4, 6, 5),
                    ArchSigned(input[4]),
                    ref seed);
            }
            else if (kind == "Cell")
            {
                Assert.Equal(4, input.Length);
                uint mask = uint.Parse(input[0], CultureInfo.InvariantCulture);
                globals.NoRegion = ArchSigned(input[2]);
                context.Codes[(70 * 256) + 70] = (byte)uint.Parse(input[3], CultureInfo.InvariantCulture);
                (int Dx, int Dy)[] neighbors = TerrainBuilder.NeighborCells;
                // 명시 마스크를 고정 방향의 이웃 입력으로만 변환하는 반복문이다. 2026-11-07 R4 지형·섬 생성 1차 이식에서 추가됐다.
                for (int direction = 0; direction < neighbors.Length; ++direction)
                {
                    (int dx, int dy) = neighbors[direction];
                    context.Codes[((70 + dx) * 256) + 70 + dy] = (mask & (1u << direction)) != 0u ? (byte)'x' : (byte)'.';
                }

                result = growth.RemoveSparseCell(70, 70, ArchSigned(input[1]));
            }
            else if (kind == "Sparse")
            {
                Assert.Equal(2, input.Length);
                globals.NoRegion = ArchSigned(input[1]);
                growth.RemoveSparse();
            }
            else
            {
                Assert.Equal("Clear", kind);
                Assert.Equal(2, input.Length);
                globals.NoRegion = ArchSigned(input[1]);
                ChunkRect[] bounds =
                {
                    new(4, 4, 6, 5),
                    new(3, 3, 4, 5),
                    new(6, 5, 4, 4),
                    new(4, 4, 4, 5),
                    new(4, 5, 6, 4),
                    new(0x10000004, 4, 0x10000005, 5),
                };
                growth.ClearPlacement(bounds[uint.Parse(input[0], CultureInfo.InvariantCulture)]);
            }

            byte[] codes = TerrainGrowthTests.GrowthBytes(row[5]);
            byte[] regions = TerrainGrowthTests.GrowthBytes(row[6]);
            Assert.Equal(codes, context.Codes);
            Assert.Equal(regions, context.Regions);
            Assert.Equal(uint.Parse(row[3], CultureInfo.InvariantCulture), result ? 1u : 0u);
            Assert.Equal(uint.Parse(row[4], CultureInfo.InvariantCulture), seed);
            ++checkedRows;
        }

        Assert.Equal(expected, checkedRows);
    }
}
