using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(palettecolors-x86.tsv: 3판본 × 8행)로 파일 팔레트 로더와 전체 색 표의 L1 일치를 검사한다.
/// 근거: cpppj/tests/PaletteColorsTests.cpp. 기대값은 cpppj가 원본 exe를 격리 실행해 얻은 값이며 C# 결과로 바꾸지 않는다. (R2a, 2026-10-28)
/// </summary>
public sealed class X86PaletteColorsTests
{
    /// <summary>fixture 행 수(판본 3개 × 관측 8개).</summary>
    private const int Rows = 24;

    /// <summary>리틀 엔디언 16진수 문자열을 DWORD 배열로 바꾼다. 최상위 플래그 바이트도 보존한다.</summary>
    private static uint[] Dwords(string hex)
    {
        var result = new uint[hex.Length / 8];
        // DWORD마다 네 바이트를 원본 순서(낮은 주소가 낮은 자리)로 합친다.
        for (int i = 0; i < result.Length; i++)
            for (int b = 0; b < 4; b++)
                result[i] |= (uint)Convert.FromHexString(hex.Substring(i * 8 + b * 2, 2))[0] << (b * 8);
        return result;
    }

    /// <summary>표시 팔레트 256색을 B·G·R·예약 바이트 순서로 펼친다.</summary>
    private static byte[] PaletteBytes(Screen screen)
    {
        var output = new byte[1024];
        for (int i = 0; i < 256; i++)
        {
            ScreenColor color = screen.Palette[i];
            output[i * 4] = color.Blue;
            output[i * 4 + 1] = color.Green;
            output[i * 4 + 2] = color.Red;
            output[i * 4 + 3] = color.Reserved;
        }
        return output;
    }

    /// <summary>원본의 파일 변환·예약 바이트·기본 9색·전체 58·50색·날씨 별칭이 같은 입력에서 일치한다.</summary>
    [Fact]
    public void FileLoaderAndColorTableMatchNative()
    {
        string[][] rows = X86Fixture.Read("palettecolors");
        Assert.Equal(Rows, rows.Length);
        // 관측마다 이전 팔레트와 파일 팔레트를 순서대로 읽고 색 표를 대조한다
        foreach (string[] row in rows)
        {
            bool patch = row[0] == "originals";
            var screen = new Screen(640, 480);
            screen.SetMode(ScreenMode.BackBufferDib | ScreenMode.ModeSet);
            screen.LoadPalette(new GamePalette(X86Fixture.Bytes(row[4])), patch1078: patch);
            screen.LoadPalette(new GamePalette(X86Fixture.Bytes(row[3])), patch1078: patch);
            // 저장 팔레트 256색 전체가 같다
            Assert.Equal(X86Fixture.Bytes(row[5]), PaletteBytes(screen));
            PaletteColorTable colors = screen.Colors();
            // 기본 배열과 날씨 별칭은 실제 전역의 최종 바이트와 같다
            uint[] basic = Dwords(row[6]);
            for (int i = 0; i < colors.Basic.Length; i++) Assert.Equal(basic[i], colors.Basic[i]);
            uint[] weather = Dwords(row[7]);
            for (int i = 0; i < colors.Weather.Length; i++) Assert.Equal(weather[i], colors.Weather[i]);
            // 각 항목은 R,G,B,index 네 정수다
            int count = 0;
            foreach (string item in row[9].Split('|'))
            {
                string[] values = item.Split(',');
                Assert.Equal(4, values.Length);
                NamedPaletteColor color = colors.Named[count];
                Assert.Equal(byte.Parse(values[0]), color.Red);
                Assert.Equal(byte.Parse(values[1]), color.Green);
                Assert.Equal(byte.Parse(values[2]), color.Blue);
                Assert.Equal(uint.Parse(values[3]), color.Index);
                Assert.Equal(color.Index, screen.FindColor(color.Red, color.Green, color.Blue));
                count++;
            }
            Assert.Equal(colors.Named.Count, count);
            Assert.Equal(patch ? PaletteColors.PatchColorCount : PaletteColors.CdColorCount, count);
            // 원본 논리 플래그는 예약 바이트가 아닌 4다. RGB는 표시 팔레트와 함께 검사한다
            uint[] logical = Dwords(row[8]);
            for (int i = 0; i < 256; i++)
            {
                ScreenColor color = screen.Palette[i];
                Assert.Equal((logical[i] & 0xffffffu), (uint)(color.Red | (color.Green << 8) | (color.Blue << 16)));
                Assert.Equal((i == 0 || i == 255) ? 0u : 4u, logical[i] >> 24);
            }
        }
    }
}
