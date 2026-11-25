// 지역 이미지의 DataManager 덤프를 실제 Client 필터·FILE·공유 버퍼·NULL Chat에 연결해 검사한다.
using System.Text;
using Netstorm.Assets;
using Netstorm.Client;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

// R4-7c3 미룬 검사 중 Template 진단 전체 Client 연결이다. cpppj tests/TemplateDiagnosticClientTests.cpp의
// TemplateDiagnosticClient_UsesSameFilterFileBufferAndActualNullChat을 옮겼다. 저장소가 하는 덤프 호출 부분은
// CampaignTemplateLoginTests가 먼저 대조했고 여기서는 실제 Client의 필터·FILE·공유 버퍼·Chat 연결을 대조한다.
// DN에 없던 Client 진단 전역·로그·장치만 더하고 제품 본체는 바꾸지 않는다. 1.002에서 추가됐다.
public sealed class TemplateDiagnosticClientTests
{
    /// <summary>같은 임시 CRT 파일만 소유하며 로그 장치의 본문을 검사 대체물로 바꾸지 않는다.</summary>
    private sealed class TemplateDiagnosticFile : IDisposable
    {
        /// <summary>실제 Client의 장기 로그 전역이다.</summary>
        private readonly TraceLogState _state;
        /// <summary>검사 시작 전에 쓰던 Stream이다.</summary>
        private readonly Stream? _previous;
        /// <summary>이 검사만 소유하는 실제 임시 파일이다.</summary>
        private readonly FileStream _file;
        /// <summary>검사 파일 경로다. 닫은 뒤 읽고 지운다.</summary>
        private readonly string _path;

        /// <summary>새 파일이 열린 뒤에만 현재 Stream을 바꾼다. 읽는 쪽과 공유한다.</summary>
        /// <param name="state">실제 Client의 장기 로그 전역이다.</param>
        public TemplateDiagnosticFile(TraceLogState state)
        {
            _state = state;
            _previous = state.Stream;
            _path = Path.GetTempFileName();
            _file = new FileStream(_path, FileMode.Open, FileAccess.ReadWrite, FileShare.ReadWrite);
            state.Stream = _file;
        }

        /// <summary>실제 출력 파일을 처음부터 읽으며 게임 데이터에는 쓰지 않는다.</summary>
        /// <returns>현재 CRT 출력 전체다.</returns>
        public string Text()
        {
            _file.Flush();
            _file.Seek(0, SeekOrigin.Begin);
            using var reader = new StreamReader(_file, Encoding.Latin1, leaveOpen: true);
            return reader.ReadToEnd();
        }

        /// <summary>Client의 빌림을 복원하고 자체 파일만 반납한다.</summary>
        public void Dispose()
        {
            _state.Stream = _previous;
            _file.Dispose();
            File.Delete(_path);
        }
    }

    /// <summary>실제 level 29 장치가 필터되면 공유 상태를 보존하고, 활성화되면 두 원문 로그를 같은 FILE에 쓴다.</summary>
    [Fact]
    public void UsesSameFilterFileBufferAndActualNullChat()
    {
        string? directory = GameDataLocator.FindDataDirectory();
        Assert.NotNull(directory);
        GameFileSystem files = GameFileSystem.Open(directory!);
        byte[] input = files.ReadAllBytes("d/thewarbegins.fort");
        var globals = new ClientTraceGlobals();
        var storage = new CampaignTemplateStorage();
        TraceLogHooks hooks = ClientDiagnosticLog.NullChatHooks(
            globals,
            (stream, text) =>
            {
                byte[] bytes = Encoding.Latin1.GetBytes(text);
                stream.Write(bytes, 0, bytes.Length);
            },
            stream => stream.Flush());
        storage.BindDiagnosticLog((level, text) => ClientDiagnosticLog.WriteDiagnosticLog(globals, level, text, hooks));
        var load = new TemplateFileLoadHooks { Read = _ => input, TypeCount = () => 188 };
        globals.Log.Level = 28;
        Array.Fill(globals.Log.Buffer, 'q');
        char[] before = (char[])globals.Log.Buffer.Clone();
        FortImageData filtered = storage.ReadLoginImage("source.fort", load, 0);
        Assert.True(filtered.End > 72 && globals.Log.Stream is null && globals.Log.Buffer.SequenceEqual(before) && storage.OwnedImages == 0);
        using var file = new TemplateDiagnosticFile(globals.Log);
        globals.Log.Level = 29;
        globals.Log.Indent = 2;
        globals.Log.Flush = 1;
        globals.TraceToChat = 1;
        FortImageData image = storage.ReadLoginImage("source.fort", load, 0);
        string total = $"  {image.End - 72:0000} bytes total.\n";
        Assert.Equal(filtered.Bytes().Length, image.Bytes().Length);
        Assert.True(image.Bytes().SequenceEqual(filtered.Bytes()));
        Assert.Equal($"  File NONAME: Stripped Image:\n{total}", file.Text());
        int nul = Array.IndexOf(globals.Log.Buffer, '\0');
        Assert.Equal(total, new string(globals.Log.Buffer, 0, nul));
        Assert.Equal(29, globals.Log.Level);
        Assert.Equal(2u, globals.Log.Indent);
        Assert.Equal(1u, globals.Log.Flush);
        Assert.Equal(0, storage.OwnedImages);
    }
}
