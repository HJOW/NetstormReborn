using System.Text.RegularExpressions;
using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;

namespace Netstorm.Game;

/// <summary>미션 스크립트의 튜토리얼 섹션을 표시하는 모달 안내 창.</summary>
internal sealed partial class FortMapViewer
{
    /// <summary>본문의 줄 높이.</summary>
    private const int TutorialLineHeight = 28;
    /// <summary>문단 사이에 더하는 간격.</summary>
    private const int TutorialParagraphGap = 13;
    /// <summary>스크롤 키가 한 번에 이동하는 본문 높이.</summary>
    private const int TutorialPageScroll = 180;
    /// <summary>단어와 공백을 분리해 인라인 강조를 유지하며 줄을 감는다.</summary>
    private static readonly Regex TutorialWords = new(@"\S+|\s+", RegexOptions.Compiled);

    private TutorialDialogScript? _tutorialDialog;
    private TutorialDialogAction? _pendingTutorialAction;
    private int _tutorialScroll;
    private int _selectedTutorialButton;

    /// <summary>튜토리얼 안내 창이 현재 지도의 입력을 가로막는지.</summary>
    public bool TutorialDialogOpen => _tutorialDialog?.Current != null;

    /// <summary>미션 스크립트와 설정 치환표를 안내 창에 연결한다.</summary>
    private void InitializeTutorialDialog(MissionScript? script, ConfigStore? settings)
    {
        if (script != null && settings != null && _mission?.TutorialNumber is > 0)
        {
            _tutorialDialog = new TutorialDialogScript(script, settings);
            if (_session.Tutorial == null && _tutorialDialog.OpenStage("A."))
            {
                ResetTutorialPage();
            }
        }
    }

    /// <summary>세션의 단계 안내를 열되 보정 안내는 F8 복귀 지점으로 기록하지 않는다.</summary>
    private void OpenTutorialTell(string section)
    {
        if (_tutorialDialog == null)
        {
            return;
        }
        bool stage = section.Length == 2 && char.IsAsciiLetter(section[0]) && section[1] == '.';
        bool opened = stage ? _tutorialDialog.OpenStage(section) : _tutorialDialog.OpenSection(section);
        if (opened)
        {
            _tutorialScroll = 0;
            _selectedTutorialButton = _tutorialDialog.Current!.Buttons.Count - 1;
        }
    }

    /// <summary>창 바깥에서 처리해야 하는 미션 이동 또는 종료 요청을 한 번만 돌려준다.</summary>
    public TutorialDialogAction? TakeTutorialAction()
    {
        TutorialDialogAction? action = _pendingTutorialAction;
        _pendingTutorialAction = null;
        return action;
    }

    /// <summary>모달 안내 창이 열린 동안 클릭·키보드·휠만 처리하고 지도 명령은 받지 않는다.</summary>
    private void UpdateTutorialDialog(KeyboardState keyboard, MouseState mouse, int width, int height)
    {
        TutorialDialogContent content = _tutorialDialog!.Current!;
        if (Pressed(keyboard, Keys.Escape))
        {
            _tutorialDialog.Close();
            return;
        }
        if (Pressed(keyboard, Keys.F8))
        {
            if (_tutorialDialog.Review()) ResetTutorialPage();
            return;
        }
        int wheel = mouse.ScrollWheelValue - _previousMouse.ScrollWheelValue;
        if (wheel != 0)
        {
            _tutorialScroll = Math.Max(0, _tutorialScroll - Math.Sign(wheel) * TutorialLineHeight * 3);
        }
        if (Pressed(keyboard, Keys.PageDown)) _tutorialScroll += TutorialPageScroll;
        if (Pressed(keyboard, Keys.PageUp)) _tutorialScroll = Math.Max(0, _tutorialScroll - TutorialPageScroll);
        if (Pressed(keyboard, Keys.Down)) _tutorialScroll += TutorialLineHeight;
        if (Pressed(keyboard, Keys.Up)) _tutorialScroll = Math.Max(0, _tutorialScroll - TutorialLineHeight);
        if (Pressed(keyboard, Keys.Tab) || Pressed(keyboard, Keys.Right))
            _selectedTutorialButton = (_selectedTutorialButton + 1) % content.Buttons.Count;
        if (Pressed(keyboard, Keys.Left))
            _selectedTutorialButton = (_selectedTutorialButton + content.Buttons.Count - 1) % content.Buttons.Count;
        if (Pressed(keyboard, Keys.Enter) || Pressed(keyboard, Keys.Space))
        {
            ActivateTutorialButton(_selectedTutorialButton);
            return;
        }
        if (mouse.LeftButton == ButtonState.Pressed && _previousMouse.LeftButton != ButtonState.Pressed)
        {
            Rectangle panel = TutorialPanel(width, height);
            // 화면 버튼 중 커서가 닿은 하나만 실행한다.
            for (int index = 0; index < content.Buttons.Count; index++)
            {
                if (TutorialButton(panel, content.Buttons.Count, index).Contains(mouse.X, mouse.Y))
                {
                    ActivateTutorialButton(index);
                    return;
                }
            }
        }
    }

    /// <summary>버튼의 Tell 이동은 창 안에서 끝내고 미션 이동은 게임 본체에 전달한다.</summary>
    private void ActivateTutorialButton(int index)
    {
        TutorialDialogAction action = _tutorialDialog!.Choose(index);
        if (action.Kind == TutorialDialogActionKind.Navigate)
        {
            ResetTutorialPage();
        }
        else if (action.Kind == TutorialDialogActionKind.Unsupported)
        {
            _notice = action.Argument;
        }
        else if (action.Kind is TutorialDialogActionKind.LeaveBattle or TutorialDialogActionKind.MissionBegin)
        {
            _pendingTutorialAction = action;
        }
    }

    /// <summary>새 섹션을 맨 위에서 보여 주고 기본 선택을 마지막 버튼에 둔다.</summary>
    private void ResetTutorialPage()
    {
        _tutorialScroll = 0;
        _selectedTutorialButton = _tutorialDialog!.Current!.Buttons.Count - 1;
    }

    /// <summary>화면 가운데에 들어가는 안내 창의 최대 크기와 작은 창 여백을 정한다.</summary>
    private static Rectangle TutorialPanel(int width, int height)
    {
        int panelWidth = Math.Min(800, width - 32);
        int panelHeight = Math.Min(640, height - 32);
        return new Rectangle((width - panelWidth) / 2, (height - panelHeight) / 2, panelWidth, panelHeight);
    }

    /// <summary>버튼 개수와 번호로 하단의 클릭 영역을 계산한다.</summary>
    private static Rectangle TutorialButton(Rectangle panel, int count, int index)
    {
        int buttonWidth = Math.Min(180, Math.Max(60, (panel.Width - 48 - (count - 1) * 10) / count));
        int totalWidth = count * buttonWidth + (count - 1) * 10;
        return new Rectangle(panel.Center.X - totalWidth / 2 + index * (buttonWidth + 10), panel.Bottom - 65,
            buttonWidth, 34);
    }

    /// <summary>지도 위에 제목·스크롤 가능한 본문·스크립트 버튼을 그린다.</summary>
    private void DrawTutorialDialog(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        TutorialDialogContent? content = _tutorialDialog?.Current;
        if (content == null)
        {
            return;
        }
        Rectangle panel = TutorialPanel(width, height);
        var body = new Rectangle(panel.X + 24, panel.Y + 62, panel.Width - 48, panel.Height - 143);
        IReadOnlyList<TutorialVisualLine> lines = LayoutTutorialText(content.Runs, font, body.Width);
        int contentHeight = lines.Sum(line => line.Height);
        _tutorialScroll = Math.Clamp(_tutorialScroll, 0, Math.Max(0, contentHeight - body.Height));

        batch.Draw(_pixel, new Rectangle(0, 0, width, height), Color.Black * 0.72f);
        batch.Draw(_pixel, panel, new Color(23, 33, 53));
        Outline(batch, panel, Color.Wheat);
        batch.Draw(_pixel, new Rectangle(panel.X + 1, panel.Y + 1, panel.Width - 2, 48), new Color(40, 54, 77));
        batch.DrawString(font, content.Title, new Vector2(panel.X + 24, panel.Y + 12), Color.Gold);
        batch.Draw(_pixel, new Rectangle(body.X - 5, body.Y - 4, body.Width + 10, body.Height + 8), new Color(18, 26, 42));

        int y = body.Y - _tutorialScroll;
        // 완전히 들어오는 줄만 그려 별도의 GPU 가위 영역 없이 본문 경계를 지킨다.
        foreach (TutorialVisualLine line in lines)
        {
            if (line.Spans.Count > 0 && y >= body.Y && y + line.Height <= body.Bottom)
            {
                float x = body.X;
                // 한 줄의 강조 구간을 이어 그린다.
                foreach (TutorialVisualSpan span in line.Spans)
                {
                    batch.DrawString(font, span.Text, new Vector2(x, y), TutorialColor(span.Style));
                    x += font.MeasureString(span.Text).X;
                }
            }
            y += line.Height;
        }
        if (contentHeight > body.Height)
        {
            int barHeight = Math.Max(16, body.Height * body.Height / contentHeight);
            int barY = body.Y + _tutorialScroll * (body.Height - barHeight) / (contentHeight - body.Height);
            batch.Draw(_pixel, new Rectangle(body.Right + 1, barY, 3, barHeight), Color.Gold);
        }
        // 원본 $Button 순서대로 클릭 영역과 현재 키보드 선택을 보여 준다.
        for (int index = 0; index < content.Buttons.Count; index++)
        {
            Rectangle button = TutorialButton(panel, content.Buttons.Count, index);
            bool selected = index == _selectedTutorialButton;
            batch.Draw(_pixel, button, selected ? new Color(95, 78, 45) : new Color(54, 67, 87));
            Outline(batch, button, selected ? Color.Gold : Color.Gray);
            string label = content.Buttons[index].Label;
            Vector2 size = font.MeasureString(label);
            batch.DrawString(font, label, new Vector2(button.Center.X - size.X / 2, button.Y + 4), Color.White);
        }
        batch.DrawString(font, "F8 다시 보기 · Esc 닫기 · ↑↓/휠 스크롤", new Vector2(panel.X + 24, panel.Bottom - 26), Color.LightGray);
    }

    /// <summary>HTML 강조 종류를 개발용 대화상자의 읽기 쉬운 색으로 바꾼다.</summary>
    private static Color TutorialColor(TutorialTextStyle style) => style switch
    {
        TutorialTextStyle.Heading => Color.Gold,
        TutorialTextStyle.Emphasis => Color.Wheat,
        TutorialTextStyle.Highlight => Color.LightSkyBlue,
        _ => Color.White,
    };

    /// <summary>실제 글꼴 폭으로 구간을 이어 감아 긴 튜토리얼 본문을 화면 폭 안에 둔다.</summary>
    private static IReadOnlyList<TutorialVisualLine> LayoutTutorialText(IReadOnlyList<TutorialTextRun> runs,
        SpriteFontBase font, int maxWidth)
    {
        var lines = new List<TutorialVisualLine>();
        var current = new TutorialVisualLine(TutorialLineHeight);
        float currentWidth = 0;
        bool spacePending = false;
        // 문단·줄바꿈을 먼저 처리하고 텍스트와 공백을 순서대로 배치한다.
        foreach (TutorialTextRun run in runs)
        {
            if (run.BreakBefore != TutorialTextBreak.None)
            {
                FlushLine();
                if (run.BreakBefore == TutorialTextBreak.Paragraph && lines.Count > 0)
                    lines.Add(new TutorialVisualLine(TutorialParagraphGap));
            }
            // 스타일이 달라도 같은 줄에서 읽히도록 단어마다 폭을 측정한다.
            foreach (Match token in TutorialWords.Matches(run.Text))
            {
                if (string.IsNullOrWhiteSpace(token.Value))
                {
                    spacePending = true;
                    continue;
                }
                string word = token.Value;
                string piece = currentWidth > 0 && spacePending ? " " + word : word;
                if (currentWidth > 0 && currentWidth + font.MeasureString(piece).X > maxWidth)
                {
                    FlushLine();
                    piece = word;
                }
                if (font.MeasureString(piece).X > maxWidth)
                {
                    // 한 단어가 폭보다 길면 글자 단위로 나눠 항상 본문 안에 남긴다.
                    foreach (char character in word)
                    {
                        if (currentWidth > 0 && currentWidth + font.MeasureString(character.ToString()).X > maxWidth)
                            FlushLine();
                        AddText(character.ToString(), run.Style);
                    }
                }
                else
                {
                    AddText(piece, run.Style);
                }
                spacePending = false;
            }
        }
        FlushLine();
        return lines;

        /// <summary>현재 줄에 같은 스타일을 합쳐 글자를 붙인다.</summary>
        void AddText(string text, TutorialTextStyle style)
        {
            if (current.Spans.Count > 0 && current.Spans[^1].Style == style)
                current.Spans[^1] = current.Spans[^1] with { Text = current.Spans[^1].Text + text };
            else
                current.Spans.Add(new TutorialVisualSpan(text, style));
            currentWidth += font.MeasureString(text).X;
        }

        /// <summary>완성한 줄을 보관하고 다음 줄에서 이어 쓴다.</summary>
        void FlushLine()
        {
            if (current.Spans.Count > 0) lines.Add(current);
            current = new TutorialVisualLine(TutorialLineHeight);
            currentWidth = 0;
            spacePending = false;
        }
    }

    /// <summary>한 화면 줄의 스타일 구간.</summary>
    private sealed record TutorialVisualSpan(string Text, TutorialTextStyle Style);

    /// <summary>한 화면 줄과 그 줄이 차지하는 높이.</summary>
    private sealed class TutorialVisualLine(int height)
    {
        /// <summary>줄에서 그릴 구간.</summary>
        public List<TutorialVisualSpan> Spans { get; } = [];
        /// <summary>본문 스크롤에서 차지하는 높이.</summary>
        public int Height { get; } = height;
    }
}
