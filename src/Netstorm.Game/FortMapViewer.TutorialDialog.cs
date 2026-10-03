using System.Text.RegularExpressions;
using FontStashSharp;
using Microsoft.Xna.Framework;
using Microsoft.Xna.Framework.Graphics;
using Microsoft.Xna.Framework.Input;
using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Game;

/// <summary>
/// 미션 스크립트의 튜토리얼 안내와 캠페인 초기 브리핑(섹션 A.)을 표시하는 모달 안내 창.
/// 창이 열려 있는 동안에는 세션 시간이 흐르지 않는다(원본과 같음 — docs/gameplay/dialog-pause.md).
/// </summary>
internal sealed partial class FortMapViewer
{
    /// <summary>본문의 줄 높이.</summary>
    private const int TutorialLineHeight = 16;
    /// <summary>문단 사이에 더하는 간격.</summary>
    private const int TutorialParagraphGap = 12;
    /// <summary>스크롤 키가 한 번에 이동하는 본문 높이.</summary>
    private const int TutorialPageScroll = 180;
    /// <summary>글 사이 그림과 그 뒤 글자 사이의 간격(논리 픽셀).</summary>
    private const int TutorialPictureGap = 6;
    /// <summary>단어와 공백을 분리해 인라인 강조를 유지하며 줄을 감는다.</summary>
    private static readonly Regex TutorialWords = new(@"\S+|\s+", RegexOptions.Compiled);

    private TutorialDialogScript? _tutorialDialog;
    private TutorialDialogAction? _pendingTutorialAction;
    private int _tutorialScroll;
    /// <summary>안내 창 버튼의 누름·떼기 처리기. 번호는 버튼 순번이다 (원본 돌 버튼 규칙 — <see cref="ButtonGump"/>).</summary>
    private readonly ButtonGump _tutorialGump = new();

    /// <summary>
    /// 지금 눌린 모양인 안내·확인·지식 창 버튼의 글자 (없으면 null). 자동 UI 검사가 읽는다.
    /// </summary>
    public string? PressedLabel =>
        TutorialDialogOpen && _tutorialGump.Held is int tutorial && _tutorialGump.IsPressed(tutorial)
            ? LocalizeCampaign(_tutorialDialog!.Current!).Buttons[tutorial].Label
        : _leaveMissionPrompt && _leaveGump.Held is int leave && _leaveGump.IsPressed(leave) ? LeaveMissionLabels()[leave]
        : _knowledgeDetail != null && _knowledgeGump.Held is int detail && _knowledgeGump.IsPressed(detail) ? (detail == 0 ? "Back" : "OK")
        : null;

    /// <summary>튜토리얼 안내 창이 현재 지도의 입력을 가로막는지.</summary>
    public bool TutorialDialogOpen => _tutorialDialog?.Current != null;

    /// <summary>
    /// 미션 스크립트와 설정 치환표를 안내 창에 연결한다. 튜토리얼뿐 아니라 캠페인 미션도 포함한다 —
    /// 원본은 캠페인 미션도 같은 "Tutorial" 미션 클래스라 시작하면 섹션 A.(초기 브리핑)를 Tell하고 시계를 멈춘다.
    /// 세션이 튜토리얼 단계를 직접 처리하는 미션(튜토리얼 1·2)은 세션 이벤트가 A.를 열므로 여기서 열지 않는다.
    /// 브리핑이 열려 있는 동안에는 Update 가 세션 틱을 건너뛰므로 브리핑을 닫기 전까지 게임 시간이 0 이다.
    /// 원본은 미션 시작 뒤 다이얼로그 없이 10프레임이 지나야 브리핑이 뜨지만(그 사이 시계가 흐름), 클론은 즉시 연다.
    /// </summary>
    private void InitializeTutorialDialog(MissionScript? script, ConfigStore? settings, MissionScript? commonScript = null)
    {
        if (script != null && settings != null && _mission != null)
        {
            // 공용 메뉴 스크립트(tell.english)의 {mission.title}·{mission.fileName} 이 현재 미션을 가리키게 한다
            var missionValues = new Dictionary<string, string> { ["title"] = _mission.Title ?? Name, ["fileName"] = Name };
            _tutorialDialog = new TutorialDialogScript(script, settings, commonScript, missionValues);
            if (_session.Tutorial == null && _tutorialDialog.OpenBriefing())
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
            ResetTutorialPage();
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
            // 원본 안내 창은 Esc 로 닫히지 않고 화면 버튼으로만 진행한다.
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
        // 키보드(Tab·Enter·Space 등)는 버튼에 영향을 주지 않는다 (원본에는 버튼 키보드 조작이 없었다 — 사용자 확인 2026-10-04).
        Rectangle panel = TutorialPanel(width, height);
        var buttons = new List<GumpButton>();
        // 화면 버튼마다 판정 영역을 만든다 (잠긴 버튼은 눌리지 않는다)
        for (int index = 0; index < content.Buttons.Count; index++)
            buttons.Add(OriginalUiSkin.Hit(index, TutorialButton(panel, content.Buttons.Count, index), !TutorialButtonLocked(content.Buttons[index])));
        // 누르는 순간 소리, 눌린 채 안쪽에서 뗄 때 실행
        GumpResult result = _tutorialGump.Update(buttons, mouse.X, mouse.Y, mouse.LeftButton == ButtonState.Pressed);
        if (result.Pressed != null) QueueSound(OriginalUiSkin.ButtonSound);
        if (result.Activated is int activated) ActivateTutorialButton(activated);
    }

    /// <summary>버튼의 Tell 이동은 창 안에서 끝내고 미션 이동은 게임 본체에 전달한다.</summary>
    private void ActivateTutorialButton(int index)
    {
        if (TutorialButtonLocked(_tutorialDialog!.Current!.Buttons[index])) return;
        // 클릭음은 버튼을 누르는 순간 이미 났으므로 실행(뗄 때)에서는 내지 않는다.
        TutorialDialogAction action = _tutorialDialog!.Choose(index);
        if (action.Kind == TutorialDialogActionKind.Navigate)
        {
            ResetTutorialPage();
        }
        else if (action.Kind == TutorialDialogActionKind.ShowKnowledge)
        {
            OpenKnowledge();
        }
        else if (action.Kind == TutorialDialogActionKind.Unsupported)
        {
            _notice = action.Argument;
        }
        else if (action.Kind == TutorialDialogActionKind.ConfirmLeave)
        {
            // 원본 [ABORT] 와 같은 버튼(Main Menu·Replay Mission·Continue Mission)을 가진 Leave Mission 확인 창을 연다
            _leaveMissionPrompt = true;
        }
        else if (action.Kind == TutorialDialogActionKind.RestartMission)
        {
            _pendingMissionMenuAction = MissionMenuAction.Restart;
        }
        else if (action.Kind is TutorialDialogActionKind.LeaveBattle or TutorialDialogActionKind.MissionBegin)
        {
            _pendingTutorialAction = action;
        }
    }

    /// <summary>새 섹션을 맨 위에서 보여 주고 붙잡고 있던 버튼은 놓는다 (키보드 선택 표시는 원본에 없다).</summary>
    private void ResetTutorialPage()
    {
        _tutorialScroll = 0;
        _tutorialGump.Cancel();
    }

    /// <summary>다음 미션 버튼은 표시·마우스·키보드 모두 동일한 범위 잠금을 사용한다.</summary>
    private bool TutorialButtonLocked(TutorialDialogButton button) => _playUi && button.Action.Equals("MissionBegin", StringComparison.OrdinalIgnoreCase)
        && !Netstorm.Core.Rules.CampaignAccess.IsAvailable(button.Argument);

    /// <summary>본문 길이·버튼 폭으로 원본처럼 작은 안내 창을 만들고 화면 안에 둔다.</summary>
    private Rectangle TutorialPanel(int width, int height)
    {
        TutorialDialogContent content = LocalizeCampaign(_tutorialDialog!.Current!);
        int desiredWidth = content.Section is "Succeeded" or "BadTeamDead" ? 374 : TutorialTitle(content).Length == 0 ? 300 : 350;
        int buttonsWidth = content.Buttons.Sum(b => TutorialButtonWidth(b)) + (content.Buttons.Count - 1) * 16;
        int panelWidth = Math.Min(width - 32, Math.Max(desiredWidth, Math.Max(buttonsWidth + 48,
            (int)_uiSkin.Title.MeasureString(TutorialTitle(content)).X + 60)));
        int padding = content.Section == "A." ? 48 : 30;
        int bodyHeight = LayoutTutorialText(content.Runs, _uiSkin.Body, panelWidth - padding * 2, _uiSkin.Title).Sum(l => l.Height);
        int panelHeight = Math.Min(height - 32, Math.Max(136, bodyHeight + (TutorialTitle(content).Length == 0 ? 78 : 110)));
        int centerX = width / 2 + (_playUi ? 42 : 0);
        return new Rectangle(Math.Clamp(centerX - panelWidth / 2, 16, width - panelWidth - 16), (height - panelHeight) / 2, panelWidth, panelHeight);
    }

    /// <summary>창 위쪽에 따로 그릴 제목. 제목이 본문 흐름 안에 있는 안내(그림 옆 제목)는 빈 문자열이다.</summary>
    private static string TutorialTitle(TutorialDialogContent content) => content.TitleInBody ? "" : content.Title;

    /// <summary>버튼의 번역된 문구 폭에 맞춰 원본의 낮은 버튼 너비를 계산한다.</summary>
    private int TutorialButtonWidth(TutorialDialogButton button) => Math.Max(36, (int)Math.Ceiling(_uiSkin.Body.MeasureString(button.Label).X) + 12);

    /// <summary>같은 번역·글꼴·간격으로 하단 버튼의 표시와 클릭 영역을 계산한다.</summary>
    private Rectangle TutorialButton(Rectangle panel, int count, int index)
    {
        var buttons = LocalizeCampaign(_tutorialDialog!.Current!).Buttons;
        int totalWidth = buttons.Sum(TutorialButtonWidth) + (count - 1) * 16;
        int x = panel.Center.X - totalWidth / 2;
        // 앞 버튼의 실제 폭을 더해 짧은 문구의 버튼도 가운데에 모인다.
        for (int i = 0; i < index; i++) x += TutorialButtonWidth(buttons[i]) + 16;
        return new Rectangle(x, panel.Bottom - 35, TutorialButtonWidth(buttons[index]), OriginalUiSkin.ButtonHeight);
    }

    /// <summary>지도 위에 제목·스크롤 가능한 본문·스크립트 버튼을 그린다.</summary>
    private void DrawTutorialDialog(SpriteBatch batch, SpriteFontBase font, int width, int height)
    {
        TutorialDialogContent? content = _tutorialDialog?.Current;
        if (content != null) content = LocalizeCampaign(content);
        if (content == null)
        {
            return;
        }
        Rectangle panel = TutorialPanel(width, height);
        int padding = content.Section == "A." ? 48 : 30;
        int top = TutorialTitle(content).Length == 0 ? 22 : 54;
        var body = new Rectangle(panel.X + padding, panel.Y + top, panel.Width - padding * 2, panel.Height - top - 51);
        IReadOnlyList<TutorialVisualLine> lines = LayoutTutorialText(content.Runs, font, body.Width, _uiSkin.Title);
        int contentHeight = lines.Sum(line => line.Height);
        _tutorialScroll = Math.Clamp(_tutorialScroll, 0, Math.Max(0, contentHeight - body.Height));

        _uiSkin.Panel(batch, panel);
        OriginalUiSkin.Text(batch, _uiSkin.Title, TutorialTitle(content), new Vector2(panel.X + padding, panel.Y + 20));

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
                    // 그림·큰 제목·본문 글자를 줄의 아래쪽에 맞춰 그린다 (원본은 그림 옆 제목이 그림 아래 끝에 놓인다).
                    if (span.Picture != null)
                    {
                        batch.Draw(span.Picture, new Vector2(x, y + line.Height - span.Picture.Height), Color.White);
                        x += span.Picture.Width + TutorialPictureGap;
                        continue;
                    }
                    SpriteFontBase spanFont = span.Style == TutorialTextStyle.Heading ? _uiSkin.Title : font;
                    int spanHeight = span.Style == TutorialTextStyle.Heading ? TutorialHeadingHeight(_uiSkin.Title) : TutorialLineHeight;
                    OriginalUiSkin.Text(batch, spanFont, span.Text, new Vector2(x, y + line.Height - spanHeight), TutorialColor(span.Style));
                    x += spanFont.MeasureString(span.Text).X;
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
            bool locked = TutorialButtonLocked(content.Buttons[index]);
            _uiSkin.Button(batch, button, content.Buttons[index].Label, !locked, pressed: _tutorialGump.IsPressed(index));
        }
    }

    /// <summary>HTML 강조 종류를 원본의 흰색·노란색 본문 계열로 바꾼다.</summary>
    private static Color TutorialColor(TutorialTextStyle style) => style switch
    {
        TutorialTextStyle.Heading => Color.White,
        TutorialTextStyle.Emphasis => Color.Wheat,
        TutorialTextStyle.Highlight => Color.Yellow,
        _ => Color.White,
    };

    /// <summary>본문 흐름 안의 제목 한 줄이 차지하는 높이 (제목 글꼴의 글자 높이 + 여백 2).</summary>
    private static int TutorialHeadingHeight(SpriteFontBase headingFont) =>
        Math.Max(TutorialLineHeight, (int)Math.Ceiling(headingFont.MeasureString("Ag").Y) + 2);

    /// <summary>
    /// 실제 글꼴 폭으로 구간을 이어 감아 긴 튜토리얼 본문을 화면 폭 안에 둔다.
    /// 글 사이 그림(<c>~[I타입.프레임]</c>)은 그 자리에 원본 스프라이트를 놓고 줄 높이를 그림 높이로 키운다.
    /// </summary>
    /// <param name="headingFont">본문 흐름 안의 제목에 쓸 큰 글꼴 (null 이면 제목도 본문 글꼴)</param>
    private IReadOnlyList<TutorialVisualLine> LayoutTutorialText(IReadOnlyList<TutorialTextRun> runs,
        SpriteFontBase font, int maxWidth, SpriteFontBase? headingFont = null)
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
            if (run.Style == TutorialTextStyle.Picture)
            {
                // 모르는 타입·그림 없는 프레임은 원본 표기를 화면에 드러내지 않고 건너뛴다.
                if (InlinePicture.Resolve(_knowledgeTypes, run.Text) is { } picture
                    && GetTexture(picture.Type.LoadIndex, picture.Frame) is { } sprite)
                {
                    if (currentWidth > 0 && currentWidth + sprite.Texture.Width > maxWidth) FlushLine();
                    current.Spans.Add(new TutorialVisualSpan("", run.Style, sprite.Texture));
                    current.Height = Math.Max(current.Height, sprite.Texture.Height + 2);
                    currentWidth += sprite.Texture.Width + TutorialPictureGap;
                }
                spacePending = false;
                continue;
            }
            SpriteFontBase runFont = run.Style == TutorialTextStyle.Heading && headingFont != null ? headingFont : font;
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
                if (currentWidth > 0 && currentWidth + runFont.MeasureString(piece).X > maxWidth)
                {
                    FlushLine();
                    piece = word;
                }
                if (runFont.MeasureString(piece).X > maxWidth)
                {
                    // 한 단어가 폭보다 길면 글자 단위로 나눠 항상 본문 안에 남긴다.
                    foreach (char character in word)
                    {
                        if (currentWidth > 0 && currentWidth + runFont.MeasureString(character.ToString()).X > maxWidth)
                            FlushLine();
                        AddText(character.ToString(), run.Style, runFont);
                    }
                }
                else
                {
                    AddText(piece, run.Style, runFont);
                }
                spacePending = false;
            }
        }
        FlushLine();
        return lines;

        /// <summary>현재 줄에 같은 스타일을 합쳐 글자를 붙인다. 큰 제목 글꼴이 들어오면 줄 높이를 그만큼 키운다.</summary>
        void AddText(string text, TutorialTextStyle style, SpriteFontBase textFont)
        {
            if (current.Spans.Count > 0 && current.Spans[^1].Style == style && current.Spans[^1].Picture == null)
                current.Spans[^1] = current.Spans[^1] with { Text = current.Spans[^1].Text + text };
            else
                current.Spans.Add(new TutorialVisualSpan(text, style));
            if (textFont != font) current.Height = Math.Max(current.Height, TutorialHeadingHeight(textFont));
            currentWidth += textFont.MeasureString(text).X;
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

    /// <summary>한 화면 줄의 스타일 구간. 글 사이 그림이면 Picture 에 그릴 텍스처가 들어 있고 Text 는 비어 있다.</summary>
    private sealed record TutorialVisualSpan(string Text, TutorialTextStyle Style, Texture2D? Picture = null);

    /// <summary>한 화면 줄과 그 줄이 차지하는 높이.</summary>
    private sealed class TutorialVisualLine(int height)
    {
        /// <summary>줄에서 그릴 구간.</summary>
        public List<TutorialVisualSpan> Spans { get; } = [];
        /// <summary>본문 스크롤에서 차지하는 높이. 그림이나 큰 제목이 들어오면 커진다.</summary>
        public int Height { get; set; } = height;
    }
}
