// 열린 음악 파일의 현재 토큰·헤더와 선택된 실제 파일 장치를 채널에 전달한다.
namespace Netstorm.Client.Audio;

/// <summary>cpppj SoundMusic.h의 음악 헤더 입출력이다. 실패한 열기도 실제 진행한 필드와 형식 접두부만 변경한다.</summary>
public sealed class MusicFileHeader
{
    /// <summary>호스트 파일 저장소가 발급한 실제 DWORD 토큰이다. 실패 뒤 닫힌 값이 남을 수 있다.</summary>
    public uint File { get; set; }
    /// <summary>현재 data 청크의 바이트 수다. 헤더 검사가 끝난 뒤에만 갱신한다.</summary>
    public uint Length { get; set; }
    /// <summary>같은 열린 파일의 data 절대 시작 위치다.</summary>
    public uint DataOffset { get; set; }
    /// <summary>원문 WAVEFORMATEX18바이트다. 짧은 읽기와 진단 예외에서도 이미 읽은 부분은 보존한다.</summary>
    public byte[] Format { get; } = new byte[18];
}

/// <summary>음악의 실제 경로 검색·열린 헤더와 상위 demo.mus 선택 경계다. 언어별 효과음 검색과 구별한다.</summary>
public sealed class MusicOpenHooks
{
    /// <summary>채널이 작성한 현재 주/보조 검색 문자열을 한 번 조회한다.</summary>
    public Func<string, string?>? Find { get; set; }
    /// <summary>실제 파일을 열어 같은 입력 헤더에 부분 출력과 토큰을 남긴다.</summary>
    public Func<string, MusicFileHeader, bool>? Open { get; set; }
    /// <summary>상위 음악 옵션과 현재 이름 소유자가 실행하는 실제 대체 곡 선택이다.</summary>
    public Action<string>? Fallback { get; set; }
}
