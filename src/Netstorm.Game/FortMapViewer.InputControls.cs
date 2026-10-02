using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>원본 도움말에서 확인한 조작키를 공개 캠페인의 입력에 연결한다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>미션마다 기억하는 숫자 키 카메라 위치.</summary>
    private readonly Dictionary<Keys, Vector2> _cameraLocations = [];
    /// <summary>T 키로 켜는 게임 타이머 표시 (원본 녹화 세 개 모두 타이머가 보이지 않아 꺼진 상태로 시작한다)</summary>
    private bool _showTimer;
    /// <summary>F2 건물 그림 숨기기 (View 메뉴 "Hide buildings - F2")</summary>
    private bool _hideBuildings;
    /// <summary>F7 섬 소유 색 표시 (View 메뉴 "Hide Island Ownership - F7" 이 켜지면 false)</summary>
    private bool _islandColors = true;
    /// <summary>Shift+F3 섬 테마 숨기기 (View 메뉴 "Hide/Show Island Themes - Shift F3"). 켜면 모든 섬을 기본 초록 지면으로 그린다.</summary>
    private bool _hideIslandThemes;
    private readonly List<int> _recentPlaced = [];
    /// <summary>일반 도움말·About 창을 게임 본체에 요청한다.</summary>
    public Action<string>? HelpRequested { get; set; }

    /// <summary>F4/H 홈 템플, P/R 사제 선택, T 시계, 숫자 카메라와 Alt/가운데 버튼 스크롤.</summary>
    private void UpdateOriginalControls(double seconds, KeyboardState keyboard, MouseState mouse)
    {
        bool shift = keyboard.IsKeyDown(Keys.LeftShift) || keyboard.IsKeyDown(Keys.RightShift);
        if (Pressed(keyboard, Keys.F2)) _hideBuildings = !_hideBuildings;
        if (!shift && Pressed(keyboard, Keys.F7)) _islandColors = !_islandColors;
        // 2026-09-30 녹화 05:02: View 메뉴의 Hide/Show Island Themes 를 켜자 눈·갈색 섬이 모두 초록 지면으로 바뀌었다
        if (shift && Pressed(keyboard, Keys.F3)) _hideIslandThemes = !_hideIslandThemes;
        if (Pressed(keyboard, Keys.F4) || Pressed(keyboard, Keys.H))
        {
            GameEntity? temple = _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Temple);
            if (temple != null) _camera = WorldPixels(temple.Footprint.AnchorX, temple.Footprint.AnchorY);
            _session.Submit(new ReturnHomeCommand(TestPlayer));
        }
        if (Pressed(keyboard, Keys.P) || Pressed(keyboard, Keys.R))
        {
            CancelCursor();
            GameEntity? priest = _session.Entities.FirstOrDefault(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Priest);
            if (priest != null)
            {
                if (_session.Player(TestPlayer).SelectedEntityId == priest.Id) CenterOnPriest();
                SubmitCommand(new SelectEntityCommand(TestPlayer, priest.Id));
            }
        }
        bool control = keyboard.IsKeyDown(Keys.LeftControl) || keyboard.IsKeyDown(Keys.RightControl);
        if (Pressed(keyboard, Keys.N) || control && Pressed(keyboard, Keys.F5))
        {
            GameEntity[] transports = [.. _session.Entities.Where(e => e.Owner == TestPlayer && e.Kind == ObjectKind.Transport).OrderBy(e => e.Id)];
            if (transports.Length > 0)
            {
                int current = Array.FindIndex(transports, t => t.Id == _session.Player(TestPlayer).SelectedEntityId);
                GameEntity next = transports[(current + 1) % transports.Length];
                CancelCursor(); SubmitCommand(new SelectEntityCommand(TestPlayer, next.Id));
            }
        }
        if (Pressed(keyboard, Keys.D) && _lastProductionType != null) { CancelCursor(); ChooseProduction(_lastProductionType); }
        if (Pressed(keyboard, Keys.Tab))
        {
            int[] recent = [.. _recentPlaced.Where(id => _session.Entity(id)?.Owner == TestPlayer)];
            if (recent.Length > 0)
            {
                int current = Array.IndexOf(recent, _session.Player(TestPlayer).SelectedEntityId);
                CancelCursor(); SubmitCommand(new SelectEntityCommand(TestPlayer, recent[(current + 1) % recent.Length]));
            }
        }
        if (Pressed(keyboard, Keys.T)) _showTimer = !_showTimer;
        // 원본 팁 9: "U 키를 누르면 마지막으로 잃은 유닛 위치로 간다" (exe 는 내 오브젝트 파괴 때 위치를 저장한다)
        if (Pressed(keyboard, Keys.U) && _lastLostCell is (int lostX, int lostY)) _camera = WorldPixels(lostX, lostY);
        // Shift+숫자는 현재 카메라를 저장하고 같은 숫자는 저장한 위치로 복귀한다.
        for (Keys key = Keys.D0; key <= Keys.D9; key++)
        {
            if (!Pressed(keyboard, key)) continue;
            if (shift) _cameraLocations[key] = _camera;
            else if (_cameraLocations.TryGetValue(key, out Vector2 location)) _camera = location;
        }
        bool alt = keyboard.IsKeyDown(Keys.LeftAlt) || keyboard.IsKeyDown(Keys.RightAlt);
        if ((alt || mouse.MiddleButton == ButtonState.Pressed) && !IsPlayUiPoint(mouse.X, mouse.Y))
        {
            Vector2 offset = new(mouse.X - _playWidth / 2f, mouse.Y - (_playHeight + HeaderHeight) / 2f);
            if (offset.LengthSquared() > 16) _camera = Vector2.Clamp(_camera + Vector2.Normalize(offset) * PanSpeed * (float)seconds,
                Vector2.Zero, WorldPixelSize);
        }
    }

    /// <summary>F2는 고정 건물·생산 유닛의 그림만 숨기며 사제·수송체·가이저는 유지한다.</summary>
    private bool HiddenByBuildingToggle(TypeInfo type)
    {
        ObjectKind kind = ObjectKinds.Of(type);
        return _playUi && _hideBuildings && (ObjectKinds.IsBuilding(kind)
            || ObjectKinds.IsProducibleUnit(kind) && kind != ObjectKind.Transport);
    }
}
