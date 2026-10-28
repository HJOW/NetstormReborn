using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(paletteflash-x86.tsv: 768행)로 번개 팔레트 프로세스의 L1 일치를 검사한다.
/// 근거: cpppj/tests/PaletteFlashTests.cpp. 기대값은 cpppj가 원본 exe를 격리 실행해 얻은 값이며 C# 결과로 바꾸지 않는다. (R2a, 2026-10-28)
/// </summary>
public sealed class X86PaletteFlashTests
{
    /// <summary>fixture 행 수(세 원본의 초기화·색 복사·간격과 10.78 전체 진행·세 판본 중단).</summary>
    private const int Rows = 768;

    /// <summary>fixture의 double 바이트(little-endian hex)를 복원한다. NaN·무한대도 원본 저장 값을 유지한다.</summary>
    private static double ReadDouble(string hex) => BitConverter.UInt64BitsToDouble(HexDoubleBits(hex));

    /// <summary>little-endian byte hex를 그대로 합쳐 double 비트 패턴으로 읽는다.</summary>
    private static ulong HexDoubleBits(string hex)
    {
        byte[] bytes = Convert.FromHexString(hex);
        ulong word = 0;
        // 원본의 8바이트를 각각 정해진 자리로 합친다
        for (int i = 0; i < 8; i++) word |= (ulong)bytes[i] << (i * 8);
        return word;
    }

    /// <summary>부분 팔레트를 little-endian byte hex로 바꾼다. 예약 바이트도 실제 관찰과 대조한다.</summary>
    private static string ColorHex(ScreenColor[] colors, int count)
    {
        if (count == 0)
        {
            return "-";
        }
        var hex = new char[count * 8];
        for (int i = 0; i < count * 4; i++)
        {
            byte value = i % 4 == 0 ? colors[i / 4].Blue : i % 4 == 1 ? colors[i / 4].Green : i % 4 == 2 ? colors[i / 4].Red : colors[i / 4].Reserved;
            hex[i * 2] = "0123456789abcdef"[value >> 4];
            hex[i * 2 + 1] = "0123456789abcdef"[value & 15];
        }
        return new string(hex);
    }

    /// <summary>byte hex를 부분 팔레트의 메모리 배치로 읽는다. 시작 번호 위치부터 채운다. 색의 R/B 순서를 바꾸지 않는다.</summary>
    private static void ReadColors(string hex, ScreenColor[] colors, int start, int count)
    {
        if (count == 0)
        {
            return;
        }
        byte[] bytes = Convert.FromHexString(hex);
        // 네 바이트 모두 입력 자료 그대로 복원한다
        for (int i = 0; i < count; i++)
        {
            colors[start + i] = new ScreenColor(bytes[i * 4], bytes[i * 4 + 1], bytes[i * 4 + 2], bytes[i * 4 + 3]);
        }
    }

    /// <summary>세 원본의 초기화·색 복사·간격과 10.78의 전체 진행·세 판본의 중단을 대조한다.</summary>
    [Fact]
    public void NativeBodiesMatchX86()
    {
        string[][] rows = X86Fixture.Read("paletteflash");
        Assert.Equal(Rows, rows.Length);
        // 주석을 건너뛰고 각 독립 입력을 복원 모듈에 전달한다
        foreach (string[] row in rows)
        {
            string edition = row[0];
            string op = row[1];
            bool full = X86Fixture.Int(row[2]) != 0;
            var state = new PaletteFlashState
            {
                Start = X86Fixture.UInt(row[5]),
                Count = X86Fixture.UInt(row[6]),
                Duration = op == "entry" ? PaletteFlashRange.ThunderDuration(full) : ReadDouble(row[4]),
                Offset = op == "entry" ? PaletteFlashRange.ThunderOffset(X86Fixture.Int(row[3])) : X86Fixture.Int(row[3]),
                Solid = ColorFromDword(X86Fixture.UInt(row[10])),
            };
            var palette = new ScreenColor[256];
            ReadColors(row[11], palette, (int)state.Start, (int)state.Count);
            double wall = ReadDouble(row[7]);
            var trace = new List<string>();
            int reads = 0;
            var flash = new PaletteFlash(state, new PaletteFlashHooks
            {
                WallSeconds = () => wall,
                FullScreen = () => full,
                ReadPalette = (first, size, destination) =>
                {
                    Assert.Equal(state.Start, first);
                    Assert.Equal(state.Count, size);
                    Array.Copy(palette, (int)first, destination, 0, (int)size);
                    reads++;
                },
                ApplyPalette = (first, size, colors) =>
                    trace.Add($"apply:{first}:{size}:" + (size == 0 ? "" : ColorHex(colors, (int)size))),
                RestorePalette = () => trace.Add("restore"),
            });
            flash.Init();
            Assert.Equal(row[12], ColorHex(state.Current, (int)state.Count));
            if (op != "entry")
            {
                state.Steps = X86Fixture.Int(row[9]);
            }
            Assert.Equal(2, reads);
            Assert.Equal(row[13], ColorHex(state.Original, (int)state.Count));
            Assert.Equal(HexDoubleBits(row[15]), BitConverter.DoubleToUInt64Bits(state.Interval));
            // CD·10.37 진행은 초기화되지 않은 스택을 읽는 원본 버그가 있어 10.78과 같다고 취급하지 않는다
            if (edition != "originals" && op != "cancel")
            {
                continue;
            }
            if (op == "cancel")
            {
                flash.Cancel();
            }
            else
            {
                wall = ReadDouble(row[8]);
                if (!flash.Frame())
                {
                    trace.Add("kill");
                }
            }
            Assert.Equal(2, reads);
            Assert.Equal(row[13], ColorHex(state.Original, (int)state.Count));
            Assert.Equal(row[14], ColorHex(state.Current, (int)state.Count));
            Assert.Equal(HexDoubleBits(row[15]), BitConverter.DoubleToUInt64Bits(state.Interval));
            Assert.Equal(HexDoubleBits(row[16]), BitConverter.DoubleToUInt64Bits(state.Last));
            Assert.Equal(X86Fixture.Int(row[17]), state.Steps);
            Assert.Equal(X86Fixture.Int(row[18]) != 0, state.Initialized);
            Assert.Equal(row[19] == "-" ? [] : row[19].Split('|'), trace);
        }
    }

    /// <summary>DWORD를 RGBQUAD 바이트 순서(파랑·초록·빨강·예약)로 바꾼다.</summary>
    private static ScreenColor ColorFromDword(uint value) =>
        new((byte)(value & 255), (byte)((value >> 8) & 255), (byte)((value >> 16) & 255), (byte)(value >> 24));
}
