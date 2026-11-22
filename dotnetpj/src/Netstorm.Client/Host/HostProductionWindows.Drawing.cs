using Netstorm.Client.Display;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Host; // 같은 실제 공통 클립/색표/표면에서 생산 파생을 출력한다.

/// <summary>Source 현재 Color/Draw/SetPalette의 실제 소비 장치다.</summary>
public sealed partial class HostProductionWindows
{
    /// <summary>현재 원본 색표 토큰을 같은 실제 BYTE 배열로 해석한다.</summary>
    private byte[] Color(uint token)
    {
        if (token == 0x556038) return _colors.Disabled; if (token == 0x556338) return _colors.Underlay; if (token == 0x556438) return _shades.Dark; if (_context.OwnerColorTables is { } tables) return tables.ColorToken(token);
        // 독립 문맥의 실제 등록 표만 허용한다.
        foreach (var pair in _localColors) { uint first = _imageState.OwnerMaps[pair.Key]; if (token >= first && token < first + 2304 && (token - first) % 256 == 0) return pair.Value.Tables[(token - first) / 256]; } throw new InvalidOperationException("생산 아이콘 현재 색표 토큰 없음");
    }
    /// <summary>현재 동일 표면을 호출 동안만 빌리고 모든 파생을 원문 장치로 출력한다.</summary>
    private void Draw(uint token, GumpPixelTarget target, GumpGlyphColorState colors, GumpBackgroundDraw background)
    {
        Prepare(); var node = Node(token); if (node is ProductionHintNode) { _hint.Draw(token); return; } if (node is TextGumpNode) { _text.Single.Draw(token); return; } if (node is GumpBackgroundNode) { background.Draw(token); return; }
        if (node is InventoryItemNode) { _activeBackground = background; try { _item.Draw(token); } finally { _activeBackground = null; } return; }
        if (node is ImageGumpNode) { _activeImage = new ImageGumpPixelTarget(target, _imageAssets, colors.VfxColors, Color).Hooks(new()); try { _image.Draw(token); } finally { _activeImage = null; } }
    }
    /// <summary>파일 팔레트/고정 최초 명도/기존9표로 원문 실제 소비 표를 설치한다.</summary>
    public void SetPalette(ReadOnlySpan<uint> logical, ReadOnlySpan<uint> colorSortLogical = default, IReadOnlyList<byte[]>? colorMaps = null)
    {
        if (logical.Length != 256 || (!colorSortLogical.IsEmpty && colorSortLogical.Length != 256)) throw new ArgumentException("생산 논리 팔레트256색 필요"); var fixedInput = colorSortLogical.IsEmpty ? logical : colorSortLogical;
        if (colorMaps is null) { _colors = ProductionColorMapBuilder.BuildProductionColorMaps(fixedInput); _shades = PaletteShade.BuildGumpShadeMaps(fixedInput); }
        else { if (colorMaps.Count != 9 || colorMaps.Any(map => map.Length != 256)) throw new ArgumentException("생산 현재 공통9색표 물리 계약", nameof(colorMaps)); _colors.Disabled = colorMaps[2].ToArray(); _colors.Underlay = colorMaps[5].ToArray(); _shades = new(); colorMaps[4].CopyTo(_shades.Bright, 0); colorMaps[6].CopyTo(_shades.Dark, 0); }
        var palette = PaletteColors.BuildPaletteColorTable(logical); _text.ProductionHints.White = palette.Basic[7]; _text.ProductionHints.Black = palette.Basic[0];
        if (_context.OwnerColorTables is null)
        {
            // 기존 독립 입력만 파일 팔레트의 실제 전용 표를 만든다.
            foreach (uint type in new uint[] { 111, 141, 137, 94, 97 }) _localColors[type] = OwnerColorMapBuilder.BuildSpecialOwnerColorMaps(fixedInput, type);
        }
    }
}
