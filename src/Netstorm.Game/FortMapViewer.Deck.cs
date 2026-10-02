using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 공개 캠페인의 왼쪽 생산 창(덱). 원본 Combatgump(FUN_0043e640)처럼 위에서부터 Storm Power, 다리 칸 2열,
/// 유닛 칸 1열(골렘 → 워크샵에 등록한 유닛), 미니맵 순서다. 버튼 격자는 없고, 건물은 사제 우클릭 Construct 로 짓는다.
/// 항목 상태는 원본 그리기 모드를 따른다: 재충전 중·집은 다리 = 어둡게, Storm Power 부족 = 빨갛게
/// (docs/screens/clone-deck.md, 2026-09-30·2026-10-01 녹화 대조).
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>다리 칸 패널의 위쪽 끝 y (FUN_0043e640 의 다리 패널 위치 0x41d00000 = 26)</summary>
    private const int DeckTrayTop = 26;

    /// <summary>다리 칸 한 행의 높이 (패널 높이 = 칸 수 × 18 → 2열이므로 한 행 36)</summary>
    private const int DeckTrayRowHeight = 36;

    /// <summary>다리 칸 한 열의 폭 (사이드바 안쪽 80px 을 2열로 나눔, 녹화의 조각 중심 x 20·60)</summary>
    private const int DeckTrayColumnWidth = 40;

    /// <summary>생산 창 안쪽 폭 (오른쪽 테두리 제외)</summary>
    private const int DeckInnerWidth = 80;

    /// <summary>다리 칸의 조각 그림 배율 (다리 패널 +0xc0 = 0x3f2147ae ≈ 0.63)</summary>
    private const float DeckTrayPieceScale = 0.63f;

    /// <summary>유닛 칸의 최소 행 수 (FUN_0043d790(1, 6, 8): 1열 · 최소 6행 · 유닛 플래그 8)</summary>
    private const int DeckUnitMinimumRows = 6;

    /// <summary>유닛 칸 아래 끝에서 화면 아래까지의 높이 (미니맵 영역. 1024×768 녹화의 유닛 칸 간격 약 94px에 맞춘 값)</summary>
    private const int DeckUnitBottomMargin = 70;

    /// <summary>유닛 그림이 칸보다 크면 줄일 때 둘 여백 (원본 그리기 함수의 +2.0)</summary>
    private const int DeckPictureMargin = 2;

    /// <summary><see cref="GetTexture"/> 에서 어둡게 변환표를 고르는 색 번호 (섬 소유자 색 1~8과 겹치지 않게 음수)</summary>
    private const int DeckDarkColor = -1;

    /// <summary><see cref="GetTexture"/> 에서 빨갛게 변환표를 고르는 색 번호</summary>
    private const int DeckRedColor = -2;

    /// <summary>재충전 중·집은 항목의 어둡게 변환표 (원본 DAT_00556438)</summary>
    private byte[]? _deckDarkTable;

    /// <summary>Storm Power 부족 항목의 빨갛게 변환표 (원본 DAT_00556038)</summary>
    private byte[]? _deckRedTable;

    /// <summary>덱 유닛 칸 하나: 칸 번호와 타입</summary>
    /// <param name="Slot">유닛 칸 번호 (0 = 골렘 자리)</param>
    /// <param name="Type">유닛 타입</param>
    private sealed record DeckUnit(int Slot, TypeInfo Type);

    /// <summary>GetTexture 의 음수 색 번호에 맞는 덱 변환표 (해당 없으면 null)</summary>
    private ReadOnlyMemory<byte>? DeckRemap(int color) => color switch
    {
        DeckDarkColor => _deckDarkTable ??= ProductionTintRemap.Darkened(_palette),
        DeckRedColor => _deckRedTable ??= ProductionTintRemap.Reddened(_palette),
        _ => null,
    };

    /// <summary>다리 칸 자리의 화면 영역 (2열 행 우선)</summary>
    /// <param name="slot">칸 번호</param>
    private static Rectangle DeckTraySlotBounds(int slot) => new(slot % TrayColumns * DeckTrayColumnWidth,
        DeckTrayTop + slot / TrayColumns * DeckTrayRowHeight, DeckTrayColumnWidth, DeckTrayRowHeight);

    /// <summary>유닛 칸 패널의 위쪽 끝 (다리 칸 패널 바로 아래)</summary>
    private int DeckUnitTop => DeckTrayTop + (_session.Player(TestPlayer).Tray.Capacity + TrayColumns - 1) / TrayColumns * DeckTrayRowHeight;

    /// <summary>
    /// 지금 덱에 있는 유닛 칸. 0번은 템플이 공급하는 골렘 자리이고(기술 표가 골렘을 막으면 자리를 두지 않는다),
    /// 그 뒤는 워크샵 번호·등록 순서다. 원본은 항목마다 칸 번호를 가지며 녹화의 순서(골렘 → Rain Generator → Sun Cannon → Whirlibase)와 같다.
    /// </summary>
    private List<DeckUnit> DeckUnits()
    {
        PlayerState player = _session.Player(TestPlayer);
        var units = new List<DeckUnit>();
        int next = player.Tech.IsAllowed(ProductionDeck.GolemType) ? 1 : 0;
        // 덱 항목 순서대로 칸 번호를 매긴다 (다리는 다리 칸 패널이 따로 그린다)
        foreach (DeckEntry entry in player.Deck.Entries())
        {
            if (entry.Kind == DeckEntryKind.Bridge
                || _candidates.FirstOrDefault(t => t.Name.Equals(entry.TypeName, StringComparison.OrdinalIgnoreCase)) is not { } type)
            {
                continue;
            }
            units.Add(new DeckUnit(entry.Kind == DeckEntryKind.Golem ? 0 : next++, type));
        }
        return units;
    }

    /// <summary>
    /// 유닛 칸의 화면 영역. 원본 FUN_0043e1e0 처럼 1열이고 행 수는 max(최소 6, 마지막 칸 번호 + 1)이며,
    /// 패널 높이를 행 수로 나눈다.
    /// </summary>
    /// <param name="slot">유닛 칸 번호</param>
    /// <param name="rows">행 수</param>
    /// <param name="height">화면 높이</param>
    private Rectangle DeckUnitBounds(int slot, int rows, int height)
    {
        int top = DeckUnitTop;
        float rowHeight = Math.Max(1, height - DeckUnitBottomMargin - top) / (float)rows;
        return new Rectangle(0, top + (int)(slot * rowHeight), DeckInnerWidth, (int)rowHeight);
    }

    /// <summary>유닛 칸의 행 수 (최소 6)</summary>
    private static int DeckRows(List<DeckUnit> units) => Math.Max(DeckUnitMinimumRows, units.Count == 0 ? 0 : units.Max(u => u.Slot) + 1);

    /// <summary>화면 좌표의 다리 칸 번호 (조각이 없는 칸이나 패널 밖이면 null)</summary>
    private int? DeckTraySlotAt(Point point)
    {
        BridgeTray tray = _session.Player(TestPlayer).Tray;
        // 칸 자리마다 영역을 비교한다 (집은 칸도 다시 집기 위해 포함)
        for (int slot = 0; slot < tray.Capacity; slot++)
        {
            if (tray.Slots[slot] != null && DeckTraySlotBounds(slot).Contains(point)) return slot;
        }
        return null;
    }

    /// <summary>화면 좌표의 덱 유닛 (없으면 null)</summary>
    private DeckUnit? DeckUnitAt(Point point, int height)
    {
        List<DeckUnit> units = DeckUnits();
        int rows = DeckRows(units);
        return units.FirstOrDefault(u => DeckUnitBounds(u.Slot, rows, height).Contains(point));
    }

    /// <summary>
    /// 덱 유닛의 그리기 상태: 재충전 중이면 어둡게(모드 1)가 먼저이고, 아니면 Storm Power 가 비용보다 적을 때 빨갛게(모드 2)다.
    /// 2026-09-30 녹화 03:30 부근에서 방금 놓은 유닛만 어둡고 나머지 400 SP 유닛은 빨간 것을 확인했다.
    /// </summary>
    private int DeckUnitColor(TypeInfo type)
    {
        PlayerState player = _session.Player(TestPlayer);
        if (_session.IsUnitRecharging(TestPlayer, type.Name)) return DeckDarkColor;
        return StormPower.TypeCost(type.Definition) > player.StormPower ? DeckRedColor : 0;
    }

    /// <summary>다리 칸·유닛 칸을 그린다 (사이드바 바탕 위, 미니맵 위쪽).</summary>
    private void DrawDeck(SpriteBatch batch, int height)
    {
        BridgeTray tray = _session.Player(TestPlayer).Tray;
        TypeFrameTable frames = _bridgeType.Definition.Frames;
        // 다리 칸: 빈 칸은 바탕만 보이고, 집은 조각은 제자리에 어둡게 남는다
        for (int slot = 0; slot < tray.Capacity; slot++)
        {
            if (tray.Slots[slot] is not { } piece) continue;
            Rectangle box = DeckTraySlotBounds(slot);
            var size = new Vector2(piece.Width * FortMap.CellPixelWidth, piece.Height * FortMap.CellPixelHeight) * DeckTrayPieceScale;
            Vector2 origin = new Vector2(box.Center.X, box.Center.Y) - size / 2;
            int color = tray.HeldSlot == slot ? DeckDarkColor : 0;
            // 회전된 칸마다 기준점(칸 오른쪽 아래)에 원본 프레임을 그린다
            foreach (PlacedBridgeCell cell in piece.Cells())
            {
                var anchor = origin + new Vector2((cell.Dx + 1) * FortMap.CellPixelWidth, (cell.Dy + 1) * FortMap.CellPixelHeight) * DeckTrayPieceScale;
                DrawSprite(batch, _bridgeType.LoadIndex, BridgeFrames.Find(frames, cell.Cell), anchor, color, scale: DeckTrayPieceScale);
            }
        }
        List<DeckUnit> units = DeckUnits();
        int rows = DeckRows(units);
        // 유닛 칸: 원래 크기로 칸 가운데에 그리고, 칸보다 크면 줄인다
        foreach (DeckUnit unit in units)
        {
            DrawDeckPicture(batch, unit.Type, DeckUnitBounds(unit.Slot, rows, height), DeckUnitColor(unit.Type));
        }
    }

    /// <summary>유닛 기본 프레임을 덱 변환표와 함께 칸 가운데에 그린다 (원본 FUN_0043ce40 의 그림 배율 규칙).</summary>
    private void DrawDeckPicture(SpriteBatch batch, TypeInfo type, Rectangle box, int color)
    {
        if (GetTexture(type.LoadIndex, MapSpriteFrames.BodyFrame(type.Definition, type.Definition.Frames.DefaultFrame), color) is not { } sprite)
        {
            return;
        }
        Texture2D texture = sprite.Texture;
        // 칸보다 크면(여백 포함) 줄이고, 작으면 원래 크기로 둔다
        float scale = Math.Min(1f, Math.Min(box.Width / (float)(texture.Width + DeckPictureMargin),
            box.Height / (float)(texture.Height + DeckPictureMargin)));
        var size = new Vector2(texture.Width, texture.Height) * scale;
        batch.Draw(texture, new Vector2(box.Center.X - size.X / 2, box.Center.Y - size.Y / 2), null, Color.White, 0f, Vector2.Zero,
            scale, SpriteEffects.None, 0f);
    }
}
