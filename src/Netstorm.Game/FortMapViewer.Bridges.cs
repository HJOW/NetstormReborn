using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 맵 뷰어의 다리 조각 시험 모드 (B): 게임 세션이 플레이어 1 의 다리 칸을 원본 규칙(템플이 있으면 1초마다, Bridge Slots 칸 수,
/// 5번째마다 한 칸 조각)대로 채우고, 조각을 집어(1~6) 돌려(R) 놓는다(좌클릭). 집기·되돌리기·놓기는 모두 세션 명령이며
/// 배치 판정과 붕괴는 세션의 BridgeGrid(섬·다른 다리와 겹침 불가, 섬 가장자리나 내 다리 열린 끝에 이어짐,
/// 열린 끝이 있는 연결망은 10초마다 수명이 줄어 금 간 뒤 무너짐)가 한다 — docs/exe/bridge-pieces.md 8절.
/// 조각의 회전은 커서(화면)가 관리하고 놓을 때 명령에 값으로 실어 보낸다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>다리 칸 패널의 한 칸 크기(논리 픽셀)</summary>
    private static readonly Point TraySlotSize = new(112, 84);

    /// <summary>다리 칸 패널의 열 수 (원본 사이드바처럼 2열)</summary>
    private const int TrayColumns = 2;

    /// <summary>다리 칸 패널 안 조각 배율 (원본 사이드바도 조각을 절반 크기로 그린다 — 2026-09-29 원본 캡처)</summary>
    private const float TrayPieceScale = 0.5f;

    /// <summary>들고 있는 조각 미리보기 불투명도</summary>
    private const float HeldPieceAlpha = 0.7f;

    /// <summary>다리 모드 아래 안내 영역의 높이 (제목·알림·조작 키 3줄)</summary>
    private const int BridgePanelHeight = 94;

    /// <summary>숫자 키 → 다리 칸 번호 (1~6)</summary>
    private static readonly Keys[] TrayKeys = [Keys.D1, Keys.D2, Keys.D3, Keys.D4, Keys.D5, Keys.D6];

    /// <summary>들고 있는 조각을 놓을 수 없을 때의 색 (원본은 조각 전체를 빨강으로 칠한다)</summary>
    private static readonly Color CannotPlaceTint = new(255, 40, 40);

    /// <summary>bridge 타입 (셰이프 블록·프레임 표)</summary>
    private TypeInfo _bridgeType = null!;

    /// <summary>다리 조각 시험 모드가 켜졌는지</summary>
    private bool _bridgeMode;

    /// <summary>들고 있는 조각의 회전 번호 (화면이 관리한다. 조각을 새로 집으면 0)</summary>
    private int _heldRotation;

    /// <summary>들고 있는 조각의 현재 위치 판정 (빨강 표시용)</summary>
    private BridgePlacementCheck? _bridgeCheck;

    /// <summary>다리 모드 커서 칸 (조각 왼쪽 위 칸)</summary>
    private (int X, int Y)? _bridgeCursor;

    /// <summary>반대 회전(원본 C 키 / rotmode)이 켜졌는지</summary>
    private bool _reverseRotation;

    /// <summary>플레이어 1 이 집고 있는 조각과 화면 회전을 합친 미리보기 조각 (없으면 null)</summary>
    private BridgePiece? HeldPreview =>
        _session.Player(TestPlayer).HeldPiece is { } held ? new BridgePiece(held.Pattern.Index, _heldRotation) : null;

    /// <summary>bridge 타입을 찾는다 (다리 칸·격자는 세션이 이미 갖고 있다).</summary>
    private void InitializeBridges(TypeCatalog catalog)
    {
        _bridgeType = catalog.Find("bridge") ?? throw new InvalidDataException("bridge 타입이 없습니다.");
    }

    /// <summary>
    /// 다리 조각 시험 모드를 켠 채 시작한다 (명령줄 --bridges). 지정한 초만큼 게임 시간을 미리 흘려 칸을 채우고,
    /// hold 가 있으면 그 모양·회전의 조각을 든 상태로 두며 probe 칸이 있으면 카메라를 그곳에 둔다 (스크린샷 검증용).
    /// </summary>
    /// <param name="warmupSeconds">미리 흘릴 시간(초)</param>
    /// <param name="hold">들고 있을 (모양 번호, 회전 번호)</param>
    /// <param name="probe">조각 왼쪽 위 칸 (없으면 커서)</param>
    public void StartBridges(double warmupSeconds, (int Pattern, int Rotation)? hold, (int X, int Y)? probe)
    {
        _bridgeMode = true;
        _placementMode = false;
        // 게임 시간을 미리 흘려 칸을 채운다
        _session.RunTicks((int)Math.Round(warmupSeconds * _session.TicksPerSecond));
        if (hold is (int pattern, int rotation))
        {
            // 검증용이라 칸을 거치지 않고 지정한 모양을 곧바로 든다
            _session.DebugHoldBridgePiece(TestPlayer, pattern);
            _heldRotation = BridgeDirections.NormalizeRotation(rotation);
        }
        _probeCell = probe;
        if (probe is (int x, int y))
        {
            _camera = WorldPixels(x, y);
        }
    }

    /// <summary>
    /// 다리 모드 입력: B 모드, 1~6 조각 집기, R 회전, C 반대 회전 켜기/끄기, Backspace 되돌리기, 좌클릭 놓기.
    /// 집기·되돌리기·놓기는 세션 명령이고 결과는 다음 틱에 알림으로 온다.
    /// </summary>
    private void UpdateBridges(KeyboardState keyboard, MouseState mouse)
    {
        if (Pressed(keyboard, Keys.B))
        {
            _bridgeMode = !_bridgeMode;
            if (_bridgeMode)
            {
                _placementMode = false;
            }
        }
        if (!_bridgeMode)
        {
            return;
        }
        PlayerState player = _session.Player(TestPlayer);
        // 숫자 키로 칸의 조각을 집는다 (들고 있던 조각은 먼저 칸으로 되돌린다)
        for (int i = 0; i < TrayKeys.Length; i++)
        {
            if (Pressed(keyboard, TrayKeys[i]) && i < player.Tray.Pieces.Count)
            {
                if (player.HeldPiece != null)
                {
                    SubmitCommand(new ReturnBridgePieceCommand(TestPlayer));
                }
                SubmitCommand(new PickBridgePieceCommand(TestPlayer, i));
                _heldRotation = 0;
            }
        }
        if (Pressed(keyboard, Keys.C))
        {
            // 원본처럼 C 키는 회전 방향만 바꾼다 (조각은 돌리지 않는다)
            _reverseRotation = !_reverseRotation;
        }
        if (HeldPreview is { } preview && Pressed(keyboard, Keys.R))
        {
            // 원본의 오른쪽 클릭 회전 (뷰어는 오른쪽 드래그를 카메라 이동에 쓰므로 R 키로 대신한다)
            preview.RotateByPlayer(_reverseRotation);
            _heldRotation = preview.Rotation;
        }
        if (player.HeldPiece != null && Pressed(keyboard, Keys.Back))
        {
            SubmitCommand(new ReturnBridgePieceCommand(TestPlayer));
        }
        _bridgeCursor = _probeCell ?? (mouse.Y < HeaderHeight ? null : BridgeCellAt(new Vector2(mouse.X, mouse.Y)));
        _bridgeCheck = HeldPreview is { } held && _bridgeCursor is (int px, int py)
            ? _session.CheckBridge(TestPlayer, held.Pattern.Index, held.Rotation, px, py) : null;
        if (player.HeldPiece != null && _bridgeCursor is (int cx, int cy)
            && mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
        {
            if (_bridgeCheck is { Allowed: true })
            {
                // 판정을 통과한 조각만 명령으로 보낸다 (실제 놓기와 알림은 세션이 한다)
                SubmitCommand(new PlaceBridgeCommand(TestPlayer, _heldRotation, cx, cy));
            }
            else
            {
                _notice = $"놓을 수 없음: {SessionText.Describe(_bridgeCheck?.Problem ?? BridgePlacementProblem.None)}";
            }
        }
    }

    /// <summary>화면 좌표 → 들고 있는 조각의 왼쪽 위 칸 (원본 측정 규칙 BridgeCursor)</summary>
    private (int X, int Y) BridgeCellAt(Vector2 screen)
    {
        Vector2 world = (screen - _lastCenter) / _zoom + _camera;
        return BridgeCursor.TopLeftCell(world.X, world.Y);
    }

    /// <summary>놓인 다리 칸과 들고 있는 조각 미리보기를 그린다 (저장 오브젝트 뒤, 안내 영역 앞).</summary>
    private void DrawBridgeWorld(SpriteBatch batch, Vector2 center)
    {
        TypeFrameTable frames = _bridgeType.Definition.Frames;
        var stored = _session.StoredBridgeCells;
        // 게임 중 놓은 칸을 상태(보통·금 감·단단함)에 맞는 프레임으로 y·x 순서로 그린다 (저장 다리는 본 그리기에서 처리)
        foreach (BridgeCellState cell in _session.Bridges.Cells.Where(c => !stored.Contains(c)).OrderBy(c => c.Y).ThenBy(c => c.X))
        {
            DrawSprite(batch, _bridgeType.LoadIndex, BridgeFrames.Find(frames, cell.Cell, cell.Condition), Screen(WorldPixels(cell.X, cell.Y), center));
        }
        if (!_bridgeMode || HeldPreview is not { } held || _bridgeCursor is not (int cx, int cy))
        {
            return;
        }
        // 들고 있는 조각을 커서 칸 기준으로 반투명하게 그린다
        foreach (PlacedBridgeCell placed in held.Cells())
        {
            DrawSprite(batch, _bridgeType.LoadIndex, BridgeFrames.Find(frames, placed.Cell),
                Screen(WorldPixels(cx + placed.Dx, cy + placed.Dy), center), alpha: HeldPieceAlpha,
                tint: _bridgeCheck is { Allowed: false } ? CannotPlaceTint : null);
        }
    }

    /// <summary>다리 칸 패널(왼쪽)과 다리 모드 안내 문구를 그린다.</summary>
    private void DrawBridgeOverlay(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (!_bridgeMode)
        {
            return;
        }
        PlayerState player = _session.Player(TestPlayer);
        BridgeTray tray = player.Tray;
        TypeFrameTable frames = _bridgeType.Definition.Frames;
        int rows = (tray.Capacity + TrayColumns - 1) / TrayColumns;
        var panel = new Rectangle(0, HeaderHeight, TraySlotSize.X * TrayColumns + 12, rows * TraySlotSize.Y + 12);
        batch.Draw(_pixel, panel, new Color(40, 34, 28) * 0.9f);
        // 칸마다 테두리와 조각을 그린다 (조각 없는 칸은 빈 테두리)
        for (int slot = 0; slot < tray.Capacity; slot++)
        {
            var box = new Rectangle(6 + slot % TrayColumns * TraySlotSize.X, HeaderHeight + 6 + slot / TrayColumns * TraySlotSize.Y,
                TraySlotSize.X - 4, TraySlotSize.Y - 4);
            batch.Draw(_pixel, box, new Color(70, 60, 48));
            batch.DrawString(font, $"{slot + 1}", new Vector2(box.X + 3, box.Y), Color.Wheat * 0.7f);
            if (slot >= tray.Pieces.Count)
            {
                continue;
            }
            BridgePiece piece = tray.Pieces[slot];
            // 조각 크기(칸 16×11px)를 칸 가운데에 맞춘다
            var size = new Vector2(piece.Width * FortMap.CellPixelWidth, piece.Height * FortMap.CellPixelHeight) * TrayPieceScale;
            Vector2 origin = new Vector2(box.Center.X, box.Center.Y + 6) - size / 2;
            // 회전된 칸마다 기준점(칸 오른쪽 아래)에 원본 프레임을 그린다
            foreach (PlacedBridgeCell cell in piece.Cells())
            {
                var anchor = origin + new Vector2((cell.Dx + 1) * FortMap.CellPixelWidth, (cell.Dy + 1) * FortMap.CellPixelHeight) * TrayPieceScale;
                DrawSprite(batch, _bridgeType.LoadIndex, BridgeFrames.Find(frames, cell.Cell), anchor, scale: TrayPieceScale);
            }
        }
        string held = (HeldPreview == null ? "없음" : HeldPreview.ToString()) + (_reverseRotation ? " | 반대 회전 켜짐" : "");
        string head = $"다리 조각 시험 | 칸 {tray.Pieces.Count}/{tray.Capacity} | 추첨 {tray.DrawCount}회 | 템플 {(player.HasTemple ? "있음" : "없음 — 조각이 생기지 않음")} | 들고 있는 조각: {held}";
        string keys = "1~6: 조각 집기 · R: 회전(원본 우클릭) · C: 반대 회전 · Backspace: 되돌리기 · 좌클릭: 놓기 · Space: 정지 · B: 모드 끄기";
        if (_bridgeCheck != null)
        {
            head += _bridgeCheck.Allowed ? $" | 놓을 수 있음(연결 {_bridgeCheck.Attachments})" : $" | 불가: {SessionText.Describe(_bridgeCheck.Problem)}";
        }
        batch.Draw(_pixel, new Rectangle(0, height - BridgePanelHeight, width, BridgePanelHeight), new Color(18, 24, 38));
        batch.DrawString(font, head, new Vector2(16, height - BridgePanelHeight + 4), Color.Gold);
        batch.DrawString(font, _notice, new Vector2(16, height - BridgePanelHeight + 32), Color.LightGreen);
        batch.DrawString(font, keys, new Vector2(16, height - BridgePanelHeight + 60), Color.LightGray);
    }
}
