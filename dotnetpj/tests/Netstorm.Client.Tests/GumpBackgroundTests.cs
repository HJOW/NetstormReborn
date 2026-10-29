using System.Buffers.Binary;
using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Tests; // 원본 전체 질감 호출과 명암표·픽셀 계약을 검사한다.

/// <summary>원본177행과 독립8비트 출력/안전 경계.
/// 근거: cpppj/tests/GumpBackgroundTests.cpp. GDI 참조 DC 대신 순수 Screen을 사용한다.</summary>
public sealed class GumpBackgroundTests
{
    /// <summary>계획만 원본 관측 형식으로 직렬화한다. 기대 호출 목록은 fixture에서 읽는다.</summary>
    private static string Events(IEnumerable<GumpTexturePass> passes, ScreenRect clip)
    {
        var events = new List<string>();
        // 실제 질감 호출 순서에 clip/map/draw/복구를 기록한다.
        foreach (GumpTexturePass p in passes)
        {
            events.Add($"p,{GumpRawSupport.Coordinates(p.Rect)},{p.Flags}"); if (!p.ValidTile) continue;
            events.Add("c," + GumpRawSupport.Coordinates(p.Clip)); if (p.Shade != 0) events.Add("m," + p.Shade);
            // 빈 클립도 실제 여분 타일 호출은 그대로 남긴다.
            foreach (ScreenPoint o in p.Origins) events.Add($"d,{o.X},{o.Y},{p.Shade}");
            events.Add("c," + GumpRawSupport.Coordinates(clip));
        }
        return string.Join('|', events);
    }

    /// <summary>세 판본과 두 x87 정밀도에서 같았던 전체 명암표/질감 몸체를 대조한다.</summary>
    [Fact]
    public void NativeMapsAndTextureBodies()
    {
        string[][] rows = X86Fixture.Read("gumpbackground"); Assert.Equal(177, rows.Length); int maps = 0;
        // 독립 원본 입력과 전체 결과만 사용한다.
        foreach (string[] r in rows)
        {
            if (r[1] == "maps")
            {
                byte[] raw = X86Fixture.Bytes(r[3]); Assert.Equal(1024, raw.Length); uint[] logical = new uint[256];
                // PALETTEENTRY의 예약 바이트도 포함해 원본 DWORD를 읽는다.
                for (int i = 0; i < logical.Length; i++) logical[i] = BinaryPrimitives.ReadUInt32LittleEndian(raw.AsSpan(i * 4));
                GumpShadeMaps result = PaletteShade.BuildGumpShadeMaps(logical); Assert.Equal(X86Fixture.Bytes(r[4]), result.Bright); Assert.Equal(X86Fixture.Bytes(r[5]), result.Dark); maps++;
            }
            else
            {
                Assert.Equal("draw", r[1]); var rect = new ScreenRect(X86Fixture.Int(r[3]), X86Fixture.Int(r[4]), X86Fixture.Int(r[5]), X86Fixture.Int(r[6]));
                var clip = new ScreenRect(X86Fixture.Int(r[7]), X86Fixture.Int(r[8]), X86Fixture.Int(r[9]), X86Fixture.Int(r[10]));
                Assert.Equal(r[14], Events(GumpBackground.PlanGumpBackground(rect, clip, (short)X86Fixture.Int(r[12]), (short)X86Fixture.Int(r[13]), X86Fixture.UInt(r[11])), clip));
            }
        }
        Assert.Equal(9, maps);
    }

    /// <summary>밝음에서 애니메이션 후보를 제외하고 어두움은 허용하며 예약 바이트는 RGB에 섞지 않는다.</summary>
    [Fact]
    public void ShadeExcludesAnimatedBrightCandidates()
    {
        uint[] logical = Enumerable.Repeat(0xffffffU, 256).ToArray(); logical[0] = 0; logical[10] = 0x646464; logical[50] = 0x959595; logical[229] = 0x969696; logical[245] = 0x3c3c3c;
        GumpShadeMaps maps = PaletteShade.BuildGumpShadeMaps(logical); Assert.Equal(50, maps.Bright[10]); Assert.Equal(245, maps.Dark[10]); Assert.Equal(0, maps.Bright[0]);
        logical[229] = 0; logical[10] |= 0xff000000; maps = PaletteShade.BuildGumpShadeMaps(logical); Assert.Equal(50, maps.Bright[10]); Assert.Equal(245, maps.Dark[10]);
    }

    /// <summary>화면 격자·2픽셀 명암·겹치는 모서리·투명/상대영역을 작은 수동 기대 픽셀에 대조한다.</summary>
    [Fact]
    public void TexturePixelsAndPressedBorders()
    {
        var texture = new IndexedImage { Width = 3, Height = 3, Indices = [1, 2, 1, 3, 4, 3, 1, 2, 1], Opacity = Enumerable.Repeat((byte)255, 9).ToArray() }; GumpShadeMaps maps = GumpBackgroundTestAssets.Maps();
        // 눌림은 바탕을 바꾸지 않고 테두리의 명암 방향만 바꾼다.
        foreach (bool pressed in new[] { false, true })
        {
            var canvas = new IndexedImage { Width = 8, Height = 8, Indices = Enumerable.Repeat((byte)9, 64).ToArray(), Opacity = Enumerable.Repeat((byte)255, 64).ToArray() };
            GumpBackground.DrawGumpBackground(canvas, texture, new(), GumpBackground.PlanGumpBackground(new(1, 1, 7, 7), new(0, 0, 8, 8), 3, 3, GumpBackgroundFlags.Button | (pressed ? GumpBackgroundFlags.Pressed : 0)), maps);
            Assert.Equal(4, canvas.Indices[27]); Assert.Equal(pressed ? 24 : 14, canvas.Indices[11]); Assert.Equal(pressed ? 12 : 22, canvas.Indices[51]);
            Assert.Equal(pressed ? 23 : 13, canvas.Indices[14]); Assert.Equal(pressed ? 12 : 22, canvas.Indices[49]); Assert.Equal(9, canvas.Indices[0]); Assert.Equal(9, canvas.Indices[63]); Assert.All(canvas.Opacity, b => Assert.Equal(255, b));
        }
        var output = new IndexedImage { Width = 8, Height = 8, Indices = Enumerable.Repeat((byte)9, 64).ToArray() }; texture.Opacity.AsSpan().Clear();
        GumpBackground.DrawGumpBackground(output, texture, new(-1, -1), GumpBackground.PlanGumpBackground(new(1, 1, 7, 7), new(0, 0, 8, 8), 3, 3, GumpBackgroundFlags.Button), maps); Assert.All(output.Indices, b => Assert.Equal(9, b));
        texture.Opacity.AsSpan().Fill(255); GumpBackground.DrawGumpBackground(output, texture, new(1, 1), GumpBackground.PlanGumpBackground(new(1, 1, 7, 7), new(3, 3, 5, 5), 3, 3, GumpBackgroundFlags.Texture), maps);
        Assert.Equal(1, output.Indices[27]); Assert.Equal(4, output.Indices[36]); Assert.Equal(9, output.Indices[19]); Assert.Equal(9, output.Indices[29]);
    }

    /// <summary>일시 팔레트/재적용은 명암표를 보존하고 새 파일만 갱신한다.</summary>
    [Fact]
    public void MapsRemainUntilNextPaletteFile()
    {
        var screen = new Screen(640, 480); byte[] col = new byte[776];
        // 파일 입력은 회색 계조이며 기대표는 이전 파일 결과의 보존만 검사한다.
        for (int i = 0; i < 256; i++) col[8 + i * 3] = col[9 + i * 3] = col[10 + i * 3] = (byte)i;
        screen.LoadPalette(new GamePalette(col)); GumpShadeMaps saved = screen.ShadeMaps(); screen.SetPalette(80, 1, [new(200, 200, 200, 0)], true); screen.SetPalette(0, 256, null, true);
        Assert.Equal(saved.Bright, screen.ShadeMaps().Bright); Assert.Equal(saved.Dark, screen.ShadeMaps().Dark);
        col[8 + 80 * 3] = col[9 + 80 * 3] = col[10 + 80 * 3] = 250; screen.LoadPalette(new GamePalette(col)); Assert.False(saved.Bright.SequenceEqual(screen.ShadeMaps().Bright) && saved.Dark.SequenceEqual(screen.ShadeMaps().Dark));
    }

    /// <summary>안전 API의 역전/fill 경로·과대한/감기는 타일 반복을 거부한다.</summary>
    [Fact]
    public void InvalidPathsAndExcessiveLoops()
    {
        // 질감 전용 계획에서 도달해서는 안 되는 세 분기 입력이다.
        foreach (uint flags in new uint[] { 0, 0x06000000, 0x0a000000 }) Assert.Throws<ArgumentException>(() => GumpBackground.PlanGumpBackground(new(0, 0, 4, 4), new(0, 0, 8, 8), 3, 3, flags));
        Assert.Throws<ArgumentException>(() => GumpBackground.PlanGumpBackground(new(4, 0, 0, 4), new(0, 0, 8, 8), 3, 3, GumpBackgroundFlags.Button));
        Assert.Throws<ArgumentOutOfRangeException>(() => GumpBackground.PlanGumpBackground(new(0, 0, 100000, 100000), new(0, 0, 8, 8), 2, 2, GumpBackgroundFlags.Button));
        Assert.Throws<ArgumentException>(() => GumpBackground.PlanGumpBackground(new(int.MaxValue, 0, int.MaxValue, 4), new(0, 0, 8, 8), 3, 3, GumpBackgroundFlags.Button));
    }
}
