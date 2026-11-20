using System.Text;
using Netstorm.Assets;
using Netstorm.Core.Original;

// 서버 목록의 원문 바이트를 유지하며 물리 저장 위치만 사용자 Saves로 옮긴다.
namespace Netstorm.Client.Host;

/// <summary>cpppj client/RootServerListDisk의 실제 같은 NSInfo/파일 장치/사용자 경로 읽기와 저장이다. 통신은 실행하지 않는다.</summary>
public sealed class RootServerListDisk
{
    /// <summary>같은 현재 서버 목록과 체크섬의 장기 소유자다.</summary>
    private readonly ClientNsInfoGlobals _info;
    /// <summary>설치 자료와 원문 파일 이름 고리를 공유하는 실제 파일 장치다.</summary>
    private readonly GameFileSystem _files;
    /// <summary>DataDir 치환에 쓰는 실제 현재 설정이다.</summary>
    private readonly ConfigInterface _configuration;
    /// <summary>저장 파일의 절대 사용자 경로다. 읽기만 할 때는 폴더를 생성하지 않는다.</summary>
    public string UserFile { get; }

    /// <summary>같은 장기 소유자를 빌리며 정책을 생략해도 설치 경로로 쓰지 않는다.</summary>
    public RootServerListDisk(ClientNsInfoGlobals info, GameFileSystem files, ConfigInterface configuration, UserDataRoot? userData = null)
    {
        _info = info ?? throw new ArgumentNullException(nameof(info)); _files = files ?? throw new ArgumentNullException(nameof(files));
        _configuration = configuration ?? throw new ArgumentNullException(nameof(configuration));
        UserFile = Path.Combine((userData ?? UserDataRoot.Resolve()).Saves(), "!rootservers.dat");
    }

    /// <summary>사용자 파일→설치 !rootservers→설치 rootservers 순서다. 빈 파일도 성공이며 세 파일이 없으면 현재 목록을 보존한다.</summary>
    public void Load()
    {
        byte[]? bytes = ReadUserFile();
        bytes ??= _files.TryReadAllBytes(_configuration.SubstituteLocal("{DataDir}/!rootservers.dat"));
        bytes ??= _files.TryReadAllBytes(_configuration.SubstituteLocal("{DataDir}/rootservers.dat"));
        if (bytes is not null) _info.LoadServerNames(bytes);
    }

    /// <summary>원문 rb열기 실패만 설치 자료 조회로 넘기고, 열린 뒤 길이/부분읽기 실패는 숨기지 않는다.</summary>
    private byte[]? ReadUserFile()
    {
        FileStream file;
        try { file = new FileStream(UserFile, FileMode.Open, FileAccess.Read, FileShare.ReadWrite); }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { return null; }
        using (file)
        {
            byte[] bytes = new byte[checked((int)file.Length)]; file.ReadExactly(bytes); return bytes;
        }
    }

    /// <summary>원문 root/DataDir/파일 작성 효과 뒤 사용자 파일만 w+b로 쓰며 첫NUL과 LF 포함 여부를 그대로 적용한다.</summary>
    public void Save()
    {
        _files.FileNameOwner.Format([
            new(-2, Encoding.UTF8.GetBytes(Path.GetFullPath(_files.BaseDirectory))),
            new(-2, Encoding.UTF8.GetBytes(_configuration.SubstituteLocal("{DataDir}"))),
            new(-3, "!rootservers.dat"u8.ToArray())]);
        UserDataRoot.EnsureDirectory(Path.GetDirectoryName(UserFile)!);
        FileStream file;
        try { file = new FileStream(UserFile, FileMode.Create, FileAccess.ReadWrite, FileShare.ReadWrite); }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { return; }
        try
        {
            // 현재 각 행의 첫NUL까지만 쓰며 LF가 한 개도 없을 때만 LF를 하나 추가한다.
            foreach (string row in _info.Servers)
            {
                int nul = row.IndexOf('\0'); string text = nul < 0 ? row : row[..nul]; byte[] bytes = new byte[text.Length];
                // Unicode 대체문자로 원문 서버 BYTE를 손실시키지 않는다.
                for (int index = 0; index < text.Length; index++) bytes[index] = checked((byte)text[index]);
                Write(file, bytes); if (!text.Contains('\n')) Write(file, "\n"u8);
            }
        }
        finally
        {
            try { file.Dispose(); } catch (IOException) { /* 원문 fclose의 실패 반환값도 사용하지 않는다. */ }
        }
    }

    /// <summary>원문 fwrite처럼 쓰기 실패 반환값을 소비하지 않으며 열린 파일은 바깥 finally에서 반납한다.</summary>
    private static void Write(FileStream file, ReadOnlySpan<byte> bytes)
    {
        try { file.Write(bytes); } catch (IOException) { /* 원문 fwrite 실패 뒤에도 다음 행 처리는 계속한다. */ }
    }
}
