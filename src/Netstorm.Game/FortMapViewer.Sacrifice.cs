using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Core.Rules;
using Netstorm.Core.Simulation;

namespace Netstorm.Game;

/// <summary>맵 뷰어에서 수송 사제 포획·구출·제단 의식을 조작하고 상태를 표시한다.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>D 키를 누른 뒤 빈 칸을 클릭해 수송 사제를 내려놓을지.</summary>
    private bool _dropPriestArmed;

    /// <summary>내 제단 의식이 시작되어 음악 감독에 희생 음악을 요청해야 하는지.</summary>
    private bool _mySacrificeMusicRequested;

    /// <summary>내 제단의 희생 의식 진행 상태를 배경음악 판단에 전달한다.</summary>
    public bool MySacrificeInProgress => _session.IsSacrificeInProgress(TestPlayer);

    /// <summary>의식 시작 음악 요청을 한 번 꺼내고 내부 신호를 비운다.</summary>
    public bool ConsumeMySacrificeMusicRequest()
    {
        bool requested = _mySacrificeMusicRequested;
        _mySacrificeMusicRequested = false;
        return requested;
    }

    /// <summary>선택한 수송 유닛·사제의 왼쪽 클릭을 포획·운반·희생 명령으로 바꾼다.</summary>
    private void UpdateSacrificeInput(KeyboardState keyboard, MouseState mouse)
    {
        if (_placementMode || _bridgeMode)
        {
            _dropPriestArmed = false;
            return;
        }
        GameEntity? selected = _session.Entity(_session.Player(TestPlayer).SelectedEntityId);
        bool carrying = selected is { Kind: ObjectKind.Transport, CarriedPriestId: not 0 };
        if (Pressed(keyboard, Keys.D) && carrying)
        {
            _dropPriestArmed = true;
        }
        if (!carrying)
        {
            _dropPriestArmed = false;
        }
        if (mouse.Y < HeaderHeight || mouse.LeftButton != ButtonState.Pressed
            || _previousMouse.LeftButton != ButtonState.Released || selected == null)
        {
            return;
        }

        (int x, int y) = CellAt(new Vector2(mouse.X, mouse.Y));
        GameEntity? target = _session.EntityAt(x, y);
        if (selected.Kind == ObjectKind.Transport)
        {
            if (target?.Kind == ObjectKind.Priest && selected.CarriedPriestId == 0)
            {
                SubmitCommand(new CapturePriestCommand(TestPlayer, selected.Id, target.Id));
            }
            else if (target?.Kind == ObjectKind.Altar && selected.CarriedPriestId != 0)
            {
                SubmitCommand(new DeliverPriestCommand(TestPlayer, selected.Id, target.Id));
                _dropPriestArmed = false;
            }
            else if (target == null && _dropPriestArmed && selected.CarriedPriestId != 0)
            {
                SubmitCommand(new DropPriestCommand(TestPlayer, selected.Id, x, y));
                _dropPriestArmed = false;
            }
            return;
        }
        if (selected.Kind == ObjectKind.Priest && target?.Kind == ObjectKind.Altar)
        {
            SubmitCommand(new MovePriestToAltarCommand(TestPlayer, target.Id, selected.Id));
        }
    }

    /// <summary>운반 중·제단에 묶인 사제와 진행 중인 룬 수를 월드 위에 표시한다.</summary>
    private void DrawSacrificeStatus(SpriteBatch batch, SpriteFontBase font, Vector2 center)
    {
        // 포획된 사제는 수송체 또는 제단의 논리 위치에 상태 표식을 띄운다.
        foreach (GameEntity priest in _session.Entities.Where(entity => entity.Kind == ObjectKind.Priest
                     && entity.Captivity != PriestCaptivity.Free))
        {
            Vector2 anchor = CellCenterScreen(priest.WorldX, priest.WorldY, center);
            string label = priest.Captivity == PriestCaptivity.Carried ? "운반 중" : "제단에 묶임";
            Color color = priest.Captivity == PriestCaptivity.Carried ? Color.LightSkyBlue : Color.Gold;
            batch.Draw(_pixel, new Rectangle((int)anchor.X - 25, (int)anchor.Y - 35, 50, 18), new Color(20, 24, 34) * 0.9f);
            batch.DrawString(font, label, new Vector2(anchor.X - 23, anchor.Y - 34), color);
        }
        // 제단마다 다섯 룬의 대기·진행·소멸 상태를 번호 순으로 그린다.
        foreach (AltarRitual ritual in _session.Rituals)
        {
            GameEntity? altar = _session.Entity(ritual.AltarId);
            if (altar == null)
            {
                continue;
            }
            Vector2 anchor = CellCenterScreen(altar.WorldX, altar.WorldY, center);
            for (int index = 0; index < BattleSession.SacrificeRuneCount; index++)
            {
                Color color = index < ritual.RunesBurned ? Color.DimGray
                    : index < ritual.RunesStarted ? Color.Gold : new Color(85, 90, 104);
                batch.Draw(_pixel, new Rectangle((int)anchor.X - 31 + index * 13, (int)anchor.Y - 34, 9, 9), color);
            }
            // 의식 사제가 떠나 멈춘 의식은 복귀가 필요하다는 것을 함께 보여 준다.
            string state = ritual.PerformerAway && !ritual.Completed ? " · 멈춤(사제 복귀 필요)" : "";
            batch.DrawString(font, $"의식 · 룬 {ritual.RunesBurned}/{BattleSession.SacrificeRuneCount}{state}",
                new Vector2(anchor.X - 42, anchor.Y - 22), ritual.PerformerAway ? Color.Orange : Color.Wheat);
        }
    }
}
