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
    private readonly bool _freePlay;
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
    private bool _recordingStopped;
    private bool _closing;
    private string[] _steps = [];
    private int _step;
    /// <summary>마지막으로 안내 창을 배치한 주 게임 창의 바깥 좌표.</summary>
    private Rectangle? _placedGameBounds;

    /// <summary>기존 관리 세션의 단계 안내 또는 자유 플레이 녹화 창을 게임 옆에 연다.</summary>
    public GuidedForm(SessionStore store, string sessionId, string? suppliedSteps, bool freePlay = false)
    {
        _store = store;
        _session = store.Load(sessionId);
        _freePlay = freePlay;
        using Process? process = store.OwnedProcess(_session);
        if (process == null) throw new InvalidOperationException("기존 세션의 게임이 실행 중이어야 안내 모드를 열 수 있습니다.");
        _gameProcessId = process.Id;
        string directory = store.SessionDirectory(sessionId);
        _stepsPath = Path.Combine(directory, "guide-steps.txt");
        _statePath = Path.Combine(directory, "guide-state.json");
        if (!freePlay)
        {
            if (suppliedSteps != null) CopySteps(suppliedSteps);
            LoadSteps();
            if (File.Exists(_statePath))
            {
                GuideState? state = JsonSerializer.Deserialize<GuideState>(File.ReadAllText(_statePath, Encoding.UTF8), SessionStore.Json);
                _step = Math.Clamp(state?.Step ?? 0, 0, _steps.Length - 1);
            }
        }
        Text = freePlay ? "NetStorm 기존 게임 플레이 녹화 분석" : "NetStorm 원본 분석 안내";
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
        MinimumSize = freePlay ? new Size(500, 340) : new Size(500, 540);
        ClientSize = freePlay ? new Size(560, 390) : new Size(560, 620);
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
        _heading = new Label { Dock = DockStyle.Fill, Font = new Font(Font, FontStyle.Bold), TextAlign = ContentAlignment.MiddleLeft,
            Text = freePlay ? "기존 게임 플레이 녹화 분석" : "", Margin = Padding.Empty };
        _instruction = new TextBox { Dock = DockStyle.Fill, BorderStyle = BorderStyle.FixedSingle, Font = Font,
            Multiline = true, ReadOnly = true, WordWrap = true, ScrollBars = ScrollBars.Vertical, TabStop = false, Margin = new Padding(0, 4, 0, 6),
            Text = freePlay ? "녹화 시작을 누른 뒤 원본 게임을 자유롭게 플레이하세요. 영상·소리는 playingVideos의 세션 폴더에 저장됩니다. 키·마우스 조작 시각과 좌표도 input-번호.jsonl 파일에 따로 기록됩니다. 녹화가 오류로 중단되면 아래 상태가 바뀌며 녹화 시작을 다시 누를 수 있습니다." : "" };
        _previous = new Button { Text = "이전", Dock = DockStyle.Fill, Margin = new Padding(0, 2, 4, 2) };
        _next = new Button { Text = "다음 단계", Dock = DockStyle.Fill, Margin = new Padding(4, 2, 0, 2) };
        _start = new Button { Text = freePlay ? "녹화 시작 / 다시 시작" : "녹화 시작 / 이어서", Dock = DockStyle.Fill, Margin = new Padding(0, 2, 4, 2) };
        _stop = new Button { Text = "녹화 중단", Dock = DockStyle.Fill, Enabled = false, Margin = new Padding(4, 2, 0, 2) };
        var reload = new Button { Text = "안내 다시 읽기", Dock = DockStyle.Fill, Margin = new Padding(0, 2, 0, 2) };
        _status = new Label { Dock = DockStyle.Fill, Text = freePlay ? "녹화 대기\n녹화 시작을 누르세요." : "녹화 대기", TextAlign = ContentAlignment.MiddleLeft,
            Font = freePlay ? new Font(Font, FontStyle.Bold) : Font, AutoEllipsis = true,
            BackColor = freePlay ? Color.LightYellow : SystemColors.Control, Padding = freePlay ? new Padding(8) : Padding.Empty,
            Margin = new Padding(0, 3, 0, 0) };

        // 안내 본문은 남은 공간을 채우고, 조작 버튼과 상태 줄은 항상 창 안에 남도록 배치한다.
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = freePlay ? 4 : 6, Padding = new Padding(12) };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        if (!freePlay) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 44));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, freePlay ? 56 : 46));
        if (!freePlay) layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 40));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, freePlay ? 88 : 44));

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
        if (freePlay)
        {
            layout.Controls.Add(recording, 0, 2);
            layout.Controls.Add(_status, 0, 3);
        }
        else
        {
            layout.Controls.Add(navigation, 0, 2);
            layout.Controls.Add(recording, 0, 3);
            layout.Controls.Add(reload, 0, 4);
            layout.Controls.Add(_status, 0, 5);
        }
        Controls.Add(layout);
        if (!freePlay)
        {
            _previous.Click += (_, _) => ChangeStep(-1);
            _next.Click += (_, _) => ChangeStep(1);
            reload.Click += (_, _) => { LoadSteps(); RestoreGameFocus(); };
        }
        _start.Click += (_, _) => StartRecording();
        _stop.Click += (_, _) => StopRecording();
        _statusTimer = new System.Windows.Forms.Timer { Interval = 1000 };
        _statusTimer.Tick += (_, _) => UpdateStatus();
        if (!PositionBesideGame(WindowsGame.OuterBounds(WindowsGame.FindWindow(_gameProcessId, preferForeground: false))))
            throw new InvalidOperationException("게임 옆에 안내 창을 놓을 공간이 없습니다. 더 넓은 화면이나 두 번째 모니터를 사용하세요.");
        if (!freePlay) UpdateStep();
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

    /// <summary>자유 플레이 창에서 녹화 여부와 재시작 안내를 색과 문장으로 함께 표시한다.</summary>
    private void SetFreeplayStatus(string state, string detail, Color background)
    {
        _status.Text = state + "\n" + detail;
        _status.BackColor = background;
    }

    /// <summary>게임 창 위치를 확인하고 영상·음성·입력 기록을 시작한다.</summary>
    private void StartRecording()
    {
        if (_recorder != null) return;
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process == null) throw new InvalidOperationException("게임이 종료되었습니다. 새 게임 세션이 필요합니다.");
            if (!KeepBesideGame(process.Id))
                throw new InvalidOperationException("게임 옆에 안내 창을 놓을 공간이 없어 녹화를 시작할 수 없습니다.");
            _recorder = new GuidedRecorder(_store, _session, _freePlay);
            _recorder.Start();
            if (!_freePlay)
                _store.Append(_session, "guided_step", new { index = _step + 1, total = _steps.Length, instruction = _steps[_step] });
            _recordingStopped = false;
            _start.Enabled = false;
            _stop.Enabled = true;
            if (_freePlay) SetFreeplayStatus("● 녹화 중", "게임 화면을 기록하고 있습니다. 녹화 중단을 누르면 파일을 닫습니다.", Color.LightGreen);
            RestoreGameFocus();
        }
        catch (Exception error)
        {
            Exception? cleanupError = null;
            try { _recorder?.Dispose(); }
            catch (Exception failure) { cleanupError = failure; }
            _recorder = null;
            _recordingStopped = true;
            _start.Enabled = true;
            _stop.Enabled = false;
            string message = error.Message + (cleanupError == null ? "" : " 파일 정리 오류: " + cleanupError.Message);
            if (_freePlay) SetFreeplayStatus("■ 녹화 시작 실패", message + " 다시 녹화 시작을 누를 수 있습니다.", Color.LightSalmon);
            MessageBox.Show(this, message, "녹화 시작 실패", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    /// <summary>영상과 오디오 조각을 닫고 세션은 그대로 두어 이후 녹화를 재개할 수 있게 한다.</summary>
    private void StopRecording(string? reason = null)
    {
        if (_recorder == null) return;
        GuidedRecorder recorder = _recorder;
        Exception? closeError = null;
        try { recorder.Dispose(); }
        catch (Exception error) { closeError = error; }
        Exception? recordingError = recorder.Error ?? closeError;
        if (_freePlay && recorder.FrameCount == 0 && recordingError == null)
            recordingError = new InvalidOperationException("영상 프레임이 0개입니다. 게임 창이 보이는지 확인하세요.");
        _recorder = null;
        _recordingStopped = true;
        _start.Enabled = true;
        _stop.Enabled = false;
        if (_freePlay)
        {
            try { FreeplayRecordingIndex.Write(_store, _session.Id); }
            catch (Exception error) { recordingError ??= error; }
            string detail = recordingError == null ? reason ?? "사용자가 녹화를 중지했습니다." : "오류: " + recordingError.Message;
            SetFreeplayStatus("■ 녹화 중단됨 · 다시 시작 가능", detail + " 녹화 시작을 누르면 새 파일에 이어 기록합니다.",
                recordingError == null && reason == null ? Color.LightYellow : Color.LightSalmon);
        }
        else
        {
            _status.Text = recordingError == null ? "녹화 중단됨 · 다시 시작 가능" : $"녹화 중단됨 · 오류: {recordingError.Message}";
            SaveState();
        }
        if (!_closing) RestoreGameFocus();
    }

    /// <summary>게임 창 변경에 맞춰 안내를 다시 배치하고 녹화 오류·프레임 수를 갱신한다.</summary>
    private void UpdateStatus()
    {
        if (_recorder?.Error != null)
        {
            StopRecording("녹화 오류가 발생했습니다.");
            return;
        }
        using Process? process = _store.OwnedProcess(_session);
        if (process == null)
        {
            if (_recorder != null) StopRecording("게임이 종료되었습니다.");
            _start.Enabled = false;
            if (_freePlay) SetFreeplayStatus("■ 게임 종료 · 녹화 저장됨", "새 녹화에는 새 게임 세션이 필요합니다.", Color.LightYellow);
            else _status.Text = "게임 종료 · 녹화 저장됨";
            return;
        }
        try
        {
            if (!KeepBesideGame(process.Id))
            {
                if (_recorder != null) StopRecording("게임 옆에 안내 창을 놓을 공간이 없습니다.");
                else if (_freePlay && !_recordingStopped)
                    SetFreeplayStatus("● 녹화 대기", "게임 옆에 안내 창을 놓을 공간이 없습니다.", Color.LightYellow);
                else if (!_freePlay) _status.Text = "게임 옆에 안내 창을 놓을 공간이 없습니다.";
                return;
            }
        }
        catch (Exception error)
        {
            if (_recorder != null) StopRecording("게임 창 위치 확인 실패: " + error.Message);
            else if (_freePlay && !_recordingStopped)
                SetFreeplayStatus("● 녹화 대기", "게임 창 위치 확인 실패: " + error.Message, Color.LightYellow);
            else if (!_freePlay) _status.Text = "게임 창 위치 확인 실패: " + error.Message;
            return;
        }
        if (_recorder == null)
        {
            if (_freePlay && !_recordingStopped) SetFreeplayStatus("● 녹화 대기", "녹화 시작을 누르세요.", Color.LightYellow);
            return;
        }
        if (_freePlay)
        {
            string elapsed = _recorder.Elapsed.ToString(@"hh\:mm\:ss");
            SetFreeplayStatus("● 녹화 중", $"{elapsed} · 영상 {_recorder.FrameCount}프레임 · 녹화 중단을 누르면 저장됩니다.", Color.LightGreen);
        }
        else _status.Text = $"녹화 중 · 영상 {_recorder.FrameCount}프레임";
    }

    /// <summary>버튼으로 안내를 조작한 직후 입력 초점을 게임으로 되돌린다.</summary>
    private async void RestoreGameFocus()
    {
        if (_closing) return;
        try
        {
            using Process? process = _store.OwnedProcess(_session);
            if (process != null) await WindowsGame.FocusAsync(process.Id, CancellationToken.None);
        }
        catch (Exception error)
        {
            if (_freePlay && !_recordingStopped)
                _status.Text = _status.Text.Split('\n')[0] + "\n게임 초점 복귀 실패: " + error.Message;
            else if (!_freePlay) _status.Text = "게임 포커스 복귀 실패: " + error.Message;
        }
    }

    /// <summary>안내 창을 닫아도 게임을 종료하지 않고 녹화 파일만 안전하게 닫는다.</summary>
    protected override void OnFormClosing(FormClosingEventArgs e)
    {
        _closing = true;
        _statusTimer.Stop();
        try
        {
            StopRecording();
            if (!_freePlay) SaveState();
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
