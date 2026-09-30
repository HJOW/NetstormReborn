using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>
/// 지식 창("View Netstorm Knowledge" F6, 브리핑의 Review Knowledge 버튼).
/// 원본 ShowTechnology 명령(exe 0x492b60)은 인자를 받지 않고, 내 플레이어가 아는 유닛 타입을 원소별로 모아 보여 준다.
/// 원본 창의 그림·정확한 배치는 캡처가 없어 확인하지 못했으므로 클론은 원소별 이름 목록만 그린다.
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>지식 창의 줄 높이.</summary>
    private const int KnowledgeLineHeight = 26;

    private bool _knowledgeOpen;

    /// <summary>지식 창이 지도 입력을 가로막는지.</summary>
    public bool KnowledgeOpen => _knowledgeOpen;

    /// <summary>지식 창을 연다. 안내 창이 열려 있으면 그 위에 겹쳐 그려지며 닫으면 안내 창으로 돌아간다.</summary>
    private void OpenKnowledge() => _knowledgeOpen = true;

    /// <summary>원소 순서대로 내 플레이어가 아는 유닛 이름을 모은다 (배치 시험 후보 정렬을 재사용).</summary>
    private IReadOnlyList<(Element Element, IReadOnlyList<string> Names)> KnownUnitsByElement()
    {
        IReadOnlyCollection<string> known = _session.Player(TestPlayer).Deck.Knowledge;
        var result = new List<(Element, IReadOnlyList<string>)>();
        // 원소 네 가지를 차례로 훑어 아는 유닛이 있는 원소만 모은다.
        foreach (Element element in Enum.GetValues<Element>())
        {
            string[] names = [.. _candidates
                .Where(t => ProducibleUnit.FromType(t) is { } unit && unit.Element == element && known.Contains(t.Name))
                .Select(t => t.Name)];
            if (names.Length > 0)
            {
                result.Add((element, names));
            }
        }
        return result;
    }

    /// <summary>지식 창의 화면 위치와 크기.</summary>
    private static Rectangle KnowledgePanel(int width, int height)
    {
        int panelWidth = Math.Min(620, width - 32);
        int panelHeight = Math.Min(520, height - 32);
        return new Rectangle((width - panelWidth) / 2, (height - panelHeight) / 2, panelWidth, panelHeight);
    }

    /// <summary>닫기 버튼의 클릭 영역.</summary>
    private static Rectangle KnowledgeOkButton(Rectangle panel) => new(panel.Center.X - 60, panel.Bottom - 50, 120, 34);

    /// <summary>지식 창이 열린 동안 닫기 입력만 처리한다. 세션 시간은 안내 창이 없을 때에만 흐른다.</summary>
    private void UpdateKnowledge(KeyboardState keyboard, MouseState mouse, int width, int height, double seconds)
    {
        bool close = Pressed(keyboard, Keys.Escape) || Pressed(keyboard, Keys.Enter) || Pressed(keyboard, Keys.Space)
            || Pressed(keyboard, Keys.F6);
        if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton != ButtonState.Pressed
            && KnowledgeOkButton(KnowledgePanel(width, height)).Contains(mouse.X, mouse.Y))
        {
            close = true;
        }
        if (close)
        {
            _knowledgeOpen = false;
        }
        if (!TutorialDialogOpen)
        {
            // 게임 코드가 직접 여는 창은 시계를 멈추지 않는다 (docs/gameplay/dialog-pause.md).
            UpdateSession(seconds, new KeyboardState(), mouse);
        }
    }

    /// <summary>원소별 유닛 이름을 줄바꿈해 창에 그린다.</summary>
    private void DrawKnowledge(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        if (!_knowledgeOpen)
        {
            return;
        }
        Rectangle panel = KnowledgePanel(width, height);
        batch.Draw(_pixel, new Rectangle(0, 0, width, height), Color.Black * 0.5f);
        batch.Draw(_pixel, panel, new Color(23, 33, 53));
        Outline(batch, panel, Color.Wheat);
        batch.DrawString(font, "Netstorm Knowledge", new Vector2(panel.X + 24, panel.Y + 12), Color.Gold);
        int y = panel.Y + 56;
        int maxWidth = panel.Width - 48;
        var groups = KnownUnitsByElement();
        if (groups.Count == 0)
        {
            batch.DrawString(font, "아직 아는 유닛이 없다", new Vector2(panel.X + 24, y), Color.LightGray);
        }
        // 원소마다 제목 줄과 이름 목록(폭에 맞춰 줄바꿈)을 그린다.
        foreach ((Element element, IReadOnlyList<string> names) in groups)
        {
            batch.DrawString(font, element.ToString(), new Vector2(panel.X + 24, y), Color.Gold);
            y += KnowledgeLineHeight;
            string line = "";
            // 이름을 이어 붙이다 폭을 넘으면 줄을 바꾼다.
            foreach (string name in names)
            {
                string next = line.Length == 0 ? name : line + ", " + name;
                if (line.Length > 0 && font.MeasureString(next).X > maxWidth - 16)
                {
                    batch.DrawString(font, line + ",", new Vector2(panel.X + 40, y), Color.White);
                    y += KnowledgeLineHeight;
                    next = name;
                }
                line = next;
            }
            batch.DrawString(font, line, new Vector2(panel.X + 40, y), Color.White);
            y += KnowledgeLineHeight + 8;
        }
        Rectangle ok = KnowledgeOkButton(panel);
        batch.Draw(_pixel, ok, new Color(95, 78, 45));
        Outline(batch, ok, Color.Gold);
        Vector2 size = font.MeasureString("OK");
        batch.DrawString(font, "OK", new Vector2(ok.Center.X - size.X / 2, ok.Y + 4), Color.White);
    }
}
