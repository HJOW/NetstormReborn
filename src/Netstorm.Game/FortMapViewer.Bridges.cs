using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Bridges;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>
/// 맵 뷰어의 다리 조각 시험 모드 (B): Core 의 BridgeTray 로 생산 창 다리 칸을 원본 규칙(1초마다, Bridge Slots 칸 수,
/// 5번째마다 한 칸 조각)대로 채우고, 조각을 집어 회전·배치해 본다. 원본 bridge.type 프레임으로 그린다.
/// 다리 배치 가능 판정(연결·지지)은 Bridge.cpp 분석 전이라 아직 하지 않는다 — 놓기는 표시용이다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>다리 칸 패널의 한 칸 크기(논리 픽셀)</summary>
    private static readonly Point TraySlotSize = new(112, 84);

    /// <summary>다리 칸 패널의 열 수 (원본 사이드바처럼 2열)</summary>
    private const int TrayColumns = 2;

    /// <summary>다리 칸 패널 안 조각 배율</summary>
    private const float TrayPieceScale = 1f;

    /// <summary>들고 있는 조각 미리보기 불투명도</summary>
    private const float HeldPieceAlpha = 0.7f;

    /// <summary>숫자 키 → 다리 칸 번호 (1~6)</summary>
    private static readonly Keys[] TrayKeys = [Keys.D1, Keys.D2, Keys.D3, Keys.D4, Keys.D5, Keys.D6];

    /// <summary>bridge 타입 (셰이프 블록·프레임 표)</summary>
    private TypeInfo _bridgeType = null!;

    /// <summary>플레이어 1 의 다리 조각 칸</summary>
    private BridgeTray _tray = null!;

    /// <summary>다리 조각 시험 모드가 켜졌는지</summary>
    private bool _bridgeMode;

    /// <summary>플레이어 1 에게 템플이 있는지 (없으면 조각이 생기지 않는다)</summary>
    private bool _playerHasTemple;

    /// <summary>다리 모드에서 흐른 게임 시각(초)</summary>
    private double _bridgeClock;

    /// <summary>커서에 들고 있는 조각</summary>
    private BridgePiece? _heldPiece;

    /// <summary>시험으로 놓은 다리 칸 (그리기용)</summary>
    private readonly List<(int X, int Y, BridgeCell Cell)> _laidBridges = [];

    /// <summary>다리 모드 커서 칸 (조각 왼쪽 위 칸)</summary>
    private (int X, int Y)? _bridgeCursor;

    /// <summary>다리 모드 최근 알림</summary>
    private string _bridgeNotice = "";

    /// <summary>다리 조각 칸과 bridge 타입을 준비한다.</summary>
    private void InitializeBridges(TypeCatalog catalog)
    {
        _bridgeType = catalog.Find("bridge") ?? throw new InvalidDataException("bridge 타입이 없습니다.");
        _tray = new BridgeTray(_battle.Options.BridgeSlotCount, new NetstormRandom());
        _playerHasTemple = _map.Objects.Any(o => o.Object.Owner == TestPlayer && ObjectKinds.Of(o.Object.Type) == ObjectKind.Temple);
    }

    /// <summary>
    /// 다리 조각 시험 모드를 켠 채 시작한다 (명령줄 --bridges). 지정한 초만큼 시간을 미리 흘려 칸을 채우고,
    /// hold 가 있으면 그 모양·회전의 조각을 probe 칸에 든 상태로 둔다 (스크린샷 검증용).
    /// </summary>
    /// <param name="warmupSeconds">미리 흘릴 시간(초)</param>
    /// <param name="hold">들고 있을 (모양 번호, 회전 번호)</param>
    /// <param name="probe">조각 왼쪽 위 칸 (없으면 커서)</param>
    public void StartBridges(double warmupSeconds, (int Pattern, int Rotation)? hold, (int X, int Y)? probe)
    {
        _bridgeMode = true;
        _placementMode = false;
        // 1초 간격 갱신을 흉내 내어 칸을 채운다
        for (double t = 0; t <= warmupSeconds; t += BridgeTray.RefillIntervalSeconds)
        {
            _bridgeClock = t;
            _tray.Update(_bridgeClock, _playerHasTemple);
        }
        if (hold is (int pattern, int rotation))
        {
            _heldPiece = new BridgePiece(pattern, rotation);
        }
        _probeCell = probe;
        if (probe is (int x, int y))
        {
            _camera = WorldPixels(x, y);
        }
    }

    /// <summary>다리 모드 입력 (B 모드, 1~6 조각 집기, R/Shift+R 회전, Backspace 되돌리기, 좌클릭 놓기)과 칸 채우기.</summary>
    private void UpdateBridges(double seconds, KeyboardState keyboard, MouseState mouse)
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
        _bridgeClock += seconds;
        if (_tray.Update(_bridgeClock, _playerHasTemple) is { } added)
        {
            _bridgeNotice = $"새 조각: {added.Pattern.Index}번";
        }
        // 숫자 키로 칸의 조각을 집는다 (들고 있던 조각은 칸으로 되돌린다)
        for (int i = 0; i < TrayKeys.Length; i++)
        {
            if (Pressed(keyboard, TrayKeys[i]) && i < _tray.Pieces.Count)
            {
                BridgePiece taken = _tray.Take(i);
                if (_heldPiece != null)
                {
                    _tray.Return(_heldPiece);
                }
                _heldPiece = taken;
            }
        }
        if (_heldPiece != null && Pressed(keyboard, Keys.R))
        {
            bool shift = keyboard.IsKeyDown(Keys.LeftShift) || keyboard.IsKeyDown(Keys.RightShift);
            if (shift)
            {
                _heldPiece.RotateCounterclockwise();
            }
            else
            {
                _heldPiece.RotateClockwise();
            }
        }
        if (_heldPiece != null && Pressed(keyboard, Keys.Back) && _tray.Return(_heldPiece))
        {
            _heldPiece = null;
        }
        _bridgeCursor = _probeCell ?? (mouse.Y < HeaderHeight ? null : CellAt(new Vector2(mouse.X, mouse.Y)));
        if (_heldPiece != null && _bridgeCursor is (int cx, int cy)
            && mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton == ButtonState.Released)
        {
            // 판정 없이 칸을 기록한다 (연결·지지 규칙은 Bridge.cpp 분석 후 추가)
            foreach (PlacedBridgeCell cell in _heldPiece.Cells())
            {
                _laidBridges.Add((cx + cell.Dx, cy + cell.Dy, cell.Cell));
                _bridgeOwners[(cx + cell.Dx, cy + cell.Dy)] = TestPlayer;
            }
            _bridgeNotice = $"{_heldPiece} 을(를) ({cx}, {cy})에 놓음 — 배치 판정 미구현";
            _heldPiece = null;
        }
    }

    /// <summary>놓은 다리 칸과 들고 있는 조각 미리보기를 그린다 (저장 오브젝트 뒤, 안내 영역 앞).</summary>
    private void DrawBridgeWorld(SpriteBatch batch, Vector2 center)
    {
        TypeFrameTable frames = _bridgeType.Definition.Frames;
        // 시험으로 놓은 칸을 놓은 순서대로 그린다
        foreach ((int x, int y, BridgeCell cell) in _laidBridges)
        {
            DrawSprite(batch, _bridgeType.LoadIndex, BridgeFrames.Find(frames, cell), Screen(WorldPixels(x, y), center));
        }
        if (!_bridgeMode || _heldPiece == null || _bridgeCursor is not (int cx, int cy))
        {
            return;
        }
        // 들고 있는 조각을 커서 칸 기준으로 반투명하게 그린다
        foreach (PlacedBridgeCell placed in _heldPiece.Cells())
        {
            DrawSprite(batch, _bridgeType.LoadIndex, BridgeFrames.Find(frames, placed.Cell),
                Screen(WorldPixels(cx + placed.Dx, cy + placed.Dy), center), alpha: HeldPieceAlpha);
        }
    }

    /// <summary>다리 칸 패널(왼쪽)과 다리 모드 안내 문구를 그린다.</summary>
    private void DrawBridgeOverlay(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (!_bridgeMode)
        {
            return;
        }
        TypeFrameTable frames = _bridgeType.Definition.Frames;
        int rows = (_tray.Capacity + TrayColumns - 1) / TrayColumns;
        var panel = new Rectangle(0, HeaderHeight, TraySlotSize.X * TrayColumns + 12, rows * TraySlotSize.Y + 12);
        batch.Draw(_pixel, panel, new Color(40, 34, 28) * 0.9f);
        // 칸마다 테두리와 조각을 그린다 (조각 없는 칸은 빈 테두리)
        for (int slot = 0; slot < _tray.Capacity; slot++)
        {
            var box = new Rectangle(6 + slot % TrayColumns * TraySlotSize.X, HeaderHeight + 6 + slot / TrayColumns * TraySlotSize.Y,
                TraySlotSize.X - 4, TraySlotSize.Y - 4);
            batch.Draw(_pixel, box, new Color(70, 60, 48));
            batch.DrawString(font, $"{slot + 1}", new Vector2(box.X + 3, box.Y), Color.Wheat * 0.7f);
            if (slot >= _tray.Pieces.Count)
            {
                continue;
            }
            BridgePiece piece = _tray.Pieces[slot];
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
        string held = _heldPiece == null ? "없음" : _heldPiece.ToString();
        string head = $"다리 조각 시험 | 칸 {_tray.Pieces.Count}/{_tray.Capacity} | 추첨 {_tray.DrawCount}회 | 템플 {(_playerHasTemple ? "있음" : "없음 — 조각이 생기지 않음")} | 들고 있는 조각: {held}";
        string keys = "1~6: 조각 집기 · R/Shift+R: 시계/반시계 회전 · Backspace: 되돌리기 · 좌클릭: 놓기(판정 없음) · B: 모드 끄기";
        batch.Draw(_pixel, new Rectangle(0, height - 94, width, 94), new Color(18, 24, 38));
        batch.DrawString(font, head, new Vector2(16, height - 90), Color.Gold);
        batch.DrawString(font, _bridgeNotice, new Vector2(16, height - 62), Color.LightGreen);
        batch.DrawString(font, keys, new Vector2(16, height - 34), Color.LightGray);
    }
}
