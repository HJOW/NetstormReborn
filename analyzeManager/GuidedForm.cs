using System.Diagnostics;
using System.Drawing;
using System.Drawing.Text;
using System.Text;
using System.Text.Json;

namespace Netstorm.AnalyzeManager;

/// <summary>게임 옆에 조작 순서를 보여주고 사용자가 녹화를 시작·중단할 수 있는 창.</summary>
public sealed class GuidedForm : Form
{
    /// <summary>안내 파일을 한 번에 지나치게 많이 읽지 않도록 제한한다.</summary>
    private const int StepsLimitBytes = 64_000;
    private readonly SessionStore _store;
    private readonly AnalysisSession _session;
    // DPI 배율을 적용한 뒤에도 같은 게임 창을 기준으로 안내 창을 배치하도록 프로세스 ID를 보관한다.
    private readonly int _gameProcessId;
    private readonly string _stepsPath;
    private readonly string _statePath;
    private readonly Label _heading;
    private readonly TextBox _instruction;
    private readonly Label _status;
    private readonly Button _previous;
    private readonly Button _next;
    private readonly Button _start;
    private readonly Button _stop;
    private readonly System.Windows.Forms.Timer _statusTimer;
    private readonly PrivateFontCollection? _privateFonts;
    private FileStream? _desktopLock;
    private GuidedRecorder? _recorder;
    private string[] _steps = [];
    private int _step;
    /// <summary>마지막으로 안내 창을 배치한 주 게임 창의 바깥 좌표.</summary>
    private Rectangle? _placedGameBounds;

    /// <summary>기존 관리 세션과 UTF-8 안내를 읽고 게임을 가리지 않는 위치에 창을 놓는다.</summary>
    public GuidedForm(SessionStore store, string sessionId, string? suppliedSteps)
    {
        _store = store;
        _session = store.Load(sessionId);
        using Process? process = store.OwnedProcess(_session);
        if (process == null) throw new InvalidOperationException("기존 세션의 게임이 실행 중이어야 안내 모드를 열 수 있습니다.");
        _gameProcessId = process.Id;
        string directory = store.SessionDirectory(sessionId);
        _stepsPath = Path.Combine(directory, "guide-steps.txt");
        _statePath = Path.Combine(directory, "guide-state.json");
        if (suppliedSteps != null) CopySteps(suppliedSteps);
        LoadSteps();
        if (File.Exists(_statePath))
        {
            GuideState? state = JsonSerializer.Deserialize<GuideState>(File.ReadAllText(_statePath, Encoding.UTF8), SessionStore.Json);
            _step = Math.Clamp(state?.Step ?? 0, 0, _steps.Length - 1);
        }
        Text = "NetStorm 원본 분석 안내";
        // 96 DPI를 기준으로 잡아 글꼴과 창·컨트롤이 현재 화면 배율에 맞춰 함께 커지거나 작아지게 한다.
        AutoScaleDimensions = new SizeF(96f, 96f);
        AutoScaleMode = AutoScaleMode.Dpi;
        // 안내 창의 가장자리를 끌어 크기를 바꿀 수 있도록 크기 조절 테두리를 사용한다.
        FormBorderStyle = FormBorderStyle.SizableToolWindow;
        StartPosition = FormStartPosition.Manual;
        MaximizeBox = false;
        MinimizeBox = false;
        SizeGripStyle = SizeGripStyle.Show;
        TopMost = true;
        // 설명과 버튼이 넉넉히 보이도록 기본 크기를 키우고, 작은 화면에서도 너무 작아지지 않게 한다.
        MinimumSize = new Size(500, 540);
        ClientSize = new Size(560, 620);
        string? fontFile = Directory.Exists(Path.Combine(store.Repository, "fonts"))
            ? Directory.EnumerateFiles(Path.Combine(store.Repository, "fonts"), "D2Coding*.ttc").FirstOrDefault() : null;
        if (fontFile != null)
        {
            PrivateFontCollection? loaded = null;
            try
            {
                loaded = new PrivateFontCollection();
                loaded.AddFontFile(fontFile);
            }
            catch (ArgumentException) { loaded?.Dispose(); loaded = null; }
            _privateFonts = loaded;
        }
        Font = _privateFonts?.Families.Length > 0 ? new Font(_privateFonts.Families[0], 10) : new Font("Malgun Gothic", 10);
        _heading = new Label { Dock = DockStyle.Fill, Font = new Font(Font, FontStyle.Bold), TextAlign = ContentAlignment.MiddleLeft, Margin = Padding.Empty };
        _instruction = new TextBox { Dock = DockStyle.Fill, BorderStyle = BorderStyle.FixedSingle, Font = Font,
            Multiline = true, ReadOnly = true, WordWrap = true, ScrollBars = ScrollBars.Vertical, TabStop = false, Margin = new Padding(0, 4, 0, 6) };
        _previous = new Button { Text = "이전", Dock = DockStyle.Fill, Margin = new Padding(0, 2, 4, 2) };
        _next = new Button { Text = "다음 단계", Dock = DockStyle.Fill, Margin = new Padding(4, 2, 0, 2) };
        _start = new Button { Text = "녹화 시작 / 이어서", Dock = DockStyle.Fill, Margin = new Padding(0, 2, 4, 2) };
        _stop = new Button { Text = "녹화 중단", Dock = DockStyle.Fill, Enabled = false, Margin = new Padding(4, 2, 0, 2) };
        var reload = new Button { Text = "안내 다시 읽기", Dock = DockStyle.Fill, Margin = new Padding(0, 2, 0, 2) };
        _status = new Label { Dock = DockStyle.Fill, Text = "녹화 대기", TextAlign = ContentAlignment.MiddleLeft, AutoEllipsis = true, Margin = new Padding(0, 3, 0, 0) };

        // 안내 본문은 남은 공간을 채우고, 조작 버튼과 상태 줄은 항상 창 안에 남도록 배치한다.
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 6, Padding = new Padding(12) };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 44));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 46));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 44));

        // 단계 이동 버튼은 30:70으로, 녹화 버튼은 시작 버튼을 더 넓게 나눠 창 폭에 맞춘다.
        var navigation = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Margin = Padding.Empty };
        navigation.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 30));
        navigation.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 70));
        navigation.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        navigation.Controls.Add(_previous, 0, 0);
        navigation.Controls.Add(_next, 1, 0);

        var recording = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 2, RowCount = 1, Margin = Padding.Empty };
        recording.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 64));
        recording.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 36));
        recording.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        recording.Controls.Add(_start, 0, 0);
        recording.Controls.Add(_stop, 1, 0);

        layout.Controls.Add(_heading, 0, 0);
        layout.Controls.Add(_instruction, 0, 1);
        layout.Controls.Add(navigation, 0, 2);
        layout.Controls.Add(recording, 0, 3);
        layout.Controls.Add(reload, 0, 4);
        layout.Controls.Add(_status, 0, 5);
        Controls.Add(layout);
        _previous.Click += (_, _) => ChangeStep(-1);
        _next.Click += (_, _) => ChangeStep(1);
        _start.Click += (_, _) => StartRecording();
        _stop.Click += (_, _) => StopRecording();
        reload.Click += (_, _) => { LoadSteps(); RestoreGameFocus(); };
        _statusTimer = new System.Windows.Forms.Timer { Interval = 1000 };
        _statusTimer.Tick += (_, _) => UpdateStatus();
        if (!PositionBesideGame(WindowsGame.OuterBounds(WindowsGame.FindWindow(_gameProcessId, preferForeground: false))))
            throw new InvalidOperationException("게임 옆에 안내 창을 놓을 공간이 없습니다. 더 넓은 화면이나 두 번째 모니터를 사용하세요.");
        UpdateStep();
        _desktopLock = store.LockDesktop();
        _statusTimer.Start();
    }

    /// <summary>WinForms가 DPI 배율을 반영한 뒤 실제 안내 창 크기로 게임 옆에 다시 배치한다.</summary>
    protected override void OnLoad(EventArgs e)
    {
        base.OnLoad(e);
        Rectangle bounds = WindowsGame.OuterBounds(WindowsGame.FindWindow(_gameProcessId, preferForeground: false));
        if (PositionBesideGame(bounds)) return;
        MessageBox.Show(this, "화면 배율을 적용한 안내 창을 놓을 공간이 없습니다. 안내 창 크기를 줄이거나 다른 화면을 사용하세요.",
            "안내 창 배치 불가", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        Close();
    }

    /// <summary>새 안내 텍스트를 세션 폴더에 복사해 CLI를 다시 열어도 같은 지시를 보여준다.</summary>
    private void CopySteps(string source)
    {
        SessionStore.RejectReparse(source);
        if (new FileInfo(source).Length > StepsLimitBytes) throw new ArgumentException("안내 파일은 64 KB 이하여야 합니다.");
        byte[] bytes = File.ReadAllBytes(source);
        SessionStore.WriteSmallFile(_stepsPath, bytes);
    }

    /// <summary>빈 줄을 제외한 각 줄을 하나의 조작 단계로 읽는다.</summary>
    private void LoadSteps()
    {
        if (!File.Exists(_stepsPath)) throw new ArgumentException("--steps-file로 UTF-8 안내 파일을 지정하세요.");
        if (new FileInfo(_stepsPath).Length > StepsLimitBytes) throw new ArgumentException("안내 파일은 64 KB 이하여야 합니다.");
        _steps = File.ReadAllLines(_stepsPath, new UTF8Encoding(false, true))
            .Select(line => line.Trim()).Where(line => line.Length != 0).ToArray();
        if (_steps.Length == 0) throw new ArgumentException("안내 파일에 조작 단계가 없습니다.");
        _step = Math.Clamp(_step, 0, _steps.Length - 1);
        if (_instruction != null) UpdateStep();
    }

    /// <summary>주 게임 창의 테두리까지 피해 안내 창을 오른쪽이나 왼쪽 작업 영역에 배치한다.</summary>
    private bool PositionBesideGame(Rectangle gameBounds)
    {
        Point? position = GuidePlacement.Beside(gameBounds, Size, Screen.AllScreens.Select(screen => screen.WorkingArea));
        if (position == null) return false;
        Location = position.Value;
        _placedGameBounds = gameBounds;
        return true;
    }

    /// <summary>게임 창이 바뀌거나 사용자가 안내 창을 키워 겹치게 하거나 화면 밖으로 옮기면 다시 배치한다.</summary>
    private bool KeepBesideGame(int processId)
    {
        GameWindow game = WindowsGame.FindWindow(processId, preferForeground: false);
        Rectangle bounds = WindowsGame.OuterBounds(game);
        // 사용자가 바꾼 크기를 유지하되 창 전체가 작업 영역 밖이거나 게임과 겹치면 인접 위치를 다시 찾는다.
        bool insideWorkArea = Screen.AllScreens.Any(screen => screen.WorkingArea.Contains(Bounds));
        return (_placedGameBounds == bounds && insideWorkArea && !bounds.IntersectsWith(Bounds)) || PositionBesideGame(bounds);
    }

    /// <summary>현재 지시와 단계 번호를 갱신한다.</summary>
    private void UpdateStep()
    {
        _heading.Text = $"단계 {_step + 1} / {_steps.Length}";
        _instruction.Text = _steps[_step];
        _previous.Enabled = _step > 0;
        _next.Enabled = _step < _steps.Length - 1;
    }

    /// <summary>사용자가 단계를 바꾸면 그 시각과 지시 내용을 세션 이벤트에 남긴다.</summary>
    private void ChangeStep(int direction)
    {
        int next = Math.Clamp(_step + direction, 0, _steps.Length - 1);
        if (next == _step) return;
        _step = next;
        SaveState();
        _store.Append(_session, "guided_step", new { index = _step + 1, total = _steps.Length, instruction = _steps[_step] });
        UpdateStep();
        RestoreGameFocus();
    }

    /// <summary>안내 창을 닫고 다시 열 때 같은 단계를 복구한다.</summary>
    private void SaveState() => SessionStore.WriteSmallFile(_statePath, JsonSerializer.SerializeToUtf8Bytes(new GuideState(_step), SessionStore.Json));

    /// <summary>녹화 전용 잠금을 잡은 뒤 세 기록기를 시작한다.</summary>
    private void StartRecording()
    {
        if (_recorder != null) return;
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process == null || !KeepBesideGame(process.Id))
                throw new InvalidOperationException("게임 옆에 안내 창을 놓을 공간이 없어 녹화를 시작할 수 없습니다.");
            _recorder = new GuidedRecorder(_store, _session);
            _recorder.Start();
            _store.Append(_session, "guided_step", new { index = _step + 1, total = _steps.Length, instruction = _steps[_step] });
            _start.Enabled = false;
            _stop.Enabled = true;
            RestoreGameFocus();
        }
        catch (Exception error)
        {
            _recorder?.Dispose();
            _recorder = null;
            MessageBox.Show(this, error.Message, "녹화 시작 실패", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    /// <summary>영상과 오디오 조각을 닫고 세션은 그대로 두어 이후 녹화를 재개할 수 있게 한다.</summary>
    private void StopRecording()
    {
        if (_recorder == null) return;
        Exception? error = _recorder.Error;
        _recorder.Dispose();
        _recorder = null;
        _start.Enabled = true;
        _stop.Enabled = false;
        _status.Text = error == null ? "녹화 중단됨 · 다시 시작 가능" : $"녹화 오류: {error.Message}";
        SaveState();
        RestoreGameFocus();
    }

    /// <summary>게임 창 변경에 맞춰 안내를 다시 배치하고 녹화 오류·프레임 수를 갱신한다.</summary>
    private void UpdateStatus()
    {
        if (_recorder?.Error != null)
        {
            StopRecording();
            return;
        }
        using Process? process = _store.OwnedProcess(_session);
        if (process == null)
        {
            if (_recorder != null) StopRecording();
            _status.Text = "게임 종료 · 녹화 저장됨";
            return;
        }
        try
        {
            if (!KeepBesideGame(process.Id))
            {
                if (_recorder != null) StopRecording();
                _status.Text = "게임 옆에 안내 창을 놓을 공간이 없습니다.";
                return;
            }
        }
        catch (Exception error)
        {
            _status.Text = "게임 창 위치 확인 실패: " + error.Message;
            return;
        }
        if (_recorder == null) return;
        _status.Text = $"녹화 중 · 영상 {_recorder.FrameCount}프레임";
    }

    /// <summary>버튼으로 안내를 조작한 직후 입력 초점을 게임으로 되돌린다.</summary>
    private async void RestoreGameFocus()
    {
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process != null) await WindowsGame.FocusAsync(process.Id, CancellationToken.None);
        }
        catch (Exception error) { _status.Text = "게임 포커스 복귀 실패: " + error.Message; }
    }

    /// <summary>안내 창을 닫아도 게임을 종료하지 않고 녹화 파일만 안전하게 닫는다.</summary>
    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        _statusTimer.Stop();
        try
        {
            StopRecording();
            SaveState();
        }
        finally
        {
            _desktopLock?.Dispose();
            _desktopLock = null;
        }
        base.OnFormClosing(e);
    }

    /// <summary>안내 창에서 사용한 D2Coding 개인 글꼴을 해제한다.</summary>
    protected override void Dispose(bool disposing)
    {
        if (disposing) _privateFonts?.Dispose();
        base.Dispose(disposing);
    }

    /// <summary>마지막으로 표시한 안내 단계의 세션 상태.</summary>
    private sealed record GuideState(int Step);
}
